# Architecture Overview

A 4–8 player 2D Metroidvania platformer written in C++17. Built with **SFML 2.6** (rendering/input), **Nvidia PhysX** (collision/physics), and **nlohmann/json** (serialization). Uses **CMake ≥ 3.20** with `FetchContent` for SFML and JSON; PhysX is pre-built in `third_party/`.

---

## Directory Layout

```
Metroidvania/
├── src/
│   ├── main.cpp              # Game loop, entity setup, top-level orchestration
│   ├── Core/                  # Entity model, abilities, save system, input config
│   ├── Components/            # All gameplay Component subclasses
│   ├── Map/                   # Level data, JSON loading, room transitions
│   ├── Physics/               # PhysX singleton wrapper
│   ├── Rendering/             # Renderer abstraction and back-end implementations
│   ├── Debug/                 # F1 debug map-loader dialog
│   └── UI/                    # Menus (pause, save-slot, controls), shared styling
├── assets/                    # Sprite sheets (player, slime)
├── maps/                      # JSON level files (world_01–03)
├── saves/                     # Per-slot JSON save files + controls.json
├── docs/                      # Design docs (GameObjects, Map, Physics)
├── tools/
│   ├── MapEditor/             # C# WinForms map editor
│   └── run-tasks.ps1          # Automation: runs ImplementationPlan tasks via Copilot
└── third_party/               # PhysX source, build, and install directories
```

---

## Core Entity Model (`src/Core/`)

The game uses a **Component-Based Entity System**.

| File | Role |
|---|---|
| `Component.h` | Abstract base — virtual `update(dt)` and `render(Renderer&)`. Stores owner `GameObject*`. |
| `GameObject.h/.cpp` | Entity container. Holds `sf::Vector2f position` and a `vector<unique_ptr<Component>>`. Provides `addComponent<T>()` / `getComponent<T>()` templates. |
| `Ability.h` | Enum `{DoubleJump, WallSlide, Dash}` with string serialization helpers. |
| `PlayerState.h` | Persistent progression: `set<Ability> unlockedAbilities`, `set<string> consumedPickups`. Carried across room transitions. |
| `InputBindings.h/.cpp` | **Singleton.** Configurable keyboard + controller bindings. Persists to `saves/controls.json`. |
| `SaveSystem.h/.cpp` | Static API. 3 save slots (`saves/save_N.json`). Serializes player position, map, HP, abilities, consumed pickups. |

---

## Components (`src/Components/`)

Each component adds one slice of behavior to a `GameObject`.

| Component | Purpose | Key Dependencies |
|---|---|---|
| `InputComponent` | Polls keyboard/gamepad, exposes `InputState` bools (`moveLeft`, `moveRight`, `jump`, `dash`, `attack`). AI can inject input via `setInputState()`. | `InputBindings` |
| `PhysicsComponent` | Platformer physics: gravity (980), jumping (-520), wall-slide, dash, double-jump, ceiling/floor collision, fall-death respawn. Reads abilities from `PlayerState`. | `InputComponent`, `PlayerState`, `Map`, `PhysXWorld` |
| `RenderComponent` | Draws a colored rectangle via `Renderer::drawRect()`; skips draw if `AnimationComponent` is present. Stores size/color as plain floats. | `Renderer` |
| `AnimationComponent` | Frame-based sprite animation from sprite sheets. Uses `Renderer::TextureHandle` for lazy-loaded textures. Frame rects stored as plain `{x,y,w,h}` structs. | `Renderer` |
| `HealthComponent` | HP pool with `takeDamage` / `heal`. Renders green→red HP bar via `Renderer::drawRect()` / `drawRectOutlined()`. Fires `onDeath` callback. | `Renderer` |
| `CombatComponent` | Player melee: AABB hitbox in front of player, animated arc VFX via `Renderer::drawTriangleStrip()` / `drawLines()`, per-swing hit tracking. | `InputComponent`, `PhysicsComponent`, `HealthComponent` (enemies), `Renderer` |
| `EnemyAIComponent` | Patrol between two waypoints. Contact damage on AABB overlap with player. Stuck detection reverses direction. | `InputComponent`, `PhysicsComponent`, `HealthComponent` |
| `SlimeAttackComponent` | Slime-specific ranged attack: jitter → 8-particle radial spray → per-particle AABB hit check. Renders particles via `Renderer::drawCircle()`. 4–8 s cooldown. | `EnemyAIComponent`, `AnimationComponent`, `HealthComponent` (player), `Renderer` |

