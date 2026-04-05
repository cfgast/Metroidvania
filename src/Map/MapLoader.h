#pragma once

#include <string>

#include "Map.h"

class MapLoader
{
public:
    // Loads a Map from a JSON file. Throws std::runtime_error on failure.
    static Map loadFromFile(const std::string& filePath);
};
