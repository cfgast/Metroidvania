#include "AnimationComponent.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Image.hpp>

#include "../Core/GameObject.h"

void AnimationComponent::addAnimation(const std::string& name,
                                       const std::string& texturePath,
                                       const std::vector<sf::IntRect>& frames,
                                       float frameDuration,
                                       bool loop)
{
    Animation anim;
    anim.texturePath  = texturePath;
    anim.frames       = frames;
    anim.frameDuration = frameDuration;
    anim.loop         = loop;
    m_animations[name] = std::move(anim);
}

sf::Texture& AnimationComponent::loadTexture(const std::string& path)
{
    auto it = m_textures.find(path);
    if (it != m_textures.end())
        return it->second;

    sf::Texture& tex = m_textures[path];
    if (!tex.loadFromFile(path))
    {
        // Fallback: create a solid magenta texture so missing art is obvious.
        sf::Image img;
        img.create(256, 256, sf::Color::Magenta);
        tex.loadFromImage(img);
    }
    return tex;
}

void AnimationComponent::play(const std::string& name)
{
    // If this animation is already playing, keep going without reset.
    if (m_current == name && m_playing)
        return;

    auto it = m_animations.find(name);
    if (it == m_animations.end())
        return;

    m_current    = name;
    m_frameIndex = 0;
    m_elapsed    = 0.f;
    m_playing    = true;

    const auto& anim = it->second;
    sf::Texture& tex = loadTexture(anim.texturePath);
    m_sprite.setTexture(tex, true);
    if (!anim.frames.empty())
    {
        m_sprite.setTextureRect(anim.frames[0]);
        m_sprite.setOrigin(anim.frames[0].width * 0.5f,
                           anim.frames[0].height * 0.5f);
    }
}

void AnimationComponent::stop()
{
    m_playing = false;
}

void AnimationComponent::update(float dt)
{
    if (!m_playing || m_current.empty())
        return;

    auto it = m_animations.find(m_current);
    if (it == m_animations.end())
        return;

    const auto& anim = it->second;
    const int frameCount = static_cast<int>(anim.frames.size());

    m_elapsed += dt;
    while (m_elapsed >= anim.frameDuration && frameCount > 0)
    {
        m_elapsed -= anim.frameDuration;
        m_frameIndex++;

        if (m_frameIndex >= frameCount)
        {
            if (anim.loop)
            {
                m_frameIndex = 0;
            }
            else
            {
                m_frameIndex = frameCount - 1;
                m_playing = false;
                break;
            }
        }
    }

    if (!anim.frames.empty())
    {
        m_sprite.setTextureRect(anim.frames[m_frameIndex]);
        m_sprite.setOrigin(anim.frames[m_frameIndex].width * 0.5f,
                           anim.frames[m_frameIndex].height * 0.5f);
    }

    if (getOwner())
        m_sprite.setPosition(getOwner()->position);
}

void AnimationComponent::render(sf::RenderWindow& window)
{
    if (!m_current.empty())
        window.draw(m_sprite);
}
