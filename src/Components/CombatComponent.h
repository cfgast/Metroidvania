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
    void render(sf::RenderWindow& window) override;

    bool isAttacking() const { return m_attacking; }

    void setEnemies(std::vector<std::unique_ptr<GameObject>>* enemies) { m_enemies = enemies; }

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

    std::set<GameObject*> m_hitEnemies;
    std::vector<std::unique_ptr<GameObject>>* m_enemies = nullptr;
};
