#pragma once

#include <functional>

#include <SFML/Graphics/RectangleShape.hpp>

#include "../Core/Component.h"

class HealthComponent : public Component
{
public:
    explicit HealthComponent(float maxHp);

    void takeDamage(float amount);
    void heal(float amount);

    float getCurrentHp() const { return m_currentHp; }
    float getMaxHp() const { return m_maxHp; }
    bool  isDead() const { return m_currentHp <= 0.f; }

    std::function<void()> onDeath;

    void render(sf::RenderWindow& window) override;

private:
    float m_maxHp;
    float m_currentHp;

    // HP bar visuals
    sf::RectangleShape m_barBackground;
    sf::RectangleShape m_barFill;

    static constexpr float BAR_WIDTH  = 50.f;
    static constexpr float BAR_HEIGHT = 6.f;
    static constexpr float BAR_OFFSET_Y = -40.f;
};
