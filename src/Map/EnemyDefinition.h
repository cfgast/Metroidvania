#pragma once

#include <SFML/System/Vector2.hpp>

struct EnemyDefinition
{
    sf::Vector2f position;
    sf::Vector2f waypointA;
    sf::Vector2f waypointB;
    float        speed  = 100.f;
    float        damage = 10.f;
    float        hp     = 50.f;
    sf::Vector2f size   { 40.f, 40.f };
};
