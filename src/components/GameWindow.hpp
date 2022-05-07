#include "./Component.hpp"
#include "../util/graphics.hpp"

#include "./CardTypes.hpp"
#include "./GameController.hpp"

class GameWindow : public Component
{
public:
    DEFINE_COMPONENT(GameWindow);

    void OnCreate() override;
    void OnUpdate() override;
    void OnDestroy() override;
    void OnUI() override;

private:
    void DrawBeginningNode(BeginningNode<CardType> top, Vec2f idx);
    void DrawNode(Node<CardType> top, Vec2f idx, VerticalCardPos vertPos);
    void DrawFromType(CardType type, Vec2f index, Vec2f spacing);

    std::weak_ptr<TextureObject> state;
    std::weak_ptr<TextureObject> andZero;
    std::weak_ptr<TextureObject> andOne;
    std::weak_ptr<TextureObject> orZero;
    std::weak_ptr<TextureObject> orOne;
    std::weak_ptr<TextureObject> xorZero;
    std::weak_ptr<TextureObject> xorOne;

    std::weak_ptr<TextureObject> renderTexture;

    const Vec2f scaleTo { 142.f, 204.f };
    const Vec2f spacing { 10.f, 10.f };
};