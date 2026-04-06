# Map Files

Maps are JSON files loaded at runtime by `MapLoader::loadFromFile()`. Place any `.json` map file in this directory — CMake will copy the whole folder next to the executable at build time, so files can be referenced by relative path (e.g. `"maps/world_01.json"`).

---

## File Format

```json
{
  "name": "World 1 - Starting Area",
  "bounds":     { "x": -200.0, "y": -500.0, "width": 3600.0, "height": 1200.0 },
  "spawnPoint": { "x": 150.0,  "y": 475.0 },
  "platforms": [
    { "x": -200.0, "y": 500.0, "width": 3600.0, "height": 40.0, "r": 80, "g": 80, "b": 80 },
    { "x":  250.0, "y": 380.0, "width":  200.0, "height": 20.0, "r": 120, "g": 80, "b": 40 }
  ]
}
```

### Top-level fields

| Field        | Type   | Required | Description |
|--------------|--------|----------|-------------|
| `name`       | string | No       | Human-readable level name (not used at runtime) |
| `bounds`     | object | Yes      | World extents — used for camera clamping and the fall-off death zone |
| `spawnPoint` | object | Yes      | World position the player starts at and respawns to after falling off the map |
| `platforms`  | array  | Yes      | List of walkable platform rectangles |

### `bounds` and `spawnPoint`

Both use the same `{ "x", "y" }` format. `bounds` also requires `"width"` and `"height"`.

```json
"bounds":     { "x": -200.0, "y": -500.0, "width": 3600.0, "height": 1200.0 }
"spawnPoint": { "x": 150.0,  "y": 475.0 }
```

- All values are in **world units** (pixels at 1:1 scale).
- The death zone triggers when the player's Y position exceeds `bounds.y + bounds.height`.
- `spawnPoint` should sit on top of a platform. With a player half-height of **25 units**, place the spawn Y at `platformTop - 25`.

### `platforms` entries

Each platform is an axis-aligned rectangle.

| Field         | Type  | Required | Default | Description |
|---------------|-------|----------|---------|-------------|
| `x`           | float | Yes      | —       | Left edge in world space |
| `y`           | float | Yes      | —       | Top edge in world space |
| `width`       | float | Yes      | —       | Width in world units |
| `height`      | float | Yes      | —       | Height in world units |
| `r` `g` `b`   | int   | No       | 100     | RGB fill colour (0–255 each) |

---

## Coordinate System

- **Origin (0, 0)** is the top-left of the world.
- **X increases right**, **Y increases downward**.
- The player stands *on top* of a platform, so the platform's `y` value is the floor the player walks on.

```
        x →
   ┌────────────────────┐
 y │                    │
 ↓ │   player □         │
   │  ══════════ ← platform (y=380)
   │                    │
   │════════════════════│ ← ground (y=500)
   └────────────────────┘
```

---

## Adding a New Map

1. Copy an existing `.json` file and rename it (e.g. `world_02.json`).
2. Edit `bounds` to set the world size.
3. Set `spawnPoint` on top of a solid platform (`y = platformTop - 25`).
4. Add or modify entries in `platforms`.
5. In code, load it with:
   ```cpp
   Map map = MapLoader::loadFromFile("maps/world_02.json");
   ```
6. Rebuild — CMake will copy the new file to the output directory automatically.
