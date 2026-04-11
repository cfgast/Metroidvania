#pragma once

#include <string>
#include <vector>

#include <glm/vec2.hpp>

#include "PlayerState.h"

// Data written to / read from a save file.
struct SaveData
{
    glm::vec2     playerPosition  { 0.f, 0.f };
    std::string   currentMapFile;
    float         currentHp       = 100.f;
    float         maxHp           = 100.f;
    PlayerState   playerState;
};

// Describes one save slot for the selection screen.
struct SaveSlotInfo
{
    int         slot       = 0;
    bool        exists     = false;
    std::string mapFile;
    float       hp         = 0.f;
};

// Persistent save / load system.  Saves are stored as JSON in the "saves/"
// directory next to the executable.
class SaveSystem
{
public:
    static constexpr int MAX_SLOTS = 3;

    // Write a save file for the given slot (1-based).
    static bool save(int slot, const SaveData& data);

    // Load a save file for the given slot.  Returns false if the file does
    // not exist or is corrupt.
    static bool load(int slot, SaveData& outData);

    // Quick query: does a save file exist for this slot?
    static bool slotExists(int slot);

    // Delete the save file for a slot.
    static bool deleteSlot(int slot);

    // Gather brief info about every slot (for the selection screen).
    static std::vector<SaveSlotInfo> querySlotsInfo();

private:
    static std::string slotPath(int slot);
};
