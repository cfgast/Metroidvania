==============================================================================
Implementation Notes
==============================================================================
This file contains pending tasks for agents to pick up. Each task should be
implemented by an individual agent, one at a time, in order.

When a task is completed:
1. Change "Implemented: false" to "Implemented: true" on the task
2. Move the completed task (the full block from ============ to the next
   ============) to CompletedTasks.md, appending it at the end of that file
3. Update architecture.md so future agents understand the current layout
4. Update anything in ReadMe.md that was changed by the task
5. Add new build artifacts to .gitignore
6. Commit and push with a descriptive message

See CompletedTasks.md for the history of all previously completed work.
See architecture.md for the current codebase layout.

==============================================================================
==============================================================================
==  NORMAL-MAPPED 2D LIGHTING SYSTEM                                       ==
==============================================================================
==============================================================================

Add a full normal-mapped 2D lighting system to the game. The world uses a
bright ambient (fully visible without lights). Lights are decorative and
atmospheric: point lights and spot lights interact with normal maps on
sprites to produce depth and material detail. The player emits a subtle
dynamic light that follows them.

Design overview:
- Render-to-FBO pipeline: draw scene to an off-screen color framebuffer
- Lighting pass composites lights over the scene using an additive light map
- Normal maps (optional per-sprite _n.png companion textures) provide surface
  detail for per-pixel Phong-style lighting in the sprite/flat shaders
- Lights are defined in map JSON ("lights" array) and loaded by MapLoader
- Editor gets a "Place Light" tool for positioning lights with properties
- The player has a dynamic light attached via a new LightComponent

Architecture notes for agents:
- GLRenderer owns the FBO, light-map texture, and light shader
- Renderer interface gains addLight()/clearLights() and beginFrame()/endFrame()
- All existing draw calls (drawRect, drawSprite, etc.) render to the FBO
  during the world pass, then a fullscreen quad composites the result
- Normal maps use naming convention: assets/player_spritesheet_n.png
  alongside assets/player_spritesheet.png. If no _n file exists, a flat
  normal (0.5, 0.5, 1.0) is assumed (no surface detail, still receives light)
- Sprite shader needs world-space fragment position for per-pixel lighting
- Flat-color shader also receives light (platforms should be illuminated too)
- UI rendering (after resetView) bypasses lighting entirely

Coordinate system:
- 2D world, Y-down. Normals in tangent space: (0.5, 0.5, 1.0) = flat surface
- Light Z = height above the 2D plane (controls spread and falloff)
- Light colors are RGB, intensity is a separate multiplier

==============================================================================
Task 3: Lighting compositing pass
==============================================================================
Implemented: false

Wire up the light data to the shaders during rendering so lights actually
affect the scene visually.

GLRenderer changes:
- Before each drawRect/drawSprite/drawCircle/drawRoundedRect call, upload
  the current light array to the active shader uniforms
- In beginFrame(), set a flag (m_worldPass = true)
- In endFrame(), set m_worldPass = false
- When m_worldPass is false (UI rendering), skip light uniform upload and
  use ambient = (1,1,1) so UI is fully lit
- clearLights() empties the light vector at frame start
- addLight() pushes a Light struct into the vector

Update main.cpp:
- After renderer.beginFrame(), call renderer.clearLights()
- Add a test point light at the player position:
    Light playerLight;
    playerLight.position = player.position;
    playerLight.color = {1.0, 0.95, 0.8};  // warm white
    playerLight.intensity = 0.6;
    playerLight.radius = 300;
    playerLight.z = 80;
    playerLight.type = LightType::Point;
    renderer.addLight(playerLight);
- Verify lighting is visible and moves with the player

The scene should now show the player emitting a visible warm glow.
Platforms and sprites near the player should be visibly brighter.

==============================================================================
Task 4: Map light definitions and MapLoader integration
==============================================================================
Implemented: false

Add light data to the map JSON format and load them at runtime.

