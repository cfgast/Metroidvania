#include "MovementComponent.h"

#include "../Input/InputSystem.h"
#include "../Core/GameObject.h"

MovementComponent::MovementComponent(float speed)
    : speed(speed)
{
}

void MovementComponent::update(float dt)
{
    velocity = glm::vec2(0.f, 0.f);

    auto& input = InputSystem::current();

    if (input.isKeyPressed(KeyCode::W) || input.isKeyPressed(KeyCode::Up))
        velocity.y -= speed;
    if (input.isKeyPressed(KeyCode::S) || input.isKeyPressed(KeyCode::Down))
        velocity.y += speed;
    if (input.isKeyPressed(KeyCode::A) || input.isKeyPressed(KeyCode::Left))
        velocity.x -= speed;
    if (input.isKeyPressed(KeyCode::D) || input.isKeyPressed(KeyCode::Right))
        velocity.x += speed;

    getOwner()->position += velocity * dt;
}
