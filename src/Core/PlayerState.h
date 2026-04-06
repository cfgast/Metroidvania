#pragma once

#include <set>
#include <string>

#include "Ability.h"

// Persistent player state that survives room transitions.
struct PlayerState
{
    std::set<Ability>     unlockedAbilities;
    std::set<std::string> consumedPickups;

    bool hasAbility(Ability a) const       { return unlockedAbilities.count(a) > 0; }
    void unlockAbility(Ability a)          { unlockedAbilities.insert(a); }

    bool isPickupConsumed(const std::string& id) const { return consumedPickups.count(id) > 0; }
    void consumePickup(const std::string& id)          { consumedPickups.insert(id); }
};