### Component wiring (typical player)

```
InputComponent ──► PhysicsComponent ──► RenderComponent / AnimationComponent
                         │
                    CombatComponent
                         │
                   HealthComponent
```

Enemies replace `CombatComponent` with `EnemyAIComponent` (+ optional `SlimeAttackComponent`).

---

## Map System (`src/Map/`)

| File | Role |
|---|---|
| `Platform.h` | Data struct: `FloatRect bounds`, `Color color`. |
| `EnemyDefinition.h` | Data struct: position, waypoints, speed, damage, HP, size. |
| `TransitionZone.h` | Data struct: trigger bounds, target map path, target spawn name. |
| `AbilityPickupDefinition.h` | Data struct: id, ability enum, position, size. |
| `Map.h/.cpp` | Aggregates all level data. Provides AABB checks for transitions and pickups. `registerPhysXStatics()` creates `PxRigidStatic` actors for platforms. |
| `MapLoader.h/.cpp` | Static `loadFromFile()` → parses JSON via nlohmann/json → returns `Map`. |
| `TransitionManager.h/.cpp` | State machine `Idle → FadingOut (0.4 s) → FadingIn (0.4 s)`. Fires a load callback at midpoint (screen fully black). |

### Map JSON schema (e.g. `world_01.json`)

```jsonc
{
  "bounds": { "x", "y", "width", "height" },
  "spawnPoint": { "x", "y" },
  "spawnPoints": { "<name>": { "x", "y" } },
  "platforms": [{ "x","y","width","height","r","g","b" }],
  "enemies": [{ "x","y","waypointA","waypointB","speed","damage","hp","width","height" }],
  "transitions": [{ "x","y","width","height","targetMap","targetSpawn" }],
  "abilityPickups": [{ "id","ability","x","y","width","height" }]
}
```

---

## Physics (`src/Physics/`)

`PhysXWorld` — **Singleton** wrapping the PhysX SDK.

- Initializes Foundation → Physics → CPU Dispatcher → Scene (zero global gravity, zero restitution).
- `createDynamicBox()` — for player/enemies. Z-locked, rotation-locked, CCD enabled, per-actor gravity disabled (handled in `PhysicsComponent`).
- `createStaticBox()` — for platforms.
- `clearStaticActors()` — used on map reload.
- `step(dt)` / `beginFrame()` — advance simulation once per frame.

---

## Rendering (`src/Rendering/`)

A back-end–agnostic rendering abstraction that isolates all draw calls behind a single interface so the graphics back-end can be swapped (SFML → OpenGL) without touching game code.

| File | Role |
|---|---|
| `Renderer.h` | Pure-virtual interface. Covers window operations (`isOpen`/`close`/`setMouseCursorVisible`), lifecycle (`clear`/`display`), camera/view, primitives (rect, circle, rounded-rect), textured sprites via opaque `TextureHandle`, text via `FontHandle`, and raw vertex-colored geometry (`drawTriangleStrip`/`drawLines`). No SFML types in the public API. |
| `SFMLRenderer.h/.cpp` | SFML 2.6 implementation. Owns `sf::RenderWindow` and internal `handle → sf::Texture / sf::Font` maps. Constructor takes title, width, height, FPS cap. Adds `pollEvent(sf::Event&)` for SFML-specific event polling. Exposes `getWindow()` for legacy code that still needs direct SFML access (e.g. resolution resize in SaveSlotScreen). |

**Migration status:** The entire game loop in `main.cpp` now runs through the `Renderer` abstraction end to end. All `window.clear()`, `window.display()`, `window.setView()`, `window.isOpen()`, `window.close()`, and `window.setMouseCursorVisible()` calls go through `renderer`. Dash ghost rendering uses `renderer.drawRect()` instead of `sf::RectangleShape`. The only remaining SFML usage outside `src/Rendering/` is SFML event types (`sf::Event`, `sf::Keyboard`), `sf::Clock`, math types (`sf::Vector2f`, `sf::FloatRect`) used as data types throughout game code, and `SaveSlotScreen::handleEvent()` which still takes `sf::RenderWindow&` for resolution changes via `renderer.getWindow()` — these will be migrated in subsequent tasks.

---

## UI (`src/UI/`)

