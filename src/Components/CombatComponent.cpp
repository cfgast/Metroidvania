#include "CombatComponent.h"
#include "InputComponent.h"
#include "PhysicsComponent.h"
#include "HealthComponent.h"
#include "../Core/GameObject.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <cmath>
#include <algorithm>

CombatComponent::CombatComponent(float damage, float attackDuration,
                                 float attackCooldown, float reach)
    : m_damage(damage)
    , m_attackDuration(attackDuration)
    , m_attackCooldown(attackCooldown)
    , m_reach(reach)
{
}

void CombatComponent::update(float dt)
{
    if (!getOwner())
        return;

    if (m_cooldownTimer > 0.f)
        m_cooldownTimer -= dt;

    auto* input = getOwner()->getComponent<InputComponent>();
    bool attackDown = input ? input->getInputState().attack : false;

    if (attackDown && !m_attackWasDown && !m_attacking && m_cooldownTimer <= 0.f)
    {
        m_attacking   = true;
        m_attackTimer = 0.f;
        m_hitEnemies.clear();
    }
    m_attackWasDown = attackDown;

    if (m_attacking)
    {
        m_attackTimer += dt;
        checkHits();

        if (m_attackTimer >= m_attackDuration)
        {
            m_attacking     = false;
            m_cooldownTimer = m_attackCooldown;
        }
    }
}

void CombatComponent::checkHits()
{
    if (!m_enemies || !getOwner())
        return;

    auto* physics = getOwner()->getComponent<PhysicsComponent>();
    if (!physics)
        return;

    const sf::Vector2f& pos  = getOwner()->position;
    const sf::Vector2f& size = physics->collisionSize;
    bool right = physics->facingRight();

    // Rectangular hitbox in front of the player
    float hLeft, hRight;
    if (right)
    {
        hLeft  = pos.x;
        hRight = pos.x + size.x * 0.5f + m_reach;
    }
    else
    {
        hLeft  = pos.x - size.x * 0.5f - m_reach;
        hRight = pos.x;
    }
    float hTop    = pos.y - size.y * 0.6f;
    float hBottom = pos.y + size.y * 0.6f;

    for (auto& enemy : *m_enemies)
    {
        if (m_hitEnemies.count(enemy.get()))
            continue;

        auto* hp = enemy->getComponent<HealthComponent>();
        if (!hp || hp->isDead())
            continue;

        auto* ePhys = enemy->getComponent<PhysicsComponent>();
        if (!ePhys)
            continue;

        const sf::Vector2f& ePos  = enemy->position;
        const sf::Vector2f& eSize = ePhys->collisionSize;

        float eLeft   = ePos.x - eSize.x * 0.5f;
        float eRight  = ePos.x + eSize.x * 0.5f;
        float eTop    = ePos.y - eSize.y * 0.5f;
        float eBottom = ePos.y + eSize.y * 0.5f;

        if (hLeft < eRight && hRight > eLeft && hTop < eBottom && hBottom > eTop)
        {
            hp->takeDamage(m_damage);
            m_hitEnemies.insert(enemy.get());
        }
    }
}

void CombatComponent::render(sf::RenderWindow& window)
{
    if (!m_attacking || !getOwner())
        return;

    auto* physics = getOwner()->getComponent<PhysicsComponent>();
    if (!physics)
        return;

    float progress = m_attackTimer / m_attackDuration;
    bool  right    = physics->facingRight();

    // Arc parameters
    const int   segments   = 20;
    const float innerR     = 12.f;
    const float outerR     = m_reach;
    const float startAngle = -1.3f;   // above horizontal (~-75°)
    const float totalSweep = 2.6f;    // full arc span (~149°)

    // Sweep grows quickly then holds
    float sweepProgress = std::min(progress * 2.0f, 1.0f);
    float currentSweep  = totalSweep * sweepProgress;

    // Fade out during the last 40% of the attack
    float baseAlpha = 220.f;
    if (progress > 0.6f)
        baseAlpha *= (1.f - (progress - 0.6f) / 0.4f);

    sf::Vector2f center = getOwner()->position;

    // Main arc as a TriangleStrip with per-vertex trail fade
    sf::VertexArray arc(sf::TrianglesStrip);

    for (int i = 0; i <= segments; ++i)
    {
        float t     = static_cast<float>(i) / segments;
        float angle = startAngle + currentSweep * t;

        float trailAlpha = 0.3f + 0.7f * t;
        sf::Uint8 a = static_cast<sf::Uint8>(
            std::max(0.f, std::min(255.f, baseAlpha * trailAlpha)));
        sf::Color color(220, 240, 255, a);

        float cosA = std::cos(angle);
        float sinA = std::sin(angle);

        float ix = innerR * cosA;
        float iy = innerR * sinA;
        float ox = outerR * cosA;
        float oy = outerR * sinA;

        if (!right) { ix = -ix; ox = -ox; }

        arc.append(sf::Vertex(sf::Vector2f(center.x + ix, center.y + iy), color));
        arc.append(sf::Vertex(sf::Vector2f(center.x + ox, center.y + oy), color));
    }

    window.draw(arc);

    // Bright leading-edge line
    if (currentSweep > 0.1f)
    {
        float edgeAngle = startAngle + currentSweep;
        float cosE = std::cos(edgeAngle);
        float sinE = std::sin(edgeAngle);

        float eix = innerR * cosE;
        float eiy = innerR * sinE;
        float eox = outerR * cosE;
        float eoy = outerR * sinE;

        if (!right) { eix = -eix; eox = -eox; }

        sf::Uint8 ea = static_cast<sf::Uint8>(
            std::max(0.f, std::min(255.f, baseAlpha)));

        sf::VertexArray edge(sf::Lines, 2);
        edge[0] = sf::Vertex(
            sf::Vector2f(center.x + eix, center.y + eiy),
            sf::Color(255, 255, 255, ea));
        edge[1] = sf::Vertex(
            sf::Vector2f(center.x + eox, center.y + eoy),
            sf::Color(255, 255, 255, ea));
        window.draw(edge);
    }
}
