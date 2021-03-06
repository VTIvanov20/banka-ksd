#include "./NetworkController.hpp"

#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_CHRONO_
#define _WEBSOCKETPP_CPP11_MEMORY_
#define _WEBSOCKETPP_CPP11_FUNCTIONAL_
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_REGEX_
#define _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
#define _WEBSOCKETPP_CPP11_THREAD_
#define _WEBSOCKETPP_INITIALIZER_LISTS_

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include "../util/json.hpp"

using client = websocketpp::client<websocketpp::config::asio_client>;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// pull out the type of messages sent by our config
using message_ptr = websocketpp::config::asio_client::message_type::ptr;

#include <memory>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <mutex>
#include <array>

static std::atomic<client *> wsClient;
static std::unique_ptr<std::thread> wsThread;

static std::mutex connHandleMutex;
static websocketpp::connection_hdl connHandle;

static std::mutex stateMutex;
static std::shared_ptr<NetworkState> networkState;

void OnMessage(client*, websocketpp::connection_hdl hdl, message_ptr msg)
{
    websocketpp::lib::error_code ec;
    printf("INFO: -> %s\n", msg->get_payload().c_str());

    nlohmann::json obj = nlohmann::json::parse(msg->get_payload());

    stateMutex.lock();
    std::string type{};
    obj["t"].get_to(type);
    try
    {
        if (type == "create-room-success")
        {
            networkState->inGame = true;
            obj["d"]["code"].get_to(networkState->code);
        }
        else if (type == "join-room-success")
        {
            networkState->inGame = true;
            obj["d"]["code"].get_to(networkState->code);
        }
        else if (type == "room-close")
        {
            networkState = std::make_unique<NetworkState>();
            networkState->hasError = true;
            obj["d"]["reason"].get_to(networkState->lastErrorReason);
        }
        else if (type == "room-begin")
        {
            networkState->gameStarted = true;
        }
        else if (type == "status")
        {
            networkState->currentTurn = obj["d"]["canPlace"].get<bool>() ? Turn::PLAYER : Turn::OPPONENT;

            networkState->playerDeck = {};
            for (auto card : obj["d"]["playerDeck"])
                networkState->playerDeck.push_back(card.get<CardType>());

            networkState->cardState = {};
            uint8_t idx = 0;
            for (auto state : obj["d"]["placedCards"])
            {
                networkState->cardState[idx].val = state["val"].get<CardType>();

                {
                    auto jState = state["topNext"];
                    auto *cState = &networkState->cardState[idx].topNext;
                    while (true)
                    {
                        *cState = std::make_shared<Node<CardType>>();
                        try {
                            jState["val"].get_to((*cState)->val);
                            jState = jState["next"];
                            cState = &(*cState)->next;
                        } catch (std::exception const&) {
                            (*cState).reset();
                            break;
                        }
                    }
                }

                {
                    auto jState = state["bottomNext"];
                    auto *cState = &networkState->cardState[idx].bottomNext;
                    while (true)
                    {
                        *cState = std::make_shared<Node<CardType>>();
                        try {
                            jState["val"].get_to((*cState)->val);
                            jState = jState["next"];
                            cState = &(*cState)->next;
                        } catch (std::exception const&) {
                            (*cState).reset();
                            break;
                        }
                    }
                }
                
                idx++;
            }
        }
    }
    catch(nlohmann::json::exception &e)
    {
        printf("WARN: Error parsing websocket packet %s\n", e.what());
    }
    stateMutex.unlock();
    
    connHandleMutex.lock();
    connHandle = hdl;
    connHandleMutex.unlock();
}

void NetworkController::OnCreate()
{
    if (!alreadyInit)
        alreadyInit = true;
    else return;

    wsClient.store(new client());
    wsClient.load()->set_access_channels(websocketpp::log::alevel::none);
    wsClient.load()->init_asio();
    wsClient.load()->set_message_handler(bind(&OnMessage, wsClient.load(), ::_1, ::_2));
    networkState = std::make_unique<NetworkState>();

    websocketpp::lib::error_code err_code;
    client::connection_ptr conn = wsClient.load()->get_connection(WsUri, err_code);

    if (err_code) {
        printf("could not create connection because: %s\n", err_code.message().c_str());
    }

    wsClient.load()->connect(conn);

    wsThread = std::make_unique<std::thread>([]() {
        while (!wsClient.load()->stopped())
            wsClient.load()->run_one();
    });
}

void NetworkController::OnUpdate()
{
    static int delay = 0;

    if (IsActive())
    {
        if (delay % 1000 == 0)
        {
            SendPacket(nlohmann::json({
                {"t", "keep-alive"}
            }));
            delay = 0;
        }

        delay++;
    }
}

void NetworkController::CreateRoom()
{
    SendPacket(nlohmann::json({
        {"t", "create-room"}
    }));
}

void NetworkController::LeaveRoom()
{
    SendPacket(nlohmann::json({
        {"t", "leave-room"}
    }));
}

void NetworkController::JoinRoom(std::string code)
{
    SendPacket(nlohmann::json({
        {"t", "join-room"},
        {"d", {{ "code", code }}}
    }));
}

void NetworkController::SendPacket(nlohmann::json json)
{
    connHandleMutex.lock();
    if (connHandle.expired()) {
        printf("INFO: NetworkManager: Socket not ready for sending\n");
        connHandleMutex.unlock();
        return;
    }

    websocketpp::lib::error_code ec;
    wsClient.load()->send(connHandle, json.dump(), websocketpp::frame::opcode::value::BINARY, ec);
    connHandleMutex.unlock();

    printf("INFO: <- %s\n", json.dump().c_str());
}

void NetworkController::OnDestroy()
{
    wsClient.load()->stop();
    wsThread->join();

    delete wsClient.load();
    wsClient.store(nullptr);
    wsThread.reset();
}

NetworkState NetworkController::GetState()
{
    stateMutex.lock();
    NetworkState copy { *networkState };
    stateMutex.unlock();

    return copy;
}

bool NetworkController::IsActive()
{
    return !wsClient.load()->stopped();
}