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

    // Named spawn points (optional).
    if (j.contains("spawnPoints"))
    {
        for (auto& [name, val] : j["spawnPoints"].items())
        {
            map.addNamedSpawn(name, {
                val.at("x").get<float>(),
                val.at("y").get<float>()
            });
        }
    }

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

    if (j.contains("enemies"))
    {
        for (const auto& e : j["enemies"])
        {
            EnemyDefinition def;
            def.position = {
                e.at("x").get<float>(),
                e.at("y").get<float>()
            };

            const auto& wA = e.at("waypointA");
            def.waypointA = { wA.at("x").get<float>(), wA.at("y").get<float>() };

            const auto& wB = e.at("waypointB");
            def.waypointB = { wB.at("x").get<float>(), wB.at("y").get<float>() };

            def.speed  = e.value("speed",  100.f);
            def.damage = e.value("damage", 10.f);
            def.hp     = e.value("hp",     50.f);
            def.size   = {
                e.value("width",  40.f),
                e.value("height", 40.f)
            };

            map.addEnemyDefinition(def);
        }
    }

    // Transition zones (optional).
    if (j.contains("transitions"))
    {
        for (const auto& t : j["transitions"])
        {
            TransitionZone zone;
            zone.name        = t.value("name", "");
            zone.bounds      = {
                t.at("x").get<float>(),
                t.at("y").get<float>(),
                t.at("width").get<float>(),
                t.at("height").get<float>()
            };
            zone.targetMap   = t.at("targetMap").get<std::string>();
            zone.targetSpawn = t.value("targetSpawn", "default");
            map.addTransitionZone(zone);
        }
    }

    // Ability pick-ups (optional).
    if (j.contains("abilityPickups"))
    {
        for (const auto& a : j["abilityPickups"])
        {
            AbilityPickupDefinition def;
            def.id       = a.at("id").get<std::string>();
            def.ability  = abilityFromString(a.at("ability").get<std::string>());
            def.position = {
                a.at("x").get<float>(),
                a.at("y").get<float>()
            };
            def.size = {
                a.value("width",  30.f),
                a.value("height", 30.f)
            };
            map.addAbilityPickup(def);
        }
    }

    return map;
}