| File | Role |
|---|---|
| `UIStyle.h` | Shared dark-theme palette and drawing helpers (`drawMenuItem`, `drawMenuRow`, `drawGlow`, `drawAccentBar`). All helpers take `Renderer&` and use float RGBA colors. Returns `UIBounds` struct instead of `sf::FloatRect`. |
| `SaveSlotScreen.h/.cpp` | Startup screen: 3 save slots, resolution selector, controls button. Renders via `Renderer&`; lazy-loads font via `Renderer::loadFont()`. Still takes `sf::RenderWindow&` in `handleEvent()` for resolution resize. |
| `PauseMenu.h/.cpp` | In-game pause: Resume / Save / Quit. Keyboard + controller + mouse. Renders via `Renderer&`. |
| `ControlsMenu.h/.cpp` | Full-screen rebinding UI. Enter rebind mode → press new key → saved via `InputBindings`. Renders via `Renderer&`. |

---

## Debug (`src/Debug/`)

`DebugMenu` — F1-toggled overlay with "Open Map…" button that launches a Windows native file dialog to hot-load any map JSON. Renders via `Renderer&`; `handleEvent()` no longer needs the window reference.

---

## Main Game Loop (`src/main.cpp`, ~400 lines)

```
 ┌─ Init ──────────────────────────────────────┐
 │  PhysXWorld::init()                         │
 │  InputBindings::load()                      │
 │  Create SFMLRenderer (800×600, 60 FPS)      │
 │  Show SaveSlotScreen → new/load game        │
 └─────────────────────────────────────────────┘
           │
 ┌─ Per Frame ─────────────────────────────────┐
 │  1. Poll events via renderer.pollEvent()    │
 │  2. Debug menu poll (hot-load map)          │
 │  3. If paused → render pause menu, skip ↓   │
 │  4. If transitioning → render fade, skip ↓  │
 │  5. Gameplay update:                        │
 │     • PhysXWorld::beginFrame() + step(dt)   │
 │     • Update all components (player+enemies)│
 │     • Check transition zones → start fade   │
 │     • Check ability pickups → unlock        │
 │     • Remove dead enemies                   │
 │  6. Render (all via renderer.*):            │
 │     • renderer.setView() for camera         │
 │     • renderer.clear() + draw calls         │
 │     • renderer.resetView() for UI overlay   │
 │     • renderer.display()                    │
 └─────────────────────────────────────────────┘
```

### Entity setup

- **Player**: `InputComponent` → `PhysicsComponent` → `RenderComponent` → `AnimationComponent` → `HealthComponent` → `CombatComponent`. Death callback teleports to spawn and refills HP.
- **Enemy**: `InputComponent` → `PhysicsComponent` → `RenderComponent` → `AnimationComponent` → `HealthComponent` → `EnemyAIComponent` → (optional) `SlimeAttackComponent`. Death callback erases enemy from list.

---

## Build & Dependencies

| Dependency | Source | Purpose |
|---|---|---|
| SFML 2.6.1 | FetchContent (Git) | Graphics, windowing, input, audio |
| nlohmann/json 3.11.3 | FetchContent (Git) | JSON parse/serialize |
| Nvidia PhysX | Pre-built in `third_party/` | Collision detection, rigid-body physics |
| Windows API (`comdlg32`) | System | Debug file dialog |

Build: `cmake --preset default && cmake --build build`

Post-build steps copy SFML DLLs, PhysX DLLs, `maps/`, and `assets/` next to the executable.

---

## Design Patterns

| Pattern | Usage |
|---|---|
| Component-Based Entity | `GameObject` + `Component` subclasses |
| Singleton | `PhysXWorld`, `InputBindings` |
| State Machine | `TransitionManager` fade states |
| Observer / Callback | `HealthComponent::onDeath`, `TransitionManager::setLoadCallback` |
| Factory | `MapLoader::loadFromFile()` |
| Strategy | Each `Component` encapsulates one behavior |

---

## Save File Format (`saves/save_N.json`)

```jsonc
{
  "currentMapFile": "maps/world_01.json",
  "playerPosition": { "x": 110, "y": 240 },
  "currentHp": 100.0,
  "maxHp": 100.0,
  "unlockedAbilities": ["DoubleJump", "WallSlide", "Dash"],
  "consumedPickups": ["pickup_01", "pickup_02"]
}
```
