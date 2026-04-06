#include "PhysicsComponent.h"
#include "InputComponent.h"

#include <PxPhysicsAPI.h>

#include "../Core/GameObject.h"
#include "../Map/Map.h"
#include "../Physics/PhysXWorld.h"

using namespace physx;

PhysicsComponent::PhysicsComponent(Map& map, sf::Vector2f collisionSize, float speed)
    : m_map(map), collisionSize(collisionSize), speed(speed)
{
    // Deferred: the actor is created when an owner is assigned (see update).
}

PhysicsComponent::~PhysicsComponent()
{
    if (m_actor)
    {
        PhysXWorld::instance().getScene()->removeActor(*m_actor);
        m_actor->release();
        m_actor = nullptr;
    }
}

// Cast a thin probe box just below the player to detect ground.
bool PhysicsComponent::checkGrounded() const
{
    PxScene* scene = PhysXWorld::instance().getScene();
    if (!scene || !m_actor)
        return false;

    const PxTransform pose = m_actor->getGlobalPose();
    const float hw = collisionSize.x * 0.5f - 1.f;
    const float hh = collisionSize.y * 0.5f;

    // Thin probe sitting just below the feet.
    PxBoxGeometry probe(hw, 1.f, PhysXWorld::instance().k_zHalf);
    PxTransform probePose(PxVec3(pose.p.x,
                                 pose.p.y + hh + 1.f,
                                 0.f));

    PxOverlapBuffer hit;
    PxQueryFilterData filter;
    filter.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eANY_HIT;

    return scene->overlap(probe, probePose, hit, filter);
}

void PhysicsComponent::update(float dt)
{
    // Lazy-create the PhysX actor once we have an owner with a position.
    if (!m_actor && getOwner())
    {
        sf::Vector2f half{ collisionSize.x * 0.5f, collisionSize.y * 0.5f };
        m_actor = PhysXWorld::instance().createDynamicBox(
            getOwner()->position, half);
    }
    if (!m_actor)
        return;

    // --- Read input from sibling InputComponent ---
    InputState input;
    if (auto* ic = getOwner()->getComponent<InputComponent>())
        input = ic->getInputState();

    // --- Horizontal movement ---
    velocity.x = 0.f;
    if (input.moveLeft)
        velocity.x -= speed;
    if (input.moveRight)
        velocity.x += speed;

    // --- Jump (only on the frame jump is first pressed) ---
    if (m_grounded && input.jump && !m_jumpWasDown)
        velocity.y = jumpForce;
    m_jumpWasDown = input.jump;

    // --- Gravity with multipliers for snappy feel ---
    if (velocity.y > 0.f)
        velocity.y += gravity * fallMultiplier * dt;       // descending
    else if (velocity.y < 0.f && !input.jump)
        velocity.y += gravity * lowJumpMultiplier * dt;    // rising, key released
    else
        velocity.y += gravity * dt;                        // rising, key held

    // --- Push velocity into PhysX actor and step simulation ---
    m_actor->setLinearVelocity(PxVec3(velocity.x, velocity.y, 0.f));
    PhysXWorld::instance().step(dt);

    // --- Read back resolved position ---
    const PxTransform t = m_actor->getGlobalPose();
    getOwner()->position = { t.p.x, t.p.y };

    // --- Grounding & velocity bookkeeping ---
    m_grounded = checkGrounded();
    if (m_grounded && velocity.y > 0.f)
        velocity.y = 0.f;

    // Zero upward velocity when hitting a ceiling (overlap above head).
    if (velocity.y < 0.f)
    {
        const float hw = collisionSize.x * 0.5f - 1.f;
        PxBoxGeometry ceilProbe(hw, 1.f, PhysXWorld::instance().k_zHalf);
        PxTransform ceilPose(PxVec3(t.p.x,
                                    t.p.y - collisionSize.y * 0.5f - 1.f,
                                    0.f));
        PxOverlapBuffer ceilHit;
        PxQueryFilterData filter;
        filter.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eANY_HIT;

        if (PhysXWorld::instance().getScene()->overlap(
                ceilProbe, ceilPose, ceilHit, filter))
            velocity.y = 0.f;
    }

    // --- Fall-off / death zone: teleport to spawn ---
    const sf::FloatRect& bounds = m_map.getBounds();
    if (getOwner()->position.y > bounds.top + bounds.height)
    {
        getOwner()->position = m_map.getSpawnPoint();
        velocity             = { 0.f, 0.f };
        m_grounded           = false;
        m_actor->setGlobalPose(PxTransform(
            PxVec3(getOwner()->position.x, getOwner()->position.y, 0.f)));
        m_actor->setLinearVelocity(PxVec3(0.f));
    }
}
