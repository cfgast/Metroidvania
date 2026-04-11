#include "RenderComponent.h"
#include "AnimationComponent.h"

#include "../Core/GameObject.h"
#include "../Rendering/Renderer.h"

RenderComponent::RenderComponent(float width, float height,
                                 float r, float g, float b, float a)
    : m_width(width)
    , m_height(height)
    , m_r(r), m_g(g), m_b(b), m_a(a)
{
}

void RenderComponent::update(float /*dt*/)
{
}

void RenderComponent::render(Renderer& renderer)
{
    // Bypass when an AnimationComponent is handling visuals.
    if (getOwner() && getOwner()->getComponent<AnimationComponent>())
        return;

    if (!getOwner())
        return;

    float x = getOwner()->position.x - m_width * 0.5f;
    float y = getOwner()->position.y - m_height * 0.5f;
    renderer.drawRect(x, y, m_width, m_height, m_r, m_g, m_b, m_a);
}
