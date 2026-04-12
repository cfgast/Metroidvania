# Architecture Overview

A 4–8 player 2D Metroidvania platformer written in C++17. Built with **OpenGL 3.3 Core** (rendering via GLFW + GLAD), **a custom InputSystem abstraction** (input, with GLFW backend), **GLM 1.0.1** (math types), **Nvidia PhysX** (collision/physics), and **nlohmann/json** (serialization). Uses **CMake ≥ 3.20** with `FetchContent` for GLM, JSON, GLFW, and FreeType; PhysX is pre-built in `third_party/`; GLAD and stb_image are vendored in `third_party/`.

---

## Directory Layout

```
Metroidvania/
├── src/
│   ├── main.cpp              # Game loop, entity setup, top-level orchestration
│   ├── Core/                  # Entity model, abilities, save system, input config
│   ├── Components/            # All gameplay Component subclasses
│   ├── Math/                  # GLM-based math types: Rect, IntRect, Color
│   ├── Input/                  # Backend-agnostic input abstraction
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
└── third_party/               # PhysX, GLAD, and stb_image vendored libraries
```

---

## Core Entity Model (`src/Core/`)

The game uses a **Component-Based Entity System**.

| File | Role |
|---|---|
| `Component.h` | Abstract base — virtual `update(dt)` and `render(Renderer&)`. Stores owner `GameObject*`. |
| `GameObject.h/.cpp` | Entity container. Holds `glm::vec2 position` and a `vector<unique_ptr<Component>>`. Provides `addComponent<T>()` / `getComponent<T>()` templates. |
| `Ability.h` | Enum `{DoubleJump, WallSlide, Dash}` with string serialization helpers. |
| `PlayerState.h` | Persistent progression: `set<Ability> unlockedAbilities`, `set<string> consumedPickups`. Carried across room transitions. |
| `InputBindings.h/.cpp` | **Singleton.** Configurable keyboard + controller bindings using `KeyCode` enum. Persists to `saves/controls.json`. |
| `SaveSystem.h/.cpp` | Static API. 3 save slots (`saves/save_N.json`). Serializes player position, map, HP, abilities, consumed pickups. |

---

## Components (`src/Components/`)

Each component adds one slice of behavior to a `GameObject`.

| Component | Purpose | Key Dependencies |
|---|---|---|
| `InputComponent` | Polls keyboard/gamepad via `InputSystem::current()`, exposes `InputState` bools (`moveLeft`, `moveRight`, `jump`, `dash`, `attack`). AI can inject input via `setInputState()`. | `InputBindings`, `InputSystem` |
| `PhysicsComponent` | Platformer physics: gravity (980), jumping (-520), wall-slide, dash, double-jump, ceiling/floor collision, fall-death respawn. Reads abilities from `PlayerState`. | `InputComponent`, `PlayerState`, `Map`, `PhysXWorld` |
| `RenderComponent` | Draws a colored rectangle via `Renderer::drawRect()`; skips draw if `AnimationComponent` is present. Stores size/color as plain floats. | `Renderer` |
| `AnimationComponent` | Frame-based sprite animation from sprite sheets. Uses `Renderer::TextureHandle` for lazy-loaded textures. Frame rects stored as plain `{x,y,w,h}` structs. | `Renderer` |
| `HealthComponent` | HP pool with `takeDamage` / `heal`. Renders green→red HP bar via `Renderer::drawRect()` / `drawRectOutlined()`. Fires `onDeath` callback. | `Renderer` |
| `CombatComponent` | Player melee: AABB hitbox in front of player, animated arc VFX via `Renderer::drawTriangleStrip()` / `drawLines()`, per-swing hit tracking. | `InputComponent`, `PhysicsComponent`, `HealthComponent` (enemies), `Renderer` |
| `EnemyAIComponent` | Patrol between two waypoints. Contact damage on AABB overlap with player. Stuck detection reverses direction. | `InputComponent`, `PhysicsComponent`, `HealthComponent` |
| `SlimeAttackComponent` | Slime-specific ranged attack: jitter → 8-particle radial spray → per-particle AABB hit check. Renders particles via `Renderer::drawCircle()`. 4–8 s cooldown. | `EnemyAIComponent`, `AnimationComponent`, `HealthComponent` (player), `Renderer` |
| `LightComponent` | Attaches a `Light` to a `GameObject`. `getLight()` returns the light with position synced to the owner. Used on the player for a warm dynamic glow; registered before draw calls so all geometry receives the light. | `Light` |

### Component wiring (typical player)

