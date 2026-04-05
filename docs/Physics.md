# Physics System

## Overview

`PhysicsComponent` drives a `GameObject` through the game world. It replaces `MovementComponent` for any object that needs gravity, platform collision, and jumping.

## Component: PhysicsComponent (src/Components/PhysicsComponent.h / .cpp)

Inherits from `Component`. Should be added to a `GameObject` **after** `RenderComponent` so that the render shape is synced to the already-resolved position each frame.

| Field          | Default | Purpose                                  |
|----------------|---------|------------------------------------------|
| `speed`        | 300     | Horizontal movement speed (units/sec)    |
| `gravity`      | 980     | Downward acceleration (units/sec²)       |
| `jumpForce`    | -520    | Upward velocity impulse on jump          |
| `velocity`     | {0,0}   | Current velocity (readable externally)   |
| `collisionSize`| —       | Width/height of the AABB collision box   |

### Per-frame logic (update)

```
1. Read A/D (or ←/→) → set velocity.x
2. If grounded AND Space just pressed → velocity.y = jumpForce
3. If not grounded → velocity.y += gravity * dt   (accumulate gravity)
4. map.resolveCollision(position, size, velocity, dt, grounded)
5. If position.y > map death zone → teleport to spawn, zero velocity
```

### Grounded detection

`m_grounded` is set to `true` only when `resolveCollision` detects a downward collision in the **vertical pass** (i.e., the object lands on a platform top this frame). It is cleared to `false` at the start of each resolution call. Jump is gated on `m_grounded`, preventing air-jumps.

### Jump edge-case

The `m_jumpKeyWasDown` flag tracks whether Space was held on the previous frame. Jump fires only on the frame the key transitions from up → down, preventing a held Space from re-jumping the instant the player lands.

### Spawn / fall-off safety

If `owner.position.y` exceeds the bottom of `map.getBounds()`, the object is teleported to `map.getSpawnPoint()` and velocity is zeroed. This prevents the player from falling infinitely if they walk off the edge of the map.

## Relationship to MovementComponent

`MovementComponent` (still present) is a simpler, physics-free mover suitable for objects that don't need gravity (UI cursors, floating items, enemies on rails, etc.). For any player-controlled or gravity-affected object, use `PhysicsComponent`.

## Future: Nvidia PhysX Integration

When PhysX is integrated, `PhysicsComponent` will be split:

- **PhysicsComponent** — owns a `PxRigidDynamic` actor, delegates gravity and collision to the PhysX scene
- **MovementComponent** — provides desired velocity intent to the PhysX character controller

The `Map` platforms will become static `PxRigidStatic` actors in the PhysX scene.
