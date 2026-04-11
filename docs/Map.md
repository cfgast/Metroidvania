# Map System

## Overview

The map system defines the game world geometry. It is the source of truth for walkable surfaces, world bounds, the player spawn point, transition zones, and named spawn points. All physics queries (collision, fall detection) go through `Map`.

## Architecture

```
Map
 ├── platforms[]          (list of Platform — axis-aligned rectangles)
 ├── spawnPoint           (world position where the player starts / respawns)
 ├── namedSpawns{}        (string→position map for transition targets)
 ├── transitionZones[]    (list of TransitionZone — trigger areas)
 ├── enemyDefinitions[]   (list of EnemyDefinition)
 └── bounds               (world extents; falling below triggers respawn)
```

### Platform (src/Map/Platform.h)

A lightweight struct.

| Field    | Type            | Purpose                          |
|----------|-----------------|----------------------------------|
| `bounds` | `Rect`  | World-space AABB of the surface  |
| `color`  | `Color` | Debug / placeholder render color |

### TransitionZone (src/Map/TransitionZone.h)

Describes a trigger area that moves the player to another map.

| Field         | Type            | Purpose                                     |
|---------------|-----------------|---------------------------------------------|
| `name`        | `std::string`   | Human-readable identifier                   |
| `bounds`      | `Rect`          | World-space AABB trigger region              |
| `targetMap`   | `std::string`   | Relative path to the destination map JSON    |
| `targetSpawn` | `std::string`   | Named spawn point in the destination map     |

### TransitionManager (src/Map/TransitionManager.h / .cpp)

Drives the fade-to-black room-transition effect.

| Method / Field | Purpose |
|----------------|---------|
| `setLoadCallback(cb)` | Register the function that loads a new map + repositions the player |
| `startTransition(targetMap, targetSpawn)` | Begin a fade-out → load → fade-in sequence |
| `update(dt)` | Advance the fade timer; returns `true` while active |
| `render(window)` | Draw the semi-transparent black overlay in screen space |
| `isActive()` | `true` while a transition is in progress |

State machine: **Idle → FadingOut → (load callback fires) → FadingIn → Idle**

### Map (src/Map/Map.h / .cpp)

| Method | Purpose |
|--------|---------|
| `addPlatform(platform)` | Register a platform |
| `getPlatforms()` | Read-only access to platform list |
| `getSpawnPoint()` / `setSpawnPoint()` | Player start / respawn location |
| `getBounds()` / `setBounds()` | World extents (used for death zone + camera clamping) |
| `addNamedSpawn(name, pos)` / `getNamedSpawn(name)` | Named spawn points for transitions (falls back to default spawn) |
| `addTransitionZone(zone)` / `getTransitionZones()` | Register / list transition zones |
| `checkTransition(pos, size)` | Returns the `TransitionZone*` the player overlaps, or `nullptr` |
| `registerPhysXStatics()` | Creates PhysX static bodies for all platforms |
| `render(window)` | Draws all platforms as coloured rectangles |

## Collision Resolution

Collision is handled by PhysX. Each platform is registered as a `PxRigidStatic` box actor and the player / enemies are `PxRigidDynamic` actors. See `PhysXWorld` and `PhysicsComponent` for details.

## Camera

`main.cpp` uses a `Renderer::setView()` camera centred on the player each frame, clamped to `map.getBounds()` so the camera never shows outside the world.

## Room / Zone Transitions

Maps can define **transition zones** — axis-aligned rectangles that act as triggers. When the player's bounding box overlaps a transition zone, `TransitionManager` begins a **fade-to-black** sequence:

1. **FadingOut** (0.4 s) — a black overlay fades from transparent to opaque.
2. **Load** — the load callback fires: the current map is unloaded, the target map is loaded from JSON, PhysX statics are re-registered, and the player is placed at the target map's matching named spawn point.
3. **FadingIn** (0.4 s) — the overlay fades back to transparent.
4. **Idle** — normal gameplay resumes.

Each map can also define **named spawn points** (e.g. `"from_world_02"`) via the `spawnPoints` JSON object. When no matching name is found, the default `spawnPoint` is used.

## Starting Map Layout

The starting map (`maps/world_01.json`) is 3600 units wide:

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

A transition zone at the right edge (x=3200) leads to `world_02.json`. The second map has a transition on its left edge back to `world_01.json`.

## Map File Format

Maps are stored as JSON files in the `maps/` directory. At build time CMake copies the `maps/` folder next to the executable, so files are loaded with relative paths.

### Schema

```json
{
  "name": "World 1 - Starting Area",
  "bounds":     { "x": -200.0, "y": -500.0, "width": 3600.0, "height": 1200.0 },
  "spawnPoint": { "x": 150.0,  "y": 475.0 },
  "spawnPoints": {
    "default":       { "x": 150.0, "y": 475.0 },
    "from_world_02": { "x": 100.0, "y": 475.0 }
  },
  "platforms": [
    { "x": -200.0, "y": 500.0, "width": 3600.0, "height": 40.0, "r": 80, "g": 80, "b": 80 }
  ],
  "transitions": [
    {
      "name": "to_world_02",
      "x": 3200.0, "y": 300.0, "width": 50.0, "height": 240.0,
      "targetMap": "maps/world_02.json",
      "targetSpawn": "from_world_01"
    }
  ]
}
```

| Field          | Type   | Required | Description |
|----------------|--------|----------|-------------|
| `name`         | string | no       | Human-readable level name |
| `bounds`       | object | yes      | World extents; used for death zone + camera clamping |
| `spawnPoint`   | object | yes      | Default player start / respawn world position |
| `spawnPoints`  | object | no       | Named spawn points (key → `{x, y}`) for transition targets |
| `platforms`    | array  | yes      | List of platform objects |
| `transitions`  | array  | no       | List of transition zone objects |

Each platform object:

| Field    | Type  | Required | Description |
|----------|-------|----------|-------------|
| `x`,`y`  | float | yes      | Top-left world position |
| `width`  | float | yes      | Width in world units |
| `height` | float | yes      | Height in world units |
| `r`,`g`,`b` | int | no (default 100) | Fill colour channels |

Each transition object:

| Field         | Type   | Required | Default     | Description |
|---------------|--------|----------|-------------|-------------|
| `name`        | string | no       | `""`        | Human-readable identifier |
| `x`,`y`       | float  | yes      | —           | Top-left of trigger region |
| `width`       | float  | yes      | —           | Width of trigger region |
| `height`      | float  | yes      | —           | Height of trigger region |
| `targetMap`   | string | yes      | —           | Relative path to destination map JSON |
| `targetSpawn` | string | no       | `"default"` | Named spawn point in destination map |

### MapLoader (src/Map/MapLoader.h / .cpp)

Static utility class. Uses **nlohmann/json** (fetched automatically via CMake FetchContent).

```cpp
Map map = MapLoader::loadFromFile("maps/world_01.json");
```

Throws `std::runtime_error` if the file cannot be opened or the JSON is malformed.

