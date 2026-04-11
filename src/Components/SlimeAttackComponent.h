#pragma once

#include <vector>

#include <glm/vec2.hpp>

#include "../Core/Component.h"

class GameObject;
class Renderer;

class SlimeAttackComponent : public Component
{
public:
    SlimeAttackComponent(GameObject& player, float damage = 5.f);

    void update(float dt) override;
    void render(Renderer& renderer) override;

    bool isJittering() const { return m_jittering; }

private:
    struct Particle
    {
        glm::vec2 position  { 0.f, 0.f };
        glm::vec2 velocity  { 0.f, 0.f };
        float     lifetime    = 0.f;
        float     maxLifetime = 0.f;
        bool      hitPlayer   = false;
    };

    void spawnParticles();
    void updateParticles(float dt);
    float randomFloat(float lo, float hi) const;

    GameObject& m_player;
    float       m_damage;

    // Attack timing
    float m_cooldownTimer = 0.f;

    // Jitter state
    bool  m_jittering     = false;
    float m_jitterTimer   = 0.f;

    // Particles
    std::vector<Particle> m_particles;

    static constexpr float MIN_COOLDOWN       = 4.f;
    static constexpr float MAX_COOLDOWN       = 8.f;
    static constexpr float JITTER_DURATION     = 0.4f;
    static constexpr float JITTER_MAGNITUDE    = 3.f;
    static constexpr int   PARTICLE_COUNT      = 8;
    static constexpr float PARTICLE_SPEED      = 180.f;
    static constexpr float PARTICLE_LIFETIME   = 1.2f;
    static constexpr float PARTICLE_RADIUS     = 4.f;
};
