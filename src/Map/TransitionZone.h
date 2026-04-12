#pragma once

#include <string>

#include "../Math/Types.h"

struct TransitionZone
{
    std::string name;
    Rect        bounds;
    std::string targetMap;
    std::string targetSpawn;

    std::string edgeAxis;       // "vertical" or "horizontal" (empty = legacy mode)
    float       targetBaseX = 0;// X origin of matching zone in target map
    float       targetBaseY = 0;// Y origin of matching zone in target map
};
