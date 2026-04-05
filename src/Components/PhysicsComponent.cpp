#include "PhysicsComponent.h"

#include <SFML/Window/Keyboard.hpp>

#include "../Core/GameObject.h"
#include "../Map/Map.h"

PhysicsComponent::PhysicsComponent(Map& map, sf::Vector2f collisionSize, float speed)
    : m_map(map), collisionSize(collisionSize), speed(speed)
{
}

void PhysicsComponent::update(float dt)
{
    // --- Horizontal input ---
    velocity.x = 0.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
        velocity.x -= speed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
        velocity.x += speed;

    // --- Jump (only on the frame Space is first pressed) ---
    const bool jumpKeyDown = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);
    if (m_grounded && jumpKeyDown && !m_jumpKeyWasDown)
        velocity.y = jumpForce;
    m_jumpKeyWasDown = jumpKeyDown;

    // --- Gravity (always applied so grounding detection is stable every frame)
    // Faster descent and a "cut" when Space is released while rising make the
    // arc feel like real acceleration rather than a smooth float.
    if (velocity.y > 0.f)
        velocity.y += gravity * fallMultiplier * dt;       // descending: snappy fall
    else if (velocity.y < 0.f && !jumpKeyDown)
        velocity.y += gravity * lowJumpMultiplier * dt;    // rising, key released: cut height
    else
        velocity.y += gravity * dt;                        // rising, key held: normal arc

    // --- Collision resolution ---
    bool grounded = false;
    getOwner()->position = m_map.resolveCollision(
        getOwner()->position, collisionSize, velocity, dt, grounded);
    m_grounded = grounded;

    // --- Fall-off / death zone: teleport to spawn ---
    const sf::FloatRect& bounds = m_map.getBounds();
    if (getOwner()->position.y > bounds.top + bounds.height)
    {
        getOwner()->position = m_map.getSpawnPoint();
        velocity             = { 0.f, 0.f };
        m_grounded           = false;
    }
}
