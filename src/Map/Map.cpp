#include "Map.h"

#include "../Rendering/Renderer.h"

#include <algorithm>

#include "../Physics/PhysXWorld.h"

void Map::addPlatform(const Platform& platform)
{
    m_platforms.push_back(platform);
}

glm::vec2 Map::getNamedSpawn(const std::string& name) const
{
    auto it = m_namedSpawns.find(name);
    if (it != m_namedSpawns.end())
        return it->second;
    return m_spawnPoint;   // fallback to default spawn
}

const TransitionZone* Map::checkTransition(glm::vec2 position,
                                           glm::vec2 size) const
{
    // Require the player's center to be inside the zone, meaning at least
    // half the sprite has crossed into the zone before the transition fires.
    for (const auto& zone : m_transitionZones)
    {
        if (zone.bounds.contains(position))
            return &zone;
    }
    return nullptr;
}

const AbilityPickupDefinition* Map::checkAbilityPickup(glm::vec2 position,
                                                        glm::vec2 size) const
{
    Rect playerRect(position.x - size.x * 0.5f,
                    position.y - size.y * 0.5f,
                    size.x, size.y);

    for (const auto& pickup : m_abilityPickups)
    {
        Rect pickupRect(pickup.position.x - pickup.size.x * 0.5f,
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

        glm::vec2 center{
            platform.bounds.x + hw,
            platform.bounds.y + hh
        };

        world.createStaticBox(center, { hw, hh });
    }
}

void Map::render(Renderer& renderer) const
{
    for (const auto& platform : m_platforms)
    {
        renderer.drawRect(platform.bounds.x, platform.bounds.y,
                          platform.bounds.width, platform.bounds.height,
                          platform.color.rf(),
                          platform.color.gf(),
                          platform.color.bf(),
                          platform.color.af());
    }

    // Render ability pick-ups as bright coloured rectangles.
    for (const auto& pickup : m_abilityPickups)
    {
        float px = pickup.position.x - pickup.size.x * 0.5f;
        float py = pickup.position.y - pickup.size.y * 0.5f;
        renderer.drawRectOutlined(px, py, pickup.size.x, pickup.size.y,
                                  1.f, 215.f/255.f, 0.f, 1.f,   // gold fill
                                  1.f, 1.f, 1.f, 1.f,            // white outline
                                  2.f);
    }
}
