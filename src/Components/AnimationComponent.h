#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Rect.hpp>

#include "../Core/Component.h"

class AnimationComponent : public Component
{
public:
    void addAnimation(const std::string& name,
                      const std::string& texturePath,
                      const std::vector<sf::IntRect>& frames,
                      float frameDuration,
                      bool loop = true);

    void play(const std::string& name);
    void stop();

    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

    const std::string& currentAnimation() const { return m_current; }
    bool isPlaying() const { return m_playing; }

private:
    struct Animation
    {
        std::string texturePath;
        std::vector<sf::IntRect> frames;
        float frameDuration = 0.1f;
        bool loop = true;
    };

    sf::Texture& loadTexture(const std::string& path);

    std::unordered_map<std::string, Animation> m_animations;
    std::unordered_map<std::string, sf::Texture> m_textures;
    sf::Sprite m_sprite;

    std::string m_current;
    int m_frameIndex = 0;
    float m_elapsed  = 0.f;
    bool m_playing   = false;
};
