#include "Map.h"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include <algorithm>

#include "../Physics/PhysXWorld.h"

void Map::addPlatform(const Platform& platform)
{
    m_platforms.push_back(platform);
}

sf::Vector2f Map::getNamedSpawn(const std::string& name) const
{
    auto it = m_namedSpawns.find(name);
    if (it != m_namedSpawns.end())
        return it->second;
    return m_spawnPoint;   // fallback to default spawn
}

const TransitionZone* Map::checkTransition(sf::Vector2f position,
                                           sf::Vector2f size) const
{
    sf::FloatRect playerRect(position.x - size.x * 0.5f,
                             position.y - size.y * 0.5f,
                             size.x, size.y);

    for (const auto& zone : m_transitionZones)
    {
        if (playerRect.intersects(zone.bounds))
            return &zone;
    }
    return nullptr;
}

const AbilityPickupDefinition* Map::checkAbilityPickup(sf::Vector2f position,
                                                        sf::Vector2f size) const
{
    sf::FloatRect playerRect(position.x - size.x * 0.5f,
                             position.y - size.y * 0.5f,
                             size.x, size.y);

    for (const auto& pickup : m_abilityPickups)
    {
        sf::FloatRect pickupRect(pickup.position.x - pickup.size.x * 0.5f,
                                 pickup.position.y - pickup.size.y * 0.5f,
                                 pickup.size.x, pickup.size.y);
        if (playerRect.intersects(pickupRect))
            return &pickup;
    }
    return nullptr;
}

void Map::removeAbilityPickup(const std::string& id)
{
    m_abilityPickups.erase(
        std::remove_if(m_abilityPickups.begin(), m_abilityPickups.end(),
                        [&](const AbilityPickupDefinition& p) { return p.id == id; }),
        m_abilityPickups.end());
}

void Map::registerPhysXStatics() const
{
    PhysXWorld& world = PhysXWorld::instance();
    world.clearStaticActors();

    for (const auto& platform : m_platforms)
    {
        const float hw = platform.bounds.width  * 0.5f;
        const float hh = platform.bounds.height * 0.5f;

        sf::Vector2f center{
            platform.bounds.left + hw,
            platform.bounds.top  + hh
        };

        world.createStaticBox(center, { hw, hh });
    }
}

void Map::render(sf::RenderWindow& window) const
{
    for (const auto& platform : m_platforms)
    {
        sf::RectangleShape shape(sf::Vector2f(platform.bounds.width, platform.bounds.height));
        shape.setPosition(platform.bounds.left, platform.bounds.top);
        shape.setFillColor(platform.color);
        window.draw(shape);
    }

    // Render ability pick-ups as bright coloured rectangles.
    for (const auto& pickup : m_abilityPickups)
    {
        sf::RectangleShape shape(pickup.size);
        shape.setOrigin(pickup.size.x * 0.5f, pickup.size.y * 0.5f);
        shape.setPosition(pickup.position);
        shape.setFillColor(sf::Color(255, 215, 0)); // gold
        shape.setOutlineColor(sf::Color::White);
        shape.setOutlineThickness(2.f);
        window.draw(shape);
    }
}
