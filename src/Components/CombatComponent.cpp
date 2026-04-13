#include "CombatComponent.h"
#include "InputComponent.h"
#include "PhysicsComponent.h"
#include "HealthComponent.h"
#include "AnimationComponent.h"
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

void CombatComponent::onDamageTaken()
{
    if (m_charging)
    {
        m_charging    = false;
        m_chargeTimer = 0.f;
    }
}

void CombatComponent::update(float dt)
{
    if (!getOwner())
        return;

    if (m_cooldownTimer > 0.f)
        m_cooldownTimer -= dt;

    auto* input = getOwner()->getComponent<InputComponent>();
    bool attackDown = input ? input->getInputState().attack : false;

    bool risingEdge  = attackDown && !m_attackWasDown;
    bool fallingEdge = !attackDown && m_attackWasDown;

    // --- Charging logic ---
    if (m_charging)
    {
        m_chargeTimer += dt;

        if (fallingEdge)
        {
            m_charging = false;
            if (m_chargeTimer >= m_minChargeTime)
            {
                // Execute spin slash
                m_attacking    = true;
                m_spinSlashing = true;
                m_attackTimer  = 0.f;
                m_hitEnemies.clear();

                // Play spin-slash animation
                auto* physics = getOwner()->getComponent<PhysicsComponent>();
                auto* anim    = getOwner()->getComponent<AnimationComponent>();
                if (anim && physics)
                {
                    const char* animName = physics->facingRight()
                        ? "spin-slash-right" : "spin-slash-left";
                    anim->play(animName);
                }
            }
            else
            {
                // Charge too short – fire a normal attack
                m_attacking    = true;
                m_spinSlashing = false;
                m_attackTimer  = 0.f;
                m_hitEnemies.clear();
            }
            m_chargeTimer = 0.f;
        }
    }
    // --- Normal rising-edge attack ---
    else if (risingEdge && !m_attacking && m_cooldownTimer <= 0.f)
    {
        if (m_chargeUnlocked)
        {
            m_charging    = true;
            m_chargeTimer = 0.f;
        }
        else
        {
            m_attacking    = true;
            m_spinSlashing = false;
            m_attackTimer  = 0.f;
            m_hitEnemies.clear();
        }
    }

    m_attackWasDown = attackDown;

    if (m_attacking)
    {
        m_attackTimer += dt;
        checkHits();

        float duration = m_spinSlashing ? 0.4f : m_attackDuration;
        if (m_attackTimer >= duration)
        {
            m_attacking     = false;
            m_spinSlashing  = false;
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

    float reach = m_reach;
    float damage = m_damage;
    if (m_spinSlashing)
    {
        reach  *= m_spinReachMult;
        damage *= m_spinDamageMult;
    }

    // Rectangular hitbox in front of the player
    float hLeft, hRight;
    if (right)
    {
        hLeft  = pos.x;
        hRight = pos.x + size.x * 0.5f + reach;
    }
    else
    {
        hLeft  = pos.x - size.x * 0.5f - reach;
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
            hp->takeDamage(damage);
            m_hitEnemies.insert(enemy.get());
        }
    }
}

void CombatComponent::render(Renderer& renderer)
{
    if (!getOwner())
        return;

    auto* physics = getOwner()->getComponent<PhysicsComponent>();
    if (!physics)
        return;

    float cx = getOwner()->position.x;
    float cy = getOwner()->position.y;

    // --- Charge visual: pulsing glow around the player while holding ---
    if (m_charging)
    {
        float pulse     = 0.5f + 0.5f * std::sin(m_chargeTimer * 12.f);
        float baseR     = 8.f + m_chargeTimer * 30.f;
        float radius    = std::min(baseR, 30.f);
        float alpha     = 0.15f + 0.2f * pulse;

        if (m_chargeTimer >= m_minChargeTime)
        {
            // Fully charged – brighter, steadier glow
            alpha  = 0.35f + 0.1f * pulse;
            radius = 30.f;
        }

        renderer.drawCircle(cx, cy, radius,
                            0.4f, 0.7f, 1.0f, alpha);
    }

    if (!m_attacking)
        return;

    bool  right    = physics->facingRight();
    float duration = m_spinSlashing ? 0.4f : m_attackDuration;
    float progress = m_attackTimer / duration;

    if (m_spinSlashing)
    {
        // Wider, brighter arc for spin slash
        const int   segments   = 28;
        const float innerR     = 10.f;
        const float outerR     = m_reach * m_spinReachMult;
        const float startAngle = -1.6f;
        const float totalSweep = 3.2f;

        float sweepProgress = std::min(progress * 2.0f, 1.0f);
        float currentSweep  = totalSweep * sweepProgress;

        float baseAlpha = 240.f / 255.f;
        if (progress > 0.5f)
            baseAlpha *= (1.f - (progress - 0.5f) / 0.5f);

        std::vector<Renderer::Vertex> arcVerts;
        arcVerts.reserve(static_cast<size_t>((segments + 1) * 2));

        for (int i = 0; i <= segments; ++i)
        {
            float t     = static_cast<float>(i) / segments;
            float angle = startAngle + currentSweep * t;

            float trailAlpha = 0.3f + 0.7f * t;
            float a  = std::max(0.f, std::min(1.f, baseAlpha * trailAlpha));
            float cr = 0.5f, cg = 0.8f, cb = 1.f;

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
    else
    {
        // Normal attack arc (existing visuals)
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
}
