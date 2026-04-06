#include "HealthComponent.h"

#include <algorithm>

#include <SFML/Graphics/RenderWindow.hpp>

#include "../Core/GameObject.h"

HealthComponent::HealthComponent(float maxHp)
    : m_maxHp(maxHp)
    , m_currentHp(maxHp)
{
    m_barBackground.setSize({ BAR_WIDTH, BAR_HEIGHT });
    m_barBackground.setFillColor(sf::Color(60, 60, 60));
    m_barBackground.setOutlineColor(sf::Color::Black);
    m_barBackground.setOutlineThickness(1.f);
    m_barBackground.setOrigin(BAR_WIDTH * 0.5f, BAR_HEIGHT * 0.5f);

    m_barFill.setSize({ BAR_WIDTH, BAR_HEIGHT });
    m_barFill.setFillColor(sf::Color::Green);
    m_barFill.setOrigin(BAR_WIDTH * 0.5f, BAR_HEIGHT * 0.5f);
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

void HealthComponent::render(sf::RenderWindow& window)
{
    if (!getOwner())
        return;

    sf::Vector2f barPos = getOwner()->position;
    barPos.y += BAR_OFFSET_Y;

    m_barBackground.setPosition(barPos);
    window.draw(m_barBackground);

    float ratio = (m_maxHp > 0.f) ? (m_currentHp / m_maxHp) : 0.f;
    float fillWidth = BAR_WIDTH * ratio;
    m_barFill.setSize({ fillWidth, BAR_HEIGHT });
    m_barFill.setOrigin(BAR_WIDTH * 0.5f, BAR_HEIGHT * 0.5f);
    m_barFill.setPosition(barPos);

    // Colour shifts from green to red as HP drops
    sf::Uint8 r = static_cast<sf::Uint8>((1.f - ratio) * 255);
    sf::Uint8 g = static_cast<sf::Uint8>(ratio * 255);
    m_barFill.setFillColor(sf::Color(r, g, 0));

    window.draw(m_barFill);
}