```
InputComponent ──► PhysicsComponent ──► RenderComponent / AnimationComponent
                         │
                    CombatComponent
                         │
                   HealthComponent
                         │
                   LightComponent
```

Enemies replace `CombatComponent` with `EnemyAIComponent` (+ optional `SlimeAttackComponent`).

---

## Map System (`src/Map/`)

| File | Role |
|---|---|
| `Platform.h` | Data struct: `Rect bounds`, `Color color`. |
| `EnemyDefinition.h` | Data struct: position, waypoints, speed, damage, HP, size. |
| `TransitionZone.h` | Data struct: trigger bounds, target map path, target spawn name, relative-positioning fields (`edgeAxis`, `targetBaseX`, `targetBaseY`). |
| `AbilityPickupDefinition.h` | Data struct: id, ability enum, position, size. |
| `LightDefinition.h` | Data struct: name, type, position (x/y/z), color (r/g/b), intensity, radius, spot-light cone angles. `toLight()` converts to renderable `Light`. |
| `Map.h/.cpp` | Aggregates all level data. Provides AABB checks for transitions and pickups. `registerPhysXStatics()` creates `PxRigidStatic` actors for platforms. Stores `LightDefinition` vector accessible via `getLights()`. |
| `MapLoader.h/.cpp` | Static `loadFromFile()` → parses JSON via nlohmann/json → returns `Map`. |
| `TransitionManager.h/.cpp` | State machine `Idle → FadingOut (0.4 s) → FadingIn (0.4 s)`. Fires a load callback at midpoint (screen fully black). The load callback in `main.cpp` supports **relative positioning**: when a zone has `edgeAxis` set, the player's offset along the shared edge is preserved (clamped to zone bounds); otherwise falls back to `targetSpawn` (legacy). Pending zone data and player position are captured in `main.cpp` before starting the fade. |

### Map JSON schema (e.g. `world_01.json`)

```jsonc
{
  "bounds": { "x", "y", "width", "height" },
  "spawnPoint": { "x", "y" },
  "spawnPoints": { "<name>": { "x", "y" } },
  "platforms": [{ "x","y","width","height","r","g","b" }],
  "enemies": [{ "x","y","waypointA","waypointB","speed","damage","hp","width","height" }],
  "transitions": [{ "x","y","width","height","targetMap","targetSpawn",
                    "edgeAxis","targetBaseX","targetBaseY" }],
  // edgeAxis: "vertical" (left/right adjacency) or "horizontal" (top/bottom).
  //   Empty/missing = legacy mode (uses targetSpawn).
  // targetBaseX/Y: origin in target map for relative player positioning.
  "abilityPickups": [{ "id","ability","x","y","width","height" }],
  "lights": [{ "name","type","x","y","z","r","g","b","intensity","radius",
               "directionX","directionY","innerCone","outerCone" }]
  // lights: optional array. type is "point" (default) or "spot".
  //   Spot-only fields: directionX, directionY, innerCone, outerCone (degrees).
}
```

### World JSON schema (e.g. `my_world.world.json`)

```jsonc
{
  "maps": [
    { "path": "world_01.json", "x": 0, "y": 0 },
    { "path": "world_02.json", "x": 3400, "y": 0 }
  ]
}
```

World files use the `.world.json` extension and reference individual map files by relative path. Each map entry includes a global X/Y offset in world space. The editor models (`WorldData`, `WorldMapEntry`, `EditorMap`, `LightData`) are defined in `Models.cs`. `MainForm.cs` provides File menu items for New/Open/Save World (with Ctrl+Shift+N/O/S shortcuts) and world file I/O (`LoadWorldFile`, `WriteWorldFile`). Opening a single map file auto-creates a one-entry world.

### Multi-Map Document Model

`MapCanvas` uses a multi-map document model via `List<EditorMap>`:

| Class | Role |
|---|---|
| `EditorMap` | Wraps a `MapData` with world-space offset (`WorldX`, `WorldY`), file path, and dirty flag. |
| `MapCanvas.EditorMaps` | The list of all maps currently loaded in the editor. |
| `MapCanvas.ActiveMap` | The currently focused `EditorMap`; editing tools operate only on this map's data. |
| `MapCanvas.Map` | Convenience property returning `ActiveMap?.Map` for backward compatibility. |

`LoadWorld(List<EditorMap>)` replaces the old `LoadMap()` entry point. A single-map `LoadMap(MapData)` overload still works by wrapping the map in a one-entry list. Inactive maps render dimmed with dotted borders; the active map renders at full opacity with a cyan highlighted border. `FitToView()` computes the bounding box of all loaded maps.

