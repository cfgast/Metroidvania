#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

enum class LightType
{
    Point = 0,
    Spot  = 1
};

struct Light
{
    glm::vec2 position{0.f, 0.f};   // world-space XY
    glm::vec3 color{1.f, 1.f, 1.f}; // RGB
    float     intensity = 1.f;
    float     radius    = 200.f;     // max influence distance
    float     z         = 80.f;      // height above 2D plane
    glm::vec2 direction{0.f, 1.f};   // for spot lights (normalized)
    float     innerCone = 0.9659f;   // cos(15°) -- full intensity
    float     outerCone = 0.7071f;   // cos(45°) -- falloff to zero
    LightType type      = LightType::Point;
};
