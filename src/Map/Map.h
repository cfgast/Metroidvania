#pragma once

#include <vector>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include "Platform.h"
#include "EnemyDefinition.h"

namespace sf { class RenderWindow; }

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

    // Register every platform as a PhysX static rigid body.
    void registerPhysXStatics() const;

    void render(sf::RenderWindow& window) const;

private:
    std::vector<Platform>        m_platforms;
    std::vector<EnemyDefinition> m_enemyDefinitions;
    sf::Vector2f                 m_spawnPoint { 0.f, 0.f };
    sf::FloatRect                m_bounds     { 0.f, 0.f, 3200.f, 1200.f };
};
