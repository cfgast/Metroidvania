#include "MovementComponent.h"

#include <SFML/Window/Keyboard.hpp>

#include "../Core/GameObject.h"

MovementComponent::MovementComponent(float speed)
    : speed(speed)
{
}

void MovementComponent::update(float dt)
{
    velocity = sf::Vector2f(0.f, 0.f);

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
        velocity.y -= speed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
        velocity.y += speed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
        velocity.x -= speed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
        velocity.x += speed;

    getOwner()->position += velocity * dt;
}
