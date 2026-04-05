# Map System

## Overview

The map system defines the game world geometry. It is the source of truth for walkable surfaces, world bounds, and the player spawn point. All physics queries (collision, fall detection) go through `Map`.

## Architecture

```
Map
 ├── platforms[]     (list of Platform — axis-aligned rectangles)
 ├── spawnPoint      (world position where the player starts / respawns)
 └── bounds          (world extents; falling below triggers respawn)
```

### Platform (src/Map/Platform.h)

A lightweight struct.

| Field    | Type            | Purpose                          |
|----------|-----------------|----------------------------------|
| `bounds` | `sf::FloatRect` | World-space AABB of the surface  |
| `color`  | `sf::Color`     | Debug / placeholder render color |

### Map (src/Map/Map.h / .cpp)

| Method | Purpose |
|--------|---------|
| `addPlatform(platform)` | Register a platform |
| `getPlatforms()` | Read-only access to platform list |
| `getSpawnPoint()` / `setSpawnPoint()` | Player start / respawn location |
| `getBounds()` / `setBounds()` | World extents (used for death zone + camera clamping) |
| `resolveCollision(pos, size, vel, dt, grounded)` | Moves `pos` one axis at a time, resolves AABB overlaps, and returns the corrected position |
| `render(window)` | Draws all platforms as coloured rectangles |

## Collision Resolution

`resolveCollision` uses the standard two-pass axis-separation approach:

1. **Horizontal pass** — apply `velocity.x * dt`, then check every platform. If overlapping, push the object to the nearest horizontal edge and zero `velocity.x`.
2. **Vertical pass** — apply `velocity.y * dt`, then check every platform. If overlapping and falling → snap to platform top (`grounded = true`). If rising → snap to platform bottom. Zero `velocity.y` in either case.

This prevents tunnelling at normal game speeds and avoids corner-snagging artefacts.

## Camera

`main.cpp` uses an `sf::View` centred on the player each frame, clamped to `map.getBounds()` so the camera never shows outside the world.

## Starting Map Layout

The current starting map (`buildStartingMap()` in `main.cpp`) is 3200 units wide:

| Platform         | x     | y   | w    |
|------------------|-------|-----|------|
| Ground           | -200  | 500 | 3600 |
| Ledge 1          | 250   | 380 | 200  |
| Ledge 2          | 550   | 300 | 150  |
| Ledge 3          | 800   | 220 | 180  |
| Ledge 4          | 1050  | 340 | 200  |
| Ledge 5          | 1350  | 260 | 150  |
| Ledge 6          | 1600  | 180 | 200  |
| Ledge 7          | 1900  | 340 | 180  |
| Ledge 8          | 2150  | 260 | 150  |
| Ledge 9          | 2400  | 180 | 200  |

Spawn point: **(150, 475)** — ground surface at y = 500, player half-height = 25.

## Adding New Maps

1. Instantiate a `Map`
2. Call `addPlatform` with each `Platform { bounds, color }`
3. Set `spawnPoint` and `bounds`
4. Pass the map to `PhysicsComponent` and the camera logic
