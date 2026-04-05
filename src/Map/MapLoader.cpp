#include "MapLoader.h"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

Map MapLoader::loadFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
        throw std::runtime_error("MapLoader: cannot open file: " + filePath);

    nlohmann::json j;
    try
    {
        file >> j;
    }
    catch (const nlohmann::json::parse_error& e)
    {
        throw std::runtime_error("MapLoader: JSON parse error in '" + filePath + "': " + e.what());
    }

    Map map;

    const auto& b = j.at("bounds");
    map.setBounds({
        b.at("x").get<float>(),
        b.at("y").get<float>(),
        b.at("width").get<float>(),
        b.at("height").get<float>()
    });

    const auto& s = j.at("spawnPoint");
    map.setSpawnPoint({
        s.at("x").get<float>(),
        s.at("y").get<float>()
    });

    for (const auto& p : j.at("platforms"))
    {
        Platform platform;
        platform.bounds = {
            p.at("x").get<float>(),
            p.at("y").get<float>(),
            p.at("width").get<float>(),
            p.at("height").get<float>()
        };
        platform.color = sf::Color(
            static_cast<sf::Uint8>(p.value("r", 100)),
            static_cast<sf::Uint8>(p.value("g", 100)),
            static_cast<sf::Uint8>(p.value("b", 100))
        );
        map.addPlatform(platform);
    }

    return map;
}
