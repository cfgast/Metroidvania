#pragma once

#include <SFML/System/Vector2.hpp>

#include "../Core/Component.h"

class Map;

class PhysicsComponent : public Component
{
public:
    PhysicsComponent(Map& map, sf::Vector2f collisionSize, float speed = 300.f);

    void update(float dt) override;

    bool isGrounded() const { return m_grounded; }

    float        speed             = 300.f;
    float        gravity           = 980.f;
    float        jumpForce         = -520.f;
    // Multipliers applied to gravity on the descent and when Space is released
    // mid-rise. Higher values = snappier arc, quicker fall.
    float        fallMultiplier    = 2.5f;
    float        lowJumpMultiplier = 2.0f;
    sf::Vector2f velocity          { 0.f, 0.f };
    sf::Vector2f collisionSize;

private:
    Map& m_map;
    bool m_grounded        = false;
    bool m_jumpKeyWasDown  = false;
};
