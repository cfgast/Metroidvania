#pragma once

#include <glm/vec2.hpp>

#include "../Core/Component.h"

class GameObject;

class EnemyAIComponent : public Component
{
public:
    EnemyAIComponent(GameObject& player, glm::vec2 waypointA,
                     glm::vec2 waypointB, float damage = 10.f,
                     float damageInterval = 0.5f);

    void update(float dt) override;

    void setPaused(bool paused) { m_paused = paused; }
    bool isPaused() const { return m_paused; }

private:
    bool checkPlayerOverlap() const;

    GameObject&  m_player;
    glm::vec2    m_waypointA;
    glm::vec2    m_waypointB;
    bool         m_movingToB      = true;
    float        m_damage;
    float        m_damageInterval;
    float        m_damageCooldown = 0.f;
    glm::vec2    m_prevPosition   { 0.f, 0.f };
    bool         m_initialized    = false;
    float        m_stuckTimer     = 0.f;
    bool         m_paused         = false;

    static constexpr float STUCK_THRESHOLD = 0.5f;
    static constexpr float STUCK_TIME      = 0.15f;
};
