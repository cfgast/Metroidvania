#pragma once

#include <string>

#include <glm/vec2.hpp>

#include "../Core/Ability.h"

struct AbilityPickupDefinition
{
    std::string id;                       // unique identifier for persistence
    Ability     ability;
    glm::vec2   position { 0.f, 0.f };
    glm::vec2   size     { 30.f, 30.f };
};
