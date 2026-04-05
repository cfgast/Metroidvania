#pragma once

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

#include "../Core/Component.h"

class RenderComponent : public Component
{
public:
    RenderComponent(sf::Vector2f size, sf::Color color = sf::Color::White);

    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

    sf::RectangleShape shape;
};
