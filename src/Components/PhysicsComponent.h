#pragma once

#include <SFML/System/Vector2.hpp>

#include "../Core/Component.h"

namespace physx { class PxRigidDynamic; }

class Map;

class PhysicsComponent : public Component
{
public:
    PhysicsComponent(Map& map, sf::Vector2f collisionSize, float speed = 300.f);
    ~PhysicsComponent() override;

    void update(float dt) override;

    bool isGrounded() const { return m_grounded; }

    float        speed             = 300.f;
    float        gravity           = 980.f;
    float        jumpForce         = -520.f;
    float        fallMultiplier    = 2.5f;
    float        lowJumpMultiplier = 2.0f;
    sf::Vector2f velocity          { 0.f, 0.f };
    sf::Vector2f collisionSize;

private:
    bool checkGrounded() const;

    Map& m_map;
    physx::PxRigidDynamic* m_actor = nullptr;
    bool m_grounded        = false;
    bool m_jumpKeyWasDown  = false;
};
