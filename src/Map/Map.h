#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include "Platform.h"
#include "EnemyDefinition.h"
#include "TransitionZone.h"
#include "AbilityPickupDefinition.h"

class Renderer;

class Map
{
public:
    void addPlatform(const Platform& platform);
    const std::vector<Platform>& getPlatforms() const { return m_platforms; }

    sf::Vector2f getSpawnPoint() const              { return m_spawnPoint; }
    void         setSpawnPoint(sf::Vector2f point)  { m_spawnPoint = point; }

    sf::FloatRect getBounds() const                 { return m_bounds; }
    void          setBounds(sf::FloatRect bounds)   { m_bounds = bounds; }

    void addEnemyDefinition(const EnemyDefinition& def) { m_enemyDefinitions.push_back(def); }
    const std::vector<EnemyDefinition>& getEnemyDefinitions() const { return m_enemyDefinitions; }

    // Named spawn points (e.g. "from_world_02").
    void         addNamedSpawn(const std::string& name, sf::Vector2f pos) { m_namedSpawns[name] = pos; }
    sf::Vector2f getNamedSpawn(const std::string& name) const;

    // Transition zones.
    void addTransitionZone(const TransitionZone& zone) { m_transitionZones.push_back(zone); }
    const std::vector<TransitionZone>& getTransitionZones() const { return m_transitionZones; }

    // Returns pointer to the TransitionZone the rectangle overlaps, or nullptr.
    const TransitionZone* checkTransition(sf::Vector2f position, sf::Vector2f size) const;

    // Ability pick-ups.
    void addAbilityPickup(const AbilityPickupDefinition& def) { m_abilityPickups.push_back(def); }
    const std::vector<AbilityPickupDefinition>& getAbilityPickups() const { return m_abilityPickups; }

    // Returns pointer to the first overlapping ability pick-up, or nullptr.
    const AbilityPickupDefinition* checkAbilityPickup(sf::Vector2f position, sf::Vector2f size) const;

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
    std::unordered_map<std::string, sf::Vector2f> m_namedSpawns;
    sf::Vector2f                   m_spawnPoint { 0.f, 0.f };
    sf::FloatRect                  m_bounds     { 0.f, 0.f, 3200.f, 1200.f };
};
