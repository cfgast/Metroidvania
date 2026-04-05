#include "Map.h"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

void Map::addPlatform(const Platform& platform)
{
    m_platforms.push_back(platform);
}

sf::Vector2f Map::resolveCollision(sf::Vector2f position,
                                   sf::Vector2f size,
                                   sf::Vector2f& velocity,
                                   float dt,
                                   bool& grounded) const
{
    grounded = false;

    const float hw = size.x * 0.5f;
    const float hh = size.y * 0.5f;

    // --- Horizontal pass ---
    position.x += velocity.x * dt;
    {
        sf::FloatRect rect(position.x - hw, position.y - hh, size.x, size.y);
        for (const auto& platform : m_platforms)
        {
            if (!platform.bounds.intersects(rect))
                continue;

            if (velocity.x > 0.f)
                position.x = platform.bounds.left - hw;
            else if (velocity.x < 0.f)
                position.x = platform.bounds.left + platform.bounds.width + hw;

            velocity.x = 0.f;
            break;
        }
    }

    // --- Vertical pass ---
    position.y += velocity.y * dt;
    {
        sf::FloatRect rect(position.x - hw, position.y - hh, size.x, size.y);
        for (const auto& platform : m_platforms)
        {
            if (!platform.bounds.intersects(rect))
                continue;

            if (velocity.y > 0.f)
            {
                position.y = platform.bounds.top - hh;
                grounded   = true;
            }
            else if (velocity.y < 0.f)
            {
                position.y = platform.bounds.top + platform.bounds.height + hh;
            }

            velocity.y = 0.f;
            break;
        }
    }

    return position;
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
