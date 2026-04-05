# Physics System

## Overview

`PhysicsComponent` drives a `GameObject` through the game world. It replaces `MovementComponent` for any object that needs gravity, platform collision, and jumping.

## Component: PhysicsComponent (src/Components/PhysicsComponent.h / .cpp)

Inherits from `Component`. Should be added to a `GameObject` **after** `RenderComponent` so that the render shape is synced to the already-resolved position each frame.

| Field              | Default | Purpose                                              |
|--------------------|---------|------------------------------------------------------|
| `speed`            | 300     | Horizontal movement speed (units/sec)                |
| `gravity`          | 980     | Base downward acceleration (units/sec²)              |
| `jumpForce`        | -520    | Upward velocity impulse on jump                      |
| `fallMultiplier`   | 2.5     | Gravity scale while descending (snappy fall)         |
| `lowJumpMultiplier`| 2.0     | Gravity scale while rising with Space released (cut height) |
| `velocity`         | {0,0}   | Current velocity (readable externally)               |
| `collisionSize`    | —       | Width/height of the AABB collision box               |

### Per-frame logic (update)

```
1. Read A/D (or ←/→) → set velocity.x
2. If grounded AND Space just pressed → velocity.y = jumpForce
3. Apply gravity every frame (always — see Grounded detection note):
     - velocity.y > 0  (descending)          → gravity × fallMultiplier
     - velocity.y < 0  (rising, key released) → gravity × lowJumpMultiplier
     - otherwise (rising, key held)           → gravity × 1
4. map.resolveCollision(position, size, velocity, dt, grounded)
5. If position.y > map death zone → teleport to spawn, zero velocity
```

### Grounded detection

`m_grounded` is set to `true` only when `resolveCollision` detects a downward collision in the **vertical pass**. Gravity is applied **every frame unconditionally** — this is intentional. When standing on a platform, gravity pushes the player into it by a tiny amount each frame; the collision resolution immediately snaps them back and sets `grounded = true`. This keeps the flag reliable at 60 Hz. Gating gravity on `!m_grounded` causes the flag to flicker every other frame (no movement → no overlap → grounded=false), which makes jump detection unreliable when tapping quickly or while moving.

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
