#pragma once

#include "../Core/Component.h"

class Renderer;

class RenderComponent : public Component
{
public:
    RenderComponent(float width, float height,
                    float r = 1.f, float g = 1.f, float b = 1.f, float a = 1.f);

    void update(float dt) override;
    void render(Renderer& renderer) override;

private:
    float m_width;
    float m_height;
    float m_r, m_g, m_b, m_a;
};
