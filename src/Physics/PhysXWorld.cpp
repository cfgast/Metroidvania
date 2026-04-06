#include "PhysXWorld.h"

PhysXWorld& PhysXWorld::instance()
{
    static PhysXWorld s_instance;
    return s_instance;
}

void PhysXWorld::init()
{
    m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION,
                                      m_allocator, m_errorCallback);

    m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation,
                                PxTolerancesScale());
    PxInitExtensions(*m_physics, nullptr);

    m_dispatcher = PxDefaultCpuDispatcherCreate(1);

    PxSceneDesc sceneDesc(m_physics->getTolerancesScale());
    // No built-in gravity; PhysicsComponent applies its own per-actor.
    sceneDesc.gravity          = PxVec3(0.f, 0.f, 0.f);
    sceneDesc.cpuDispatcher    = m_dispatcher;
    sceneDesc.filterShader     = PxDefaultSimulationFilterShader;
    m_scene = m_physics->createScene(sceneDesc);

    // Shared material: high friction, zero restitution (no bounce).
    m_material = m_physics->createMaterial(0.5f, 0.5f, 0.f);
}

void PhysXWorld::shutdown()
{
    if (m_scene)
    {
        clearStaticActors();
        m_scene->release();
        m_scene = nullptr;
    }
    PxCloseExtensions();
    if (m_dispatcher) { m_dispatcher->release(); m_dispatcher = nullptr; }
    if (m_physics)    { m_physics->release();    m_physics    = nullptr; }
    if (m_foundation) { m_foundation->release(); m_foundation = nullptr; }
}

void PhysXWorld::step(float dt)
{
    if (!m_scene || dt <= 0.f)
        return;
    m_scene->simulate(dt);
    m_scene->fetchResults(true);
}

PxRigidDynamic* PhysXWorld::createDynamicBox(sf::Vector2f position,
                                             sf::Vector2f halfExtents)
{
    PxTransform pose(PxVec3(position.x, position.y, 0.f));
    PxRigidDynamic* actor = m_physics->createRigidDynamic(pose);

    PxBoxGeometry box(halfExtents.x, halfExtents.y, k_zHalf);
    PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor, box,
                                                           *m_material);
    shape->setContactOffset(2.f);
    shape->setRestOffset(0.f);

    // Lock Z translation and all rotations for 2-D gameplay.
    actor->setRigidDynamicLockFlags(
        PxRigidDynamicLockFlag::eLOCK_LINEAR_Z  |
        PxRigidDynamicLockFlag::eLOCK_ANGULAR_X |
        PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y |
        PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z);

    // Disable built-in gravity; we apply custom gravity each frame.
    actor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);

    PxRigidBodyExt::updateMassAndInertia(*actor, 1.f);

    // Enable CCD for fast-moving jumps / falls.
    actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);

    m_scene->addActor(*actor);
    return actor;
}

PxRigidStatic* PhysXWorld::createStaticBox(sf::Vector2f position,
                                           sf::Vector2f halfExtents)
{
    PxTransform pose(PxVec3(position.x, position.y, 0.f));
    PxRigidStatic* actor = m_physics->createRigidStatic(pose);

    PxBoxGeometry box(halfExtents.x, halfExtents.y, k_zHalf);
    PxRigidActorExt::createExclusiveShape(*actor, box, *m_material);

    m_scene->addActor(*actor);
    m_staticActors.push_back(actor);
    return actor;
}

void PhysXWorld::clearStaticActors()
{
    for (auto* actor : m_staticActors)
    {
        m_scene->removeActor(*actor);
        actor->release();
    }
    m_staticActors.clear();
}
