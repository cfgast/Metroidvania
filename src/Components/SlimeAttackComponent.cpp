#include "SlimeAttackComponent.h"
#include "EnemyAIComponent.h"
#include "InputComponent.h"
#include "AnimationComponent.h"
#include "PhysicsComponent.h"
#include "HealthComponent.h"

#include "../Core/GameObject.h"

#include <SFML/Graphics/RenderWindow.hpp>

#include <cmath>
#include <cstdlib>

static constexpr float PI = 3.14159265f;

SlimeAttackComponent::SlimeAttackComponent(GameObject& player, float damage)
    : m_player(player)
    , m_damage(damage)
{
    m_cooldownTimer = randomFloat(MIN_COOLDOWN, MAX_COOLDOWN);
}

float SlimeAttackComponent::randomFloat(float lo, float hi) const
{
    float t = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    return lo + t * (hi - lo);
}

void SlimeAttackComponent::update(float dt)
{
    if (!getOwner() || dt <= 0.f)
        return;

    // Skip if dead.
    if (auto* hp = getOwner()->getComponent<HealthComponent>())
    {
        if (hp->isDead())
            return;
    }

    // --- Drive slime animations ---
    if (auto* anim = getOwner()->getComponent<AnimationComponent>())
    {
        if (m_jittering)
        {
            anim->play("jitter");
        }
        else if (auto* input = getOwner()->getComponent<InputComponent>())
        {
            const auto& inp = input->getInputState();
            if (inp.moveLeft)
                anim->play("move-left");
            else if (inp.moveRight)
                anim->play("move-right");
            else
                anim->play("idle");
        }
        else
        {
            anim->play("idle");
        }
    }

    // --- Attack cooldown ---
    if (!m_jittering)
    {
        m_cooldownTimer -= dt;
        if (m_cooldownTimer <= 0.f)
        {
            m_jittering   = true;
            m_jitterTimer = 0.f;

            // Pause the patrol AI during jitter.
            if (auto* ai = getOwner()->getComponent<EnemyAIComponent>())
                ai->setPaused(true);
        }
    }

    // --- Jitter phase ---
    if (m_jittering)
    {
        m_jitterTimer += dt;

        // Apply visual jitter offset to position.
        getOwner()->position.x += randomFloat(-JITTER_MAGNITUDE, JITTER_MAGNITUDE);
        getOwner()->position.y += randomFloat(-JITTER_MAGNITUDE, JITTER_MAGNITUDE);

        if (m_jitterTimer >= JITTER_DURATION)
        {
            m_jittering   = false;
            m_cooldownTimer = randomFloat(MIN_COOLDOWN, MAX_COOLDOWN);

            // Resume patrol.
            if (auto* ai = getOwner()->getComponent<EnemyAIComponent>())
                ai->setPaused(false);

            spawnParticles();
        }
    }

    // --- Update particles ---
    updateParticles(dt);
}

void SlimeAttackComponent::spawnParticles()
{
    if (!getOwner())
        return;

    const sf::Vector2f origin = getOwner()->position;

    for (int i = 0; i < PARTICLE_COUNT; ++i)
    {
        float angle = (2.f * PI * static_cast<float>(i)) / static_cast<float>(PARTICLE_COUNT)
                    + randomFloat(-0.3f, 0.3f);

        float speed = PARTICLE_SPEED * randomFloat(0.7f, 1.3f);

        Particle p;
        p.position    = origin;
        p.velocity    = { std::cos(angle) * speed, std::sin(angle) * speed };
        p.lifetime    = 0.f;
        p.maxLifetime = PARTICLE_LIFETIME * randomFloat(0.8f, 1.2f);
        p.hitPlayer   = false;

        m_particles.push_back(p);
    }
}

void SlimeAttackComponent::updateParticles(float dt)
{
    auto* playerPhys = m_player.getComponent<PhysicsComponent>();
    if (!playerPhys)
        return;

    const sf::Vector2f& pPos  = m_player.position;
    const sf::Vector2f& pSize = playerPhys->collisionSize;

    for (auto& p : m_particles)
    {
        p.position += p.velocity * dt;
        p.lifetime += dt;

        // Check collision with player (AABB with particle radius).
        if (!p.hitPlayer)
        {
            float dx = std::abs(p.position.x - pPos.x);
            float dy = std::abs(p.position.y - pPos.y);

            if (dx < (pSize.x * 0.5f + PARTICLE_RADIUS) &&
                dy < (pSize.y * 0.5f + PARTICLE_RADIUS))
            {
                p.hitPlayer = true;
                if (auto* playerHp = m_player.getComponent<HealthComponent>())
                    playerHp->takeDamage(m_damage);
            }
        }
    }

    // Remove expired particles.
    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(),
                       [](const Particle& p) { return p.lifetime >= p.maxLifetime; }),
        m_particles.end());
}

void SlimeAttackComponent::render(sf::RenderWindow& window)
{
    sf::CircleShape shape(PARTICLE_RADIUS);
    shape.setOrigin(PARTICLE_RADIUS, PARTICLE_RADIUS);

    for (const auto& p : m_particles)
    {
        if (p.hitPlayer)
            continue;

        float alpha = 1.f - (p.lifetime / p.maxLifetime);
        sf::Uint8 a = static_cast<sf::Uint8>(alpha * 220.f);

        shape.setPosition(p.position);
        shape.setFillColor(sf::Color(80, 255, 80, a));
        shape.setOutlineColor(sf::Color(30, 150, 30, a));
        shape.setOutlineThickness(1.f);
        window.draw(shape);
    }
}