### Map Activation & World-Space Coordinate Handling

Clicking on an inactive map in the Select tool switches `ActiveMap` to that map. `HitMap(PointF worldPoint)` determines which map's world-offset bounds contain a point, preferring the active map to avoid accidental switches. `SetActiveMap()` clears selection, fires `ActiveMapChanged` and `SelectionChanged` events, and repaints. `MainForm` subscribes to `ActiveMapChanged` to refresh the properties panel and status bar (which shows `Active: <map name> | ...`).

All active-map drawing uses a `Graphics.TranslateTransform` to offset local coordinates by the active map's `WorldX`/`WorldY`, so maps render at their correct world positions. Hit-testing and element creation convert world coordinates to local coordinates via `WorldToLocal()` before comparing against map-local element data.

### Move Map Tool

The `EditorTool.MoveMap` tool allows repositioning maps in world space. Activated via the toolbar "Move Map [M]" button or the `M` keyboard shortcut. Clicking on a map starts a drag; the map's `WorldX`/`WorldY` offset is updated as the user moves the mouse, with optional grid snapping. A dashed cyan outline and coordinate tooltip provide visual feedback during the drag. Only the world-space offset changes — internal map coordinates are unaffected. On mouse-up, `RegenerateTransitions()` is called to recalculate adjacency-based transitions.

### Place Light Tool

The `EditorTool.DrawLight` tool places point and spot lights on the map. Activated via the toolbar "Place Light [L]" button or the `L` keyboard shortcut. Clicking on the canvas creates a new `LightData` at the cursor position with default values (point light, white color, radius 200, intensity 1, Z height 80). Selected lights display a sun icon with radiating rays at the light position and a translucent radius circle colored to match the light color. Four cardinal grab handles on the radius circle allow interactive radius adjustment via drag. Spot lights show a direction arrow and a dash-dot radius circle. Light properties (name, type, position, Z, color, intensity, radius, spot direction, cone angles) are editable in the left sidebar property panel. The `LightData` model class in `Models.cs` serializes to/from the `"lights"` array in the map JSON. `SelectableType.Light` has been added along with full selection, dragging, deletion, and hit-testing support in `MapCanvas`. Inactive maps render lights as small dimmed circles.

### Auto-Transition Generation (`TransitionGenerator.cs`)

`TransitionGenerator` is a static class that detects edge adjacency between maps and auto-generates transition zones and spawn points. `RegenerateTransitions(List<EditorMap>)` clears all existing transitions and auto-generated spawn points (those prefixed `from_`), then checks all map pairs for shared edges. A shared edge occurs when one map's boundary exactly touches another's (within 0.5 units) and their perpendicular ranges overlap. For each adjacency, it creates:

- **Transition zones**: 50-unit deep rectangles extending inward from the shared edge in both maps, spanning the overlap length. Named `to_<target_map>` with `targetSpawn` set to `"default"` (fallback). Each zone includes `edgeAxis` (`"vertical"` for left/right, `"horizontal"` for top/bottom), `targetBaseX`, and `targetBaseY` for relative player positioning. Auto-transitions no longer generate `from_*` spawn points; existing ones are cleaned up on regeneration.

Regeneration is triggered automatically on: MoveMap mouse-up, bounds change (Apply in properties panel), world load, and before every save (both single-map and world saves).

Auto-generated transitions are **display-only** — they cannot be selected, moved, resized, or deleted by the user. They render as blue semi-transparent dashed rectangles labeled with "⚡ [auto]" to indicate they are auto-generated. The manual transition drawing tool (`DrawTransition`), its toolbar button, keyboard shortcut (`T`), and properties panel have been removed. The `SelectableType.Transition` enum value and all transition-specific hit-testing, selection, and drag logic have been removed from `MapCanvas`.

When saving, `NormalizeTransitionPaths()` converts absolute `targetMap` paths in transitions to relative paths (e.g., `maps/world_02.json`) for compatibility with the game runtime's `MapLoader`. Single-map files opened outside a world do not generate transitions (the `RegenerateTransitions` guard requires ≥2 maps).

### World Management UI

`MainForm` includes a "WORLD MAPS" panel in the left sidebar (below the dynamic property panels) with an owner-drawn `ListBox` and three buttons:

