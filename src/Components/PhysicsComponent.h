#pragma once

#include <SFML/System/Vector2.hpp>

#include "../Core/Component.h"

namespace physx { class PxRigidDynamic; }

class Map;
struct InputState;
struct PlayerState;

class PhysicsComponent : public Component
{
public:
    PhysicsComponent(Map& map, sf::Vector2f collisionSize, float speed = 300.f);
    ~PhysicsComponent() override;

    void update(float dt) override;

    // Instantly move the owning GameObject and its PhysX actor to the given
    // position, resetting velocity and air-jump state.  Use this for map
    // transitions and respawns so the PhysX actor stays in sync.
    void teleport(sf::Vector2f position);

    bool isGrounded() const { return m_grounded; }
    bool isWallSliding() const { return m_wallSliding; }
    bool isDashing() const { return m_dashing; }
    // -1 = touching left wall, +1 = touching right wall, 0 = none
    int  wallDirection() const { return m_wallDirection; }
    bool facingRight() const { return m_facingRight; }

    void setPlayerState(PlayerState* state) { m_playerState = state; }

    float        speed             = 300.f;
    float        gravity           = 980.f;
    float        jumpForce         = -520.f;
    float        fallMultiplier    = 2.5f;
    float        lowJumpMultiplier = 2.0f;
    float        wallSlideSpeed    = 60.f;
    float        dashSpeed         = 1000.f;
    float        dashDuration      = 0.15f;
    float        dashCooldown      = 0.5f;
    sf::Vector2f velocity          { 0.f, 0.f };
    sf::Vector2f collisionSize;

private:
    bool checkGrounded() const;
    bool checkWallLeft() const;
    bool checkWallRight() const;

    Map& m_map;
    physx::PxRigidDynamic* m_actor = nullptr;
    PlayerState*           m_playerState  = nullptr;
    bool m_grounded        = false;
    bool m_wallSliding     = false;
    bool m_wasWallSliding  = false;
    int  m_wallDirection   = 0;
    bool m_jumpWasDown     = false;
    bool m_dashWasDown     = false;
    bool m_dashing         = false;
    bool m_facingRight     = true;
    float m_dashTimer      = 0.f;
    float m_dashCooldownTimer = 0.f;
    int  m_airJumpsUsed    = 0;
};
