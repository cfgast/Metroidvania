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

## Map File Format

Maps are stored as JSON files in the `maps/` directory. At build time CMake copies the `maps/` folder next to the executable, so files are loaded with relative paths.

### Schema

```json
{
  "name": "World 1 - Starting Area",
  "bounds":     { "x": -200.0, "y": -500.0, "width": 3600.0, "height": 1200.0 },
  "spawnPoint": { "x": 150.0,  "y": 475.0 },
  "platforms": [
    { "x": -200.0, "y": 500.0, "width": 3600.0, "height": 40.0, "r": 80, "g": 80, "b": 80 }
  ]
}
```

| Field        | Type   | Required | Description |
|--------------|--------|----------|-------------|
| `name`       | string | no       | Human-readable level name |
| `bounds`     | object | yes      | World extents; used for death zone + camera clamping |
| `spawnPoint` | object | yes      | Player start / respawn world position |
| `platforms`  | array  | yes      | List of platform objects |

Each platform object:

| Field    | Type  | Required | Description |
|----------|-------|----------|-------------|
| `x`,`y`  | float | yes      | Top-left world position |
| `width`  | float | yes      | Width in world units |
| `height` | float | yes      | Height in world units |
| `r`,`g`,`b` | int | no (default 100) | Fill colour channels |

### MapLoader (src/Map/MapLoader.h / .cpp)

Static utility class. Uses **nlohmann/json** (fetched automatically via CMake FetchContent).

```cpp
Map map = MapLoader::loadFromFile("maps/world_01.json");
```

Throws `std::runtime_error` if the file cannot be opened or the JSON is malformed.