| Control | Purpose |
|---|---|
| Maps ListBox | Shows all `EditorMap` entries with map name, file path, and dirty indicator (`*`). Active map rendered in bold cyan. Clicking an entry calls `SetActiveMap()`. |
| "Add…" button | Opens a file dialog, loads an existing `.json` map, places it to the right of the rightmost map, calls `RegenerateTransitions()` and `FitToView()`. |
| "New" button | Prompts for a save path, creates a blank map (default bounds + floor platform), saves it immediately, adds to world. |
| "Remove" button | Removes the active map from the world (does not delete the file). Confirms if the map has unsaved changes. Switches to next available map. |

These actions are also available via the File menu ("Add Map to World…", "New Map in World…", "Remove Map from World").

**Dirty tracking** is world-level: `_isDirty` tracks per-edit changes, `_worldStructureDirty` tracks maps added/removed/repositioned, and each `EditorMap.IsDirty` tracks per-map changes. `ConfirmDiscard()` and `UpdateTitle()` check all three. The title bar shows an asterisk (`*`) if any source of dirty state is set. `WriteWorldFile` clears all dirty flags after a successful save.

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

A back-end–agnostic rendering abstraction that isolates all draw calls behind a single interface so the graphics back-end can be swapped without touching game code.

| File | Role |
|---|---|
| `Light.h` | `LightType` enum (`Point`, `Spot`) and `Light` struct: `position` (vec2), `color` (vec3), `intensity`, `radius`, `z`, `direction` (vec2), `innerCone`, `outerCone`, `type`. Data-only header used by the renderer and future light components. |
| `Renderer.h` | Pure-virtual interface. Covers `getInput()` for InputSystem access, window operations (`isOpen`/`close`/`setMouseCursorVisible`/`setWindowSize`/`setWindowPosition`/`getDesktopSize`), lifecycle (`clear`/`display`), frame/lighting pass (`beginFrame`/`endFrame`/`addLight`/`clearLights`/`setAmbientColor`), camera/view, primitives (rect, circle, rounded-rect), textured sprites via opaque `TextureHandle`, text via `FontHandle`, and raw vertex-colored geometry (`drawTriangleStrip`/`drawLines`). |
| `GLRenderer.h/.cpp` | OpenGL 3.3 Core implementation (active renderer). Owns a `GLFWwindow*`, a `GLFWInput` instance, flat-color `Shader` (with per-pixel lighting), vertex-color `Shader`, textured `Shader` (with normal map support and per-pixel lighting), text `Shader`, fullscreen blit `Shader`, persistent unit-quad VAO/VBO, fullscreen quad VAO/VBO, dynamic geometry VAO/VBO (for circles and rounded rects), vertex-color VAO/VBO (for triangle strips and lines), dynamic sprite VAO/VBO, dynamic text VAO/VBO, and an off-screen FBO (color texture + depth/stencil renderbuffer, recreated on resize). Flat and texture shaders embed a shared GLSL lighting block supporting up to 32 point/spot lights with quadratic attenuation and angular spot falloff. Normal maps are auto-loaded from `_n.png` companion files and cached. Ambient color defaults to (0.8, 0.8, 0.9). Uses a `m_worldPass` flag to differentiate world rendering from UI rendering: `beginFrame()` sets it true (binds FBO, enables lighting), `endFrame()` sets it false (blits FBO, disables lighting). When `m_worldPass` is false, `uploadLightUniforms()` sends ambient=(1,1,1) and zero lights so UI elements render at full brightness. `addLight()` / `clearLights()` / `setAmbientColor()` manage light state. All world drawing between `beginFrame` and `endFrame` renders to the FBO with lighting; UI rendering after `endFrame` renders directly to the default framebuffer without lighting. |
| `Shader.h/.cpp` | Compiles and links vertex + fragment GLSL source strings into a GL program. Provides `use()`, `setMat4()`, `setVec4()`, `setVec3()`, `setVec2()`, `setFloat()`, `setInt()` uniform helpers with error logging. |

All rendering and input go through the `Renderer` and `InputSystem` abstractions. The active back-end is `GLRenderer` + `GLFWInput`. All math types use GLM and custom types from `src/Math/Types.h`.

---

## Input (`src/Input/`)

A back-end–agnostic input abstraction that isolates all input polling and event handling behind a single interface so the input back-end can be swapped without touching game code.

