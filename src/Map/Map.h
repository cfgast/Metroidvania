#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <glm/vec2.hpp>

#include "../Math/Types.h"
#include "Platform.h"
#include "EnemyDefinition.h"
#include "TransitionZone.h"
#include "AbilityPickupDefinition.h"
#include "LightDefinition.h"

class Renderer;

class Map
{
public:
    void addPlatform(const Platform& platform);
    const std::vector<Platform>& getPlatforms() const { return m_platforms; }

    glm::vec2 getSpawnPoint() const              { return m_spawnPoint; }
    void      setSpawnPoint(glm::vec2 point)     { m_spawnPoint = point; }

    Rect getBounds() const                       { return m_bounds; }
    void setBounds(Rect bounds)                  { m_bounds = bounds; }

    void addEnemyDefinition(const EnemyDefinition& def) { m_enemyDefinitions.push_back(def); }
    const std::vector<EnemyDefinition>& getEnemyDefinitions() const { return m_enemyDefinitions; }

    // Named spawn points (e.g. "from_world_02").
    void      addNamedSpawn(const std::string& name, glm::vec2 pos) { m_namedSpawns[name] = pos; }
    glm::vec2 getNamedSpawn(const std::string& name) const;

    // Transition zones.
    void addTransitionZone(const TransitionZone& zone) { m_transitionZones.push_back(zone); }
    const std::vector<TransitionZone>& getTransitionZones() const { return m_transitionZones; }

    // Returns pointer to the TransitionZone the rectangle overlaps, or nullptr.
    const TransitionZone* checkTransition(glm::vec2 position, glm::vec2 size) const;

    // Lights.
    void addLight(const LightDefinition& def) { m_lights.push_back(def); }
    const std::vector<LightDefinition>& getLights() const { return m_lights; }

    // Ability pick-ups.
    void addAbilityPickup(const AbilityPickupDefinition& def) { m_abilityPickups.push_back(def); }
    const std::vector<AbilityPickupDefinition>& getAbilityPickups() const { return m_abilityPickups; }

    // Returns pointer to the first overlapping ability pick-up, or nullptr.
    const AbilityPickupDefinition* checkAbilityPickup(glm::vec2 position, glm::vec2 size) const;

    // Remove a consumed pick-up by its unique id.
    void removeAbilityPickup(const std::string& id);

    // Register every platform as a PhysX static rigid body.
    void registerPhysXStatics() const;

    void render(Renderer& renderer) const;

private:
    std::vector<Platform>                m_platforms;
    std::vector<EnemyDefinition>         m_enemyDefinitions;
    std::vector<TransitionZone>          m_transitionZones;
    std::vector<AbilityPickupDefinition> m_abilityPickups;
    std::vector<LightDefinition>         m_lights;
    std::unordered_map<std::string, glm::vec2> m_namedSpawns;
    glm::vec2                    m_spawnPoint { 0.f, 0.f };
    Rect                         m_bounds     { 0.f, 0.f, 3200.f, 1200.f };
};
