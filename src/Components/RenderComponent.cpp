#include "RenderComponent.h"

#include <SFML/Graphics/RenderWindow.hpp>

#include "../Core/GameObject.h"

RenderComponent::RenderComponent(sf::Vector2f size, sf::Color color)
{
    shape.setSize(size);
    shape.setFillColor(color);
    shape.setOrigin(size * 0.5f);
}

void RenderComponent::update(float dt)
{
    shape.setPosition(getOwner()->position);
}

void RenderComponent::render(sf::RenderWindow& window)
{
    window.draw(shape);
}
