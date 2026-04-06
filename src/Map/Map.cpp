#include "Map.h"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include "../Physics/PhysXWorld.h"

void Map::addPlatform(const Platform& platform)
{
    m_platforms.push_back(platform);
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
}
