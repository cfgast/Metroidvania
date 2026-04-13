#pragma once

#include <vector>
#include <memory>
#include <set>

#include "../Core/Component.h"

class GameObject;

class CombatComponent : public Component
{
public:
    CombatComponent(float damage = 17.f, float attackDuration = 0.3f,
                    float attackCooldown = 0.2f, float reach = 45.f);

    void update(float dt) override;
    void render(Renderer& renderer) override;

    bool isAttacking() const { return m_attacking; }
    bool isCharging()  const { return m_charging; }

    void setEnemies(std::vector<std::unique_ptr<GameObject>>* enemies) { m_enemies = enemies; }

    void setChargeUnlocked(bool unlocked) { m_chargeUnlocked = unlocked; }

    // Call this when the player takes damage to cancel any active charge.
    void onDamageTaken();

private:
    void checkHits();

    float m_damage;
    float m_attackDuration;
    float m_attackCooldown;
    float m_reach;

    bool  m_attacking     = false;
    float m_attackTimer   = 0.f;
    float m_cooldownTimer = 0.f;
    bool  m_attackWasDown = false;

    // Charged spin-slash state
    bool  m_chargeUnlocked = false;
    bool  m_charging       = false;
    float m_chargeTimer    = 0.f;
    float m_minChargeTime  = 0.6f;
    float m_spinDamageMult = 2.0f;
    float m_spinReachMult  = 1.5f;
    bool  m_spinSlashing   = false;   // True during a spin-slash attack

    std::set<GameObject*> m_hitEnemies;
    std::vector<std::unique_ptr<GameObject>>* m_enemies = nullptr;
};
