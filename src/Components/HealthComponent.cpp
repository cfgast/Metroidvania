#include "HealthComponent.h"

#include <algorithm>

#include "../Core/GameObject.h"
#include "../Rendering/Renderer.h"

HealthComponent::HealthComponent(float maxHp)
    : m_maxHp(maxHp)
    , m_currentHp(maxHp)
{
}

void HealthComponent::takeDamage(float amount)
{
    if (m_currentHp <= 0.f)
        return;

    m_currentHp = std::max(0.f, m_currentHp - amount);

    if (m_currentHp <= 0.f && onDeath)
        onDeath();
}

void HealthComponent::heal(float amount)
{
    if (m_currentHp <= 0.f)
        return;

    m_currentHp = std::min(m_maxHp, m_currentHp + amount);
}

void HealthComponent::render(Renderer& renderer)
{
    if (!getOwner())
        return;

    float barX = getOwner()->position.x - BAR_WIDTH * 0.5f;
    float barY = getOwner()->position.y + BAR_OFFSET_Y - BAR_HEIGHT * 0.5f;

    // Background with outline
    renderer.drawRectOutlined(barX, barY, BAR_WIDTH, BAR_HEIGHT,
                              60.f / 255.f, 60.f / 255.f, 60.f / 255.f, 1.f,
                              0.f, 0.f, 0.f, 1.f,
                              1.f);

    // Fill bar — colour shifts from green to red as HP drops
    float ratio = (m_maxHp > 0.f) ? (m_currentHp / m_maxHp) : 0.f;
    float fillWidth = BAR_WIDTH * ratio;
    float r = 1.f - ratio;
    float g = ratio;

    renderer.drawRect(barX, barY, fillWidth, BAR_HEIGHT, r, g, 0.f, 1.f);
}
