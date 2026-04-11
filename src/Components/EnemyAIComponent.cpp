#include "EnemyAIComponent.h"
#include "InputComponent.h"
#include "PhysicsComponent.h"
#include "HealthComponent.h"

#include "../Core/GameObject.h"

#include <cmath>

EnemyAIComponent::EnemyAIComponent(GameObject& player, glm::vec2 waypointA,
                                   glm::vec2 waypointB, float damage,
                                   float damageInterval)
    : m_player(player)
    , m_waypointA(waypointA)
    , m_waypointB(waypointB)
    , m_damage(damage)
    , m_damageInterval(damageInterval)
{
}

bool EnemyAIComponent::checkPlayerOverlap() const
{
    auto* ownerPhys  = getOwner()->getComponent<PhysicsComponent>();
    auto* playerPhys = m_player.getComponent<PhysicsComponent>();
    if (!ownerPhys || !playerPhys)
        return false;

    const glm::vec2& ePos  = getOwner()->position;
    const glm::vec2& eSize = ownerPhys->collisionSize;
    const glm::vec2& pPos  = m_player.position;
    const glm::vec2& pSize = playerPhys->collisionSize;

    return std::abs(ePos.x - pPos.x) < (eSize.x + pSize.x) * 0.5f &&
           std::abs(ePos.y - pPos.y) < (eSize.y + pSize.y) * 0.5f;
}

void EnemyAIComponent::update(float dt)
{
    if (!getOwner() || dt <= 0.f)
        return;

    // Skip if this enemy is dead.
    if (auto* hp = getOwner()->getComponent<HealthComponent>())
    {
        if (hp->isDead())
            return;
    }

    // When paused (e.g. during slime jitter attack), stop moving but
    // still handle contact damage cooldown below.
    if (m_paused)
    {
        if (auto* ic = getOwner()->getComponent<InputComponent>())
            ic->setInputState(InputState{});

        // Still tick damage cooldown and check contact damage.
        if (m_damageCooldown > 0.f)
            m_damageCooldown -= dt;

        if (m_damageCooldown <= 0.f && checkPlayerOverlap())
        {
            if (auto* playerHp = m_player.getComponent<HealthComponent>())
            {
                playerHp->takeDamage(m_damage);
                m_damageCooldown = m_damageInterval;
            }
        }
        return;
    }

    // Initialize previous-position tracking on first frame.
    if (!m_initialized)
    {
        m_prevPosition = getOwner()->position;
        m_initialized  = true;
    }

    // --- Patrol logic: walk toward the current target waypoint ---
    float targetX = m_movingToB ? m_waypointB.x : m_waypointA.x;
    float dx      = targetX - getOwner()->position.x;

    InputState input;

    if (dx > 1.f)
        input.moveRight = true;
    else if (dx < -1.f)
        input.moveLeft = true;
    else
    {
        // Reached the waypoint – reverse direction.
        m_movingToB = !m_movingToB;
        targetX     = m_movingToB ? m_waypointB.x : m_waypointA.x;
        dx          = targetX - getOwner()->position.x;
        if (dx > 0.f)
            input.moveRight = true;
        else
            input.moveLeft = true;
    }

    // --- Wall detection: reverse if stuck for a short time ---
    float movedX = std::abs(getOwner()->position.x - m_prevPosition.x);
    if (movedX < STUCK_THRESHOLD)
    {
        m_stuckTimer += dt;
        if (m_stuckTimer >= STUCK_TIME)
        {
            m_movingToB  = !m_movingToB;
            m_stuckTimer = 0.f;
        }
    }
    else
    {
        m_stuckTimer = 0.f;
    }

    m_prevPosition = getOwner()->position;

    // Push the desired movement into the sibling InputComponent.
    if (auto* ic = getOwner()->getComponent<InputComponent>())
        ic->setInputState(input);

    // --- Contact damage to the player ---
    if (m_damageCooldown > 0.f)
        m_damageCooldown -= dt;

    if (m_damageCooldown <= 0.f && checkPlayerOverlap())
    {
        if (auto* playerHp = m_player.getComponent<HealthComponent>())
        {
            playerHp->takeDamage(m_damage);
            m_damageCooldown = m_damageInterval;
        }
    }
}
