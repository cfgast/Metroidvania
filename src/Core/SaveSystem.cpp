#include "SaveSystem.h"

#include <fstream>
#include <iostream>
#include <filesystem>

#include <nlohmann/json.hpp>

#include "Ability.h"

std::string SaveSystem::slotPath(int slot)
{
    return "saves/save_" + std::to_string(slot) + ".json";
}

bool SaveSystem::save(int slot, const SaveData& data)
{
    namespace fs = std::filesystem;
    try
    {
        fs::create_directories("saves");
    }
    catch (...) {}

    nlohmann::json j;
    j["playerPosition"] = { {"x", data.playerPosition.x}, {"y", data.playerPosition.y} };
    j["currentMapFile"]  = data.currentMapFile;
    j["currentHp"]       = data.currentHp;
    j["maxHp"]           = data.maxHp;

    // Abilities
    nlohmann::json abilities = nlohmann::json::array();
    for (auto a : data.playerState.unlockedAbilities)
        abilities.push_back(abilityToString(a));
    j["unlockedAbilities"] = abilities;

    // Consumed pickups
    nlohmann::json pickups = nlohmann::json::array();
    for (const auto& id : data.playerState.consumedPickups)
        pickups.push_back(id);
    j["consumedPickups"] = pickups;

    // XP and level
    j["xp"]    = data.playerState.xp;
    j["level"] = data.playerState.level;

    std::ofstream file(slotPath(slot));
    if (!file.is_open())
    {
        std::cerr << "SaveSystem: cannot write to slot " << slot << '\n';
        return false;
    }
    file << j.dump(2);
    return true;
}

bool SaveSystem::load(int slot, SaveData& outData)
{
    std::ifstream file(slotPath(slot));
    if (!file.is_open())
        return false;

    nlohmann::json j;
    try
    {
        file >> j;
    }
    catch (const nlohmann::json::parse_error& e)
    {
        std::cerr << "SaveSystem: parse error in slot " << slot << ": " << e.what() << '\n';
        return false;
    }

    const auto& pos     = j.at("playerPosition");
    outData.playerPosition = { pos.at("x").get<float>(), pos.at("y").get<float>() };
    outData.currentMapFile = j.at("currentMapFile").get<std::string>();
    outData.currentHp      = j.value("currentHp", 100.f);
    outData.maxHp          = j.value("maxHp", 100.f);

    outData.playerState.unlockedAbilities.clear();
    if (j.contains("unlockedAbilities"))
    {
        for (const auto& a : j["unlockedAbilities"])
            outData.playerState.unlockedAbilities.insert(abilityFromString(a.get<std::string>()));
    }

    outData.playerState.consumedPickups.clear();
    if (j.contains("consumedPickups"))
    {
        for (const auto& p : j["consumedPickups"])
            outData.playerState.consumedPickups.insert(p.get<std::string>());
    }

    // XP and level (defaults for backward compatibility with old saves)
    outData.playerState.xp    = j.value("xp",    static_cast<uint32_t>(0));
    outData.playerState.level = j.value("level",  static_cast<uint32_t>(1));

    return true;
}

bool SaveSystem::slotExists(int slot)
{
    return std::filesystem::exists(slotPath(slot));
}

bool SaveSystem::deleteSlot(int slot)
{
    std::error_code ec;
    return std::filesystem::remove(slotPath(slot), ec);
}

std::vector<SaveSlotInfo> SaveSystem::querySlotsInfo()
{
    std::vector<SaveSlotInfo> infos;
    for (int i = 1; i <= MAX_SLOTS; ++i)
    {
        SaveSlotInfo info;
        info.slot   = i;
        info.exists = slotExists(i);
        if (info.exists)
        {
            SaveData tmp;
            if (load(i, tmp))
            {
                info.mapFile = tmp.currentMapFile;
                info.hp      = tmp.currentHp;
            }
        }
        infos.push_back(info);
    }
    return infos;
}
