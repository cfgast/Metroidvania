#pragma once

#include <vector>

#include <PxPhysicsAPI.h>

#include <SFML/System/Vector2.hpp>

using namespace physx;

// Singleton that owns the PhysX foundation, physics instance, scene and
// dispatcher.  Every physics-driven object in the game interacts with the
// world through this class.
class PhysXWorld
{
public:
    static PhysXWorld& instance();

    void init();
    void shutdown();

    // Advance the PhysX simulation by dt seconds.
    void step(float dt);

    // Create a dynamic box actor (for the player / moving objects).
    PxRigidDynamic* createDynamicBox(sf::Vector2f position,
                                     sf::Vector2f halfExtents);

    // Create a static box actor (for platforms).
    PxRigidStatic* createStaticBox(sf::Vector2f position,
                                   sf::Vector2f halfExtents);

    // Remove all static actors that were registered as platforms.
    void clearStaticActors();

    PxScene*   getScene()   const { return m_scene; }
    PxPhysics* getPhysics() const { return m_physics; }

    // Z half-extent used for all 2-D box shapes.
    static constexpr float k_zHalf = 10.f;

private:
    PhysXWorld() = default;
    ~PhysXWorld() = default;
    PhysXWorld(const PhysXWorld&) = delete;
    PhysXWorld& operator=(const PhysXWorld&) = delete;

    PxDefaultAllocator      m_allocator;
    PxDefaultErrorCallback   m_errorCallback;
    PxFoundation*            m_foundation  = nullptr;
    PxPhysics*               m_physics     = nullptr;
    PxDefaultCpuDispatcher*  m_dispatcher  = nullptr;
    PxScene*                 m_scene       = nullptr;
    PxMaterial*              m_material    = nullptr;

    // Track static actors so we can remove them when loading a new map.
    std::vector<PxRigidStatic*> m_staticActors;
};
