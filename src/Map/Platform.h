#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>

struct Platform
{
    sf::FloatRect bounds;
    sf::Color     color = sf::Color(100, 100, 100);
};
