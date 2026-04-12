#pragma once

#include <string>
#include "../Rendering/Light.h"

struct LightDefinition
{
    std::string name;
    LightType   type = LightType::Point;
    float x = 0.f, y = 0.f, z = 80.f;
    float r = 1.f, g = 1.f, b = 1.f;
    float intensity = 1.f;
    float radius = 200.f;
    // Spot-only:
    float directionX = 0.f, directionY = 1.f;
    float innerConeAngle = 30.f;  // degrees, converted to cos at load
    float outerConeAngle = 45.f;

    // Convert to a renderable Light struct.
    Light toLight() const
    {
        Light l;
        l.position  = {x, y};
        l.color     = {r, g, b};
        l.intensity = intensity;
        l.radius    = radius;
        l.z         = z;
        l.type      = type;
        l.direction = {directionX, directionY};
        l.innerCone = std::cos(innerConeAngle * 3.14159265f / 180.f);
        l.outerCone = std::cos(outerConeAngle * 3.14159265f / 180.f);
        return l;
    }
};
