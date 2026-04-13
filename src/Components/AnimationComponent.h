#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

#include "../Core/Component.h"

class Renderer;

class AnimationComponent : public Component
{
public:
    struct FrameRect { int x, y, w, h; };

    void addAnimation(const std::string& name,
                      const std::string& texturePath,
                      const std::vector<FrameRect>& frames,
                      float frameDuration,
                      bool loop = true);

    void play(const std::string& name);
    void stop();

    void update(float dt) override;
    void render(Renderer& renderer) override;

    const std::string& currentAnimation() const { return m_current; }
    bool isPlaying() const { return m_playing; }

    void setTintColor(float r, float g, float b) { m_tintR = r; m_tintG = g; m_tintB = b; }

private:
    using TextureHandle = uint64_t;

    struct Animation
    {
        std::string texturePath;
        std::vector<FrameRect> frames;
        float frameDuration = 0.1f;
        bool loop = true;
        mutable TextureHandle textureHandle = 0;
    };

    std::unordered_map<std::string, Animation> m_animations;

    std::string m_current;
    int m_frameIndex = 0;
    float m_elapsed  = 0.f;
    bool m_playing   = false;

    float m_tintR = 1.f;
    float m_tintG = 1.f;
    float m_tintB = 1.f;
};
