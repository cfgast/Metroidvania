#pragma once

#include <vector>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include "Platform.h"

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

    // Moves position one axis at a time, resolves AABB overlaps with platforms.
    // Writes corrected velocity back (zeroed on collision axis).
    // Sets grounded=true if the object landed on a platform top this frame.
    sf::Vector2f resolveCollision(sf::Vector2f position,
                                  sf::Vector2f size,
                                  sf::Vector2f& velocity,
                                  float dt,
                                  bool& grounded) const;

    void render(sf::RenderWindow& window) const;

private:
    std::vector<Platform> m_platforms;
    sf::Vector2f          m_spawnPoint { 0.f, 0.f };
    sf::FloatRect         m_bounds     { 0.f, 0.f, 3200.f, 1200.f };
};
