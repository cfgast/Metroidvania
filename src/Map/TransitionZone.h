#pragma once

#include <string>

#include <SFML/Graphics/Rect.hpp>

struct TransitionZone
{
    std::string   name;
    sf::FloatRect bounds;
    std::string   targetMap;
    std::string   targetSpawn;
};