| File | Role |
|---|---|
| `InputTypes.h` | Enums (`KeyCode`, `MouseButton`, `GamepadButton`, `GamepadAxis`, `InputEventType`) and the `InputEvent` struct used by all game code. |
| `InputSystem.h` | Pure-virtual interface: `pollEvent()`, `isKeyPressed()`, `isGamepadConnected()`, `getGamepadAxis()`, `isGamepadButtonPressed()`, `setMouseCursorVisible()`. Also provides `InputSystem::current()` static accessor (set by the active backend). |
| `GLFWInput.h/.cpp` | GLFW 3.4 implementation (active input backend). Uses GLFW callbacks (`glfwSetKeyCallback`, `glfwSetMouseButtonCallback`, `glfwSetCursorPosCallback`, `glfwSetWindowSizeCallback`, `glfwSetWindowCloseCallback`, `glfwSetJoystickCallback`) to queue `InputEvent`s. Polls gamepad state via `glfwGetGamepadState()` with raw-joystick fallback. Maps `KeyCode` ↔ `GLFW_KEY_*`, `GamepadButton` ↔ `GLFW_GAMEPAD_BUTTON_*`, synthesizes D-pad axes from buttons. Owned by `GLRenderer`. |

---

## UI (`src/UI/`)

| File | Role |
|---|---|
| `UIStyle.h` | Shared dark-theme palette and drawing helpers (`drawMenuItem`, `drawMenuRow`, `drawGlow`, `drawAccentBar`). All helpers take `Renderer&` and use float RGBA colors. Returns `UIBounds` struct. |
| `SaveSlotScreen.h/.cpp` | Startup screen: 3 save slots, resolution selector, controls button. Renders via `Renderer&`; `handleEvent()` takes `const InputEvent&` and `Renderer&` (for resolution changes via `setWindowSize`/`setWindowPosition`). |
| `PauseMenu.h/.cpp` | In-game pause: Resume / Save / Quit. Keyboard + controller + mouse. `handleEvent()` takes `const InputEvent&`. Renders via `Renderer&`. |
| `ControlsMenu.h/.cpp` | Full-screen rebinding UI. Enter rebind mode → press new key → saved via `InputBindings`. `handleEvent()` takes `const InputEvent&`. Renders via `Renderer&`. |

---

## Debug (`src/Debug/`)

`DebugMenu` — F1-toggled overlay with "Open Map…" button that launches a Windows native file dialog to hot-load any map JSON. Renders via `Renderer&`; `handleEvent()` takes `const InputEvent&`.

---

## Main Game Loop (`src/main.cpp`, ~400 lines)

```
 ┌─ Init ──────────────────────────────────────┐
 │  PhysXWorld::init()                         │
 │  InputBindings::load()                      │
 │  Create GLRenderer (800×600, 60 FPS)      │
 │  Show SaveSlotScreen → new/load game        │
 └─────────────────────────────────────────────┘
           │
 ┌─ Per Frame ─────────────────────────────────┐
 │  1. Poll events via renderer.getInput()     │
 │     .pollEvent(InputEvent)                  │
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
 │     • renderer.beginFrame() (bind FBO)      │
 │     • renderer.clearLights()                │
 │     • renderer.addLight(playerLight)        │
 │     • renderer.setView() for camera         │
 │     • renderer.clear() + draw calls         │
 │     • renderer.endFrame() (blit FBO)        │
 │     • renderer.resetView() for UI overlay   │
 │     • renderer.display()                    │
 └─────────────────────────────────────────────┘
```

### Entity setup

- **Player**: `InputComponent` → `PhysicsComponent` → `RenderComponent` → `AnimationComponent` → `HealthComponent` → `CombatComponent` → `LightComponent`. Death callback teleports to spawn and refills HP. LightComponent provides the player's dynamic light (warm white point light, registered before draw calls).
- **Enemy**: `InputComponent` → `PhysicsComponent` → `RenderComponent` → `AnimationComponent` → `HealthComponent` → `EnemyAIComponent` → (optional) `SlimeAttackComponent`. Death callback erases enemy from list.

---

## Build & Dependencies

| Dependency | Source | Purpose |
|---|---|---|
| GLFW 3.4 | FetchContent (Git) | Windowing, input, OpenGL context |
| GLAD (GL 3.3 Core) | Vendored in `third_party/glad/` | OpenGL function loader |
| stb_image | Vendored in `third_party/stb/` | Image loading for OpenGL textures |
| FreeType 2.13.3 | FetchContent (Git) | Font rasterization for OpenGL text |
| GLM 1.0.1 | FetchContent (Git) | Vector/matrix math types |
| nlohmann/json 3.11.3 | FetchContent (Git) | JSON parse/serialize |
| Nvidia PhysX | Pre-built in `third_party/` | Collision detection, rigid-body physics |
| Windows API (`comdlg32`) | System | Debug file dialog |

Build: `cmake --preset default && cmake --build build`

Post-build steps copy PhysX DLLs, `maps/`, and `assets/` next to the executable.

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
