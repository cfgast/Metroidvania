#include "CombatComponent.h"
#include "InputComponent.h"
#include "PhysicsComponent.h"
#include "HealthComponent.h"
#include "../Core/GameObject.h"
#include "../Rendering/Renderer.h"

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

    const glm::vec2& pos  = getOwner()->position;
    const glm::vec2& size = physics->collisionSize;
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

        const glm::vec2& ePos  = enemy->position;
        const glm::vec2& eSize = ePhys->collisionSize;

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

void CombatComponent::render(Renderer& renderer)
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
    const float startAngle = -1.3f;
    const float totalSweep = 2.6f;

    float sweepProgress = std::min(progress * 2.0f, 1.0f);
    float currentSweep  = totalSweep * sweepProgress;

    float baseAlpha = 220.f / 255.f;
    if (progress > 0.6f)
        baseAlpha *= (1.f - (progress - 0.6f) / 0.4f);

    float cx = getOwner()->position.x;
    float cy = getOwner()->position.y;

    // Main arc as a TriangleStrip with per-vertex trail fade
    std::vector<Renderer::Vertex> arcVerts;
    arcVerts.reserve(static_cast<size_t>((segments + 1) * 2));

    for (int i = 0; i <= segments; ++i)
    {
        float t     = static_cast<float>(i) / segments;
        float angle = startAngle + currentSweep * t;

        float trailAlpha = 0.3f + 0.7f * t;
        float a = std::max(0.f, std::min(1.f, baseAlpha * trailAlpha));
        float cr = 220.f / 255.f, cg = 240.f / 255.f, cb = 1.f;

        float cosA = std::cos(angle);
        float sinA = std::sin(angle);

        float ix = innerR * cosA;
        float iy = innerR * sinA;
        float ox = outerR * cosA;
        float oy = outerR * sinA;

        if (!right) { ix = -ix; ox = -ox; }

        arcVerts.push_back({cx + ix, cy + iy, cr, cg, cb, a});
        arcVerts.push_back({cx + ox, cy + oy, cr, cg, cb, a});
    }

    renderer.drawTriangleStrip(arcVerts);

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

        float ea = std::max(0.f, std::min(1.f, baseAlpha));

        std::vector<Renderer::Vertex> edgeVerts = {
            {cx + eix, cy + eiy, 1.f, 1.f, 1.f, ea},
            {cx + eox, cy + eoy, 1.f, 1.f, 1.f, ea}
        };
        renderer.drawLines(edgeVerts);
    }
}
