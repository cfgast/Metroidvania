#include "PhysicsComponent.h"
#include "InputComponent.h"

#include <PxPhysicsAPI.h>

#include "../Core/GameObject.h"
#include "../Core/PlayerState.h"
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
        PxScene* scene = PhysXWorld::instance().getScene();
        if (scene)
        {
            scene->removeActor(*m_actor);
            m_actor->release();
        }
        // If the scene is already null, PhysXWorld::shutdown() released
        // it (and all actors still in it), so we must not release again.
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

// Cast a thin probe box to the left of the player to detect a wall.
bool PhysicsComponent::checkWallLeft() const
{
    PxScene* scene = PhysXWorld::instance().getScene();
    if (!scene || !m_actor)
        return false;

    const PxTransform pose = m_actor->getGlobalPose();
    const float hw = collisionSize.x * 0.5f;
    const float hh = collisionSize.y * 0.5f - 2.f;

    PxBoxGeometry probe(1.f, hh, PhysXWorld::instance().k_zHalf);
    PxTransform probePose(PxVec3(pose.p.x - hw - 1.f,
                                 pose.p.y,
                                 0.f));

    PxOverlapBuffer hit;
    PxQueryFilterData filter;
    filter.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eANY_HIT;

    return scene->overlap(probe, probePose, hit, filter);
}

// Cast a thin probe box to the right of the player to detect a wall.
bool PhysicsComponent::checkWallRight() const
{
    PxScene* scene = PhysXWorld::instance().getScene();
    if (!scene || !m_actor)
        return false;

    const PxTransform pose = m_actor->getGlobalPose();
    const float hw = collisionSize.x * 0.5f;
    const float hh = collisionSize.y * 0.5f - 2.f;

    PxBoxGeometry probe(1.f, hh, PhysXWorld::instance().k_zHalf);
    PxTransform probePose(PxVec3(pose.p.x + hw + 1.f,
                                 pose.p.y,
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

    // --- Wall slide detection ---
    bool canWallSlide = m_playerState && m_playerState->hasAbility(Ability::WallSlide);
    bool touchingLeftWall  = checkWallLeft();
    bool touchingRightWall = checkWallRight();

    m_wasWallSliding = m_wallSliding;
    m_wallSliding = false;
    m_wallDirection = 0;

    if (canWallSlide && !m_grounded)
    {
        if (touchingLeftWall && input.moveLeft)
        {
            m_wallSliding = true;
            m_wallDirection = -1;
        }
        else if (touchingRightWall && input.moveRight)
        {
            m_wallSliding = true;
            m_wallDirection = 1;
        }
    }

    // Reset air jumps when wall slide begins
    if (m_wallSliding && !m_wasWallSliding)
        m_airJumpsUsed = 0;

    // --- Jump (only on the frame jump is first pressed) ---
    if (m_grounded && input.jump && !m_jumpWasDown)
    {
        velocity.y = jumpForce;
    }
    else if (m_wallSliding && input.jump && !m_jumpWasDown)
    {
        // Wall jump: push away from wall and upward
        velocity.y = jumpForce;
        velocity.x = speed * static_cast<float>(-m_wallDirection);
        m_wallSliding = false;
        m_wallDirection = 0;
    }
    else if (!m_grounded && !m_wallSliding && input.jump && !m_jumpWasDown
             && m_playerState && m_playerState->hasAbility(Ability::DoubleJump)
             && m_airJumpsUsed < 1)
    {
        velocity.y = jumpForce;
        ++m_airJumpsUsed;
    }
    m_jumpWasDown = input.jump;

    // --- Wall slide: cap downward velocity ---
    if (m_wallSliding && velocity.y > 0.f)
        velocity.y = wallSlideSpeed;

    // --- Gravity with multipliers for snappy feel ---
    if (m_wallSliding)
        velocity.y += gravity * 0.5f * dt;                 // reduced gravity on wall
    else if (velocity.y > 0.f)
        velocity.y += gravity * fallMultiplier * dt;       // descending
    else if (velocity.y < 0.f && !input.jump)
        velocity.y += gravity * lowJumpMultiplier * dt;    // rising, key released
    else
        velocity.y += gravity * dt;                        // rising, key held

    // Enforce wall slide speed cap after gravity
    if (m_wallSliding && velocity.y > wallSlideSpeed)
        velocity.y = wallSlideSpeed;

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
    if (m_grounded)
        m_airJumpsUsed = 0;

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

    // --- Fall-off / death zone: respawn ---
    const sf::FloatRect& bounds = m_map.getBounds();
    if (getOwner()->position.y > bounds.top + bounds.height)
    {
        teleport(m_map.getSpawnPoint());
    }
}

void PhysicsComponent::teleport(sf::Vector2f position)
{
    if (getOwner())
        getOwner()->position = position;
    velocity       = { 0.f, 0.f };
    m_grounded     = false;
    m_wallSliding  = false;
    m_wasWallSliding = false;
    m_wallDirection = 0;
    m_airJumpsUsed = 0;
    if (m_actor)
    {
        m_actor->setGlobalPose(PxTransform(PxVec3(position.x, position.y, 0.f)));
        m_actor->setLinearVelocity(PxVec3(0.f, 0.f, 0.f));
    }
}
