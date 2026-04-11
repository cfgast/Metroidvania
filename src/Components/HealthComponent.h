#pragma once

#include <functional>

#include "../Core/Component.h"

class Renderer;

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

    void render(Renderer& renderer) override;

private:
    float m_maxHp;
    float m_currentHp;

    static constexpr float BAR_WIDTH  = 50.f;
    static constexpr float BAR_HEIGHT = 6.f;
    static constexpr float BAR_OFFSET_Y = -40.f;
};
