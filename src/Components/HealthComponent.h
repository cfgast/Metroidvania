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

    // Passive regeneration: heals regenRate HP/s after regenDelay seconds
    // without taking damage. Disabled by default (rate = 0).
    void setRegen(float regenRate, float regenDelay = 3.f);

    void update(float dt) override;

    std::function<void()> onDeath;
    std::function<void()> onDamage;

    void render(Renderer& renderer) override;

private:
    float m_maxHp;
    float m_currentHp;

    float m_regenRate     = 0.f;
    float m_regenDelay    = 3.f;
    float m_timeSinceDmg  = 0.f;

    static constexpr float BAR_WIDTH  = 50.f;
    static constexpr float BAR_HEIGHT = 6.f;
    static constexpr float BAR_OFFSET_Y = -40.f;
};