Create src/Map/LightDefinition.h:
  struct LightDefinition {
      std::string name;
      LightType   type = LightType::Point;
      float x, y, z = 80.f;
      float r = 1.f, g = 1.f, b = 1.f;
      float intensity = 1.f;
      float radius = 200.f;
      // Spot-only:
      float directionX = 0.f, directionY = 1.f;
      float innerConeAngle = 30.f;  // degrees, converted to cos at load
      float outerConeAngle = 45.f;
  };

Update Map.h / Map.cpp:
- Add m_lights vector, addLight(), getLights() accessors

Update MapLoader.cpp:
- Parse optional "lights" array from JSON:
    {
        "name": "torch_01",
        "type": "point",
        "x": 500, "y": 200, "z": 80,
        "r": 1.0, "g": 0.8, "b": 0.3,
        "intensity": 1.2,
        "radius": 250
    }
  For spot lights, also: directionX, directionY, innerCone, outerCone

Update main.cpp:
- After loading a map, iterate map.getLights() and call renderer.addLight()
  for each one every frame (before drawing)
- Keep the hardcoded player light from Task 3

Add a test light to maps/world_01.json in the "lights" array near a platform
so the effect can be seen immediately.

==============================================================================
Task 5: LightComponent for player dynamic light
==============================================================================
Implemented: false

Replace the hardcoded player light in main.cpp with a proper component.

Create src/Components/LightComponent.h / .cpp:
  class LightComponent : public Component {
      Light m_light;
  public:
      LightComponent(GameObject& owner);
      void setLight(const Light& l) { m_light = l; }
      Light getLight() const;  // returns m_light with position = owner.position
      void update(float dt) override; // optional: animate flicker
  };

Update main.cpp:
- Attach a LightComponent to the player with warm white settings
- In the render section, get the player's LightComponent and addLight()
- Remove hardcoded player light code

Register the component in the component system if needed. The player should
have a visible subtle glow that follows them as before, but now via the
component system.

==============================================================================
Task 6: Map editor - Place Light tool
==============================================================================
Implemented: false

Add light placement and editing to the C# WinForms map editor.

Update tools/MapEditor/Models.cs:
- Add LightData class:
    public class LightData {
        [JsonPropertyName("name")]       public string Name       { get; set; }
        [JsonPropertyName("type")]       public string Type       { get; set; } = "point";
        [JsonPropertyName("x")]          public float  X          { get; set; }
        [JsonPropertyName("y")]          public float  Y          { get; set; }
        [JsonPropertyName("z")]          public float  Z          { get; set; } = 80f;
        [JsonPropertyName("r")]          public float  R          { get; set; } = 1f;
        [JsonPropertyName("g")]          public float  G          { get; set; } = 1f;
        [JsonPropertyName("b")]          public float  B          { get; set; } = 1f;
        [JsonPropertyName("intensity")]  public float  Intensity  { get; set; } = 1f;
        [JsonPropertyName("radius")]     public float  Radius     { get; set; } = 200f;
        // Spot-only:
        [JsonPropertyName("directionX")] public float  DirectionX { get; set; } = 0f;
        [JsonPropertyName("directionY")] public float  DirectionY { get; set; } = 1f;
        [JsonPropertyName("innerCone")]  public float  InnerCone  { get; set; } = 30f;
        [JsonPropertyName("outerCone")]  public float  OuterCone  { get; set; } = 45f;
    }
- Add to MapData: public List<LightData> Lights { get; set; } = new();

Update tools/MapEditor/MainForm.cs:
- Add "Place Light" tool button to the toolbar
- When active, clicking places a new point light at mouse world position
- Selected light shows a property panel (or use PropertyGrid) for editing
  color, intensity, radius, type, z, cone angles
- Delete key removes selected light
- Right-click context menu on light for quick type switching

Update tools/MapEditor/MapCanvas.cs:
- Render lights as a small sun/bulb icon with a translucent radius circle
- Color the radius circle to match the light color
- Selected light shows grab handles for radius adjustment
- On hover, show light name tooltip

The editor should serialize lights to JSON alongside existing map data.

==============================================================================