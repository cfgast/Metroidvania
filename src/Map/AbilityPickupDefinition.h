#pragma once

#include <string>

#include <SFML/System/Vector2.hpp>

#include "../Core/Ability.h"

struct AbilityPickupDefinition
{
    std::string  id;                       // unique identifier for persistence
    Ability      ability;
    sf::Vector2f position;
    sf::Vector2f size { 30.f, 30.f };
};
