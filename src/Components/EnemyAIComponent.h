#pragma once

#include <SFML/System/Vector2.hpp>

#include "../Core/Component.h"

class GameObject;

class EnemyAIComponent : public Component
{
public:
    EnemyAIComponent(GameObject& player, sf::Vector2f waypointA,
                     sf::Vector2f waypointB, float damage = 10.f,
                     float damageInterval = 0.5f);

    void update(float dt) override;

private:
    bool checkPlayerOverlap() const;

    GameObject&  m_player;
    sf::Vector2f m_waypointA;
    sf::Vector2f m_waypointB;
    bool         m_movingToB      = true;
    float        m_damage;
    float        m_damageInterval;
    float        m_damageCooldown = 0.f;
    sf::Vector2f m_prevPosition;
    bool         m_initialized    = false;
    float        m_stuckTimer     = 0.f;

    static constexpr float STUCK_THRESHOLD = 0.5f;
    static constexpr float STUCK_TIME      = 0.15f;
};
