#pragma once

#include <glm/vec2.hpp>

struct EnemyDefinition
{
    glm::vec2 position { 0.f, 0.f };
    glm::vec2 waypointA { 0.f, 0.f };
    glm::vec2 waypointB { 0.f, 0.f };
    float     speed  = 100.f;
    float     damage = 10.f;
    float     hp     = 50.f;
    glm::vec2 size   { 40.f, 40.f };
};
