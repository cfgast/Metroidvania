#pragma once

#include <cstdint>
#include <set>
#include <string>

#include "Ability.h"

// Persistent player state that survives room transitions.
struct PlayerState
{
    std::set<Ability>     unlockedAbilities;
    std::set<std::string> consumedPickups;

    uint32_t xp    = 0;   // Current XP within the current level
    uint32_t level = 1;   // Current level (1-5, capped)

    static constexpr uint32_t MAX_LEVEL   = 5;
    static constexpr uint32_t XP_TO_LEVEL = 5;  // Kills per level (linear)

    bool hasAbility(Ability a) const       { return unlockedAbilities.count(a) > 0; }
    void unlockAbility(Ability a)          { unlockedAbilities.insert(a); }

    bool isPickupConsumed(const std::string& id) const { return consumedPickups.count(id) > 0; }
    void consumePickup(const std::string& id)          { consumedPickups.insert(id); }

    // Add XP; auto-levels when xp reaches XP_TO_LEVEL. Caps at MAX_LEVEL.
    void awardXP(uint32_t amount)
    {
        if (isMaxLevel()) return;
        xp += amount;
        while (xp >= XP_TO_LEVEL && level < MAX_LEVEL)
        {
            xp -= XP_TO_LEVEL;
            ++level;
        }
        if (isMaxLevel())
            xp = XP_TO_LEVEL;  // Full bar at max level
    }

    uint32_t getXPToLevel() const { return XP_TO_LEVEL; }
    bool     isMaxLevel()   const { return level >= MAX_LEVEL; }
};
