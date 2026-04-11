# GameObject & Component System

## Overview

The GameObject/Component system provides a flexible, data-driven way to compose game entities from reusable pieces of behaviour. Instead of deep inheritance hierarchies, each `GameObject` is a container of `Component` instances — each component is responsible for exactly one concern.

## Architecture

```
GameObject
 ├── position        (glm::vec2 — shared world position)
 └── components[]
      ├── RenderComponent    → draws a colored rectangle each frame
      └── MovementComponent  → reads input, applies velocity to position
```

### Component (src/Core/Component.h)

Abstract base class. All components inherit from this.

| Method | Purpose |
|--------|---------|
| `update(float dt)` | Called every frame with delta time |
| `render(Renderer&)` | Called every frame for drawing |
| `setOwner(GameObject*)` | Set by `GameObject::addComponent` automatically |
| `getOwner()` | Access the owning `GameObject` (e.g. to read/write position) |

### GameObject (src/Core/GameObject.h / .cpp)

Owns a `std::vector<std::unique_ptr<Component>>`.

| Method | Purpose |
|--------|---------|
| `addComponent<T>(args...)` | Constructs a `T`, sets its owner, stores it |
| `getComponent<T>()` | Returns the first component of type `T`, or `nullptr` |
| `update(float dt)` | Calls `update` on all components |
| `render(window)` | Calls `render` on all components |

### MovementComponent (src/Components/MovementComponent.h / .cpp)

Reads WASD and arrow-key input each frame, builds a velocity vector, and writes it back to `getOwner()->position`.

| Field | Purpose |
|-------|---------|
| `speed` | Movement units per second (default 300) |
| `velocity` | Current frame velocity (readable by other components) |

### RenderComponent (src/Components/RenderComponent.h / .cpp)

Draws a colored rectangle via `Renderer::drawRect()`. Each frame it syncs the rectangle's position to the owner's `position` field.

| Field | Purpose |
|-------|---------|
| `m_width`, `m_height` | Dimensions of the rectangle |
| `m_r`, `m_g`, `m_b`, `m_a` | RGBA color values (0–1 floats) |

## Usage Example

```cpp
GameObject player;
player.position = { 400.f, 300.f };
player.addComponent<RenderComponent>(50.f, 50.f, 0.f, 1.f, 0.f);  // green
player.addComponent<MovementComponent>(300.f);

// In the game loop:
player.update(dt);
player.render(renderer);
```

## Adding New Components

1. Create `src/Components/MyComponent.h` — inherit from `Component`
2. Override `update` and/or `render` as needed
3. Add your `.cpp` to `CMakeLists.txt`
4. Attach with `gameObject.addComponent<MyComponent>(...)`

## Notes

- **Update order matters**: components are updated in the order they were added. `MovementComponent` should be added before `RenderComponent` so the shape is synced to the updated position in the same frame.
- **Ownership**: `GameObject` exclusively owns its components via `unique_ptr`. Components hold a raw (non-owning) back-pointer to their owner.
- **Physics**: When Nvidia PhysX is integrated, a `PhysicsComponent` will drive position instead of `MovementComponent`, which will shift to providing intent (desired velocity) rather than directly writing position.

### HealthComponent (src/Components/HealthComponent.h / .cpp)

Tracks current and maximum hit points. Renders a colour-coded HP bar above the owning `GameObject` in world space.

| Method / Field | Purpose |
|----------------|---------|
| `HealthComponent(float maxHp)` | Initialises both current and max HP |
| `takeDamage(float amount)` | Reduces HP (clamped to 0). Fires `onDeath` when HP reaches zero |
| `heal(float amount)` | Restores HP (clamped to max). No effect if already dead |
| `getCurrentHp()` | Returns current HP |
| `getMaxHp()` | Returns maximum HP |
| `isDead()` | Returns `true` when HP ≤ 0 |
| `onDeath` | `std::function<void()>` callback invoked once when HP hits zero |
| `render(window)` | Draws a background + fill HP bar above the owner's position |
