#pragma once

#include <string>

#include "../Math/Types.h"

struct TransitionZone
{
    std::string name;
    Rect        bounds;
    std::string targetMap;
    std::string targetSpawn;
};
