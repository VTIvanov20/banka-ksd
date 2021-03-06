#include "./BackgroundComponent.hpp"

void BackgroundComponent::OnCreate()
{
    /**
     * @brief Change the background texture
     */
    // tex = ObjectManager::GetInstance()->CreateObject<TextureObject>();
    // bgTex.lock()->LoadFromFile("res/img/themes/wildwest/bg.png");
    if (bgTex.id == 0)
        bgTex = LoadTexture("res/img/themes/wildwest/bg.png");
}

void BackgroundComponent::OnUpdate()
{
    /**
     * @brief Draw texture with extra properties
     */
    DrawTexturePro(bgTex, {
        0, 0,
        static_cast<float>(bgTex.width), static_cast<float>(bgTex.height)
    }, {
        0.f, 0.f,
        static_cast<float>(GetScreenWidth()),
        static_cast<float>(GetScreenHeight())
    }, {0.f, 0.f}, 0.f, { 255, 255, 255, 191 });
}

void BackgroundComponent::OnDestroy()
{
    /**
     * @brief Unload the background texture
     */
    UnloadTexture(bgTex);
}

void BackgroundComponent::ChangeBGFile(std::string file)
{
    /**
     * @brief Change the background file by unloading it and loading the new texture
     */
    UnloadTexture(bgTex);
    bgTex = LoadTexture(file.c_str());
}