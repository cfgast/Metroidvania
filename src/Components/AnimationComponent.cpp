#include "AnimationComponent.h"

#include "../Core/GameObject.h"
#include "../Rendering/Renderer.h"

void AnimationComponent::addAnimation(const std::string& name,
                                       const std::string& texturePath,
                                       const std::vector<FrameRect>& frames,
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
}

void AnimationComponent::render(Renderer& renderer)
{
    if (m_current.empty())
        return;

    auto it = m_animations.find(m_current);
    if (it == m_animations.end())
        return;

    const auto& anim = it->second;
    if (anim.frames.empty())
        return;

    // Lazy-load texture handle on first render
    if (anim.textureHandle == 0)
        anim.textureHandle = renderer.loadTexture(anim.texturePath);

    const auto& frame = anim.frames[m_frameIndex];
    float x = getOwner() ? getOwner()->position.x : 0.f;
    float y = getOwner() ? getOwner()->position.y : 0.f;

    renderer.drawSprite(anim.textureHandle, x, y,
                        frame.x, frame.y, frame.w, frame.h,
                        frame.w * 0.5f, frame.h * 0.5f,
                        m_tintR, m_tintG, m_tintB);
}
