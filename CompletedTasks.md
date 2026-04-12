==============================================================================
Completed Implementation History
==============================================================================
This file contains tasks that have been completed. They are preserved here
for historical reference. See ImplementationPlan.md for current pending tasks.

==============================================================================
Task: Create a GameObject that should be a generic object in game. You should be able to add components to a game object to give it additional functionality. To start we just want to create the basic movement component and rendering component which should be responsible for their indivdual parts.
Implemented: true

==============================================================================
Task: Create a basic map system, It should have defined areas that the player can walk on. It should understand where the player is on the map, and make sure that we draw that portion of the map on screen
Implemented: true

==============================================================================
Task: Create a basic starting map that has multiple platforms defined for the player to talk on
Implemented: true

==============================================================================
Task: Change the movement system so objects fall until they are on a walkable portion of the map, you can still move along those platforms with the controls
Implemented: true

==============================================================================
Task: Make sure the player starts on a walkable portion of the map so they don't fall. if the player falls off the map teleport them back to a valid location.
Implemented: true

==============================================================================
Task: Make is possible for the player to jump using the space bar key.
Implemented: true

==============================================================================
Task: Instead of harddcoding the platforms, make a map file format that describes the map, migrate the current map into the file, and then use the file to load the current map
Implemented: true

==============================================================================
Task: Add a debug menu that opens when I hit the F1 key. It should be a menu containing one button that lets me open a file selector and select a different map to lojad, it should pull open that map and let me use it in game
Implemented: true

==============================================================================
Task: Replace the custom physics implementation (PhysicsComponent) with Nvidia PhysX. Integrate the PhysX SDK, create a PhysXWorld singleton that owns the PxScene, and rewrite PhysicsComponent to create a PxRigidDynamic actor for its owner. Collision shapes should be PxBoxGeometry derived from the existing collisionSize. Gravity, jump impulse, and fall multipliers should produce behaviour identical to the current implementation. The Map should register each Platform as a static PxRigidStatic with a matching PxBoxGeometry. Remove the old hand-rolled AABB resolveCollision path from Map once PhysX owns all collision.
Implemented: true

==============================================================================
Task: Separate input handling from PhysicsComponent into a dedicated InputComponent. InputComponent should read keyboard state each frame and expose a simple InputState struct (moveLeft, moveRight, jump). PhysicsComponent should consume InputState rather than polling sf::Keyboard directly. This separation is required before multiplayer so that remote player input can be injected as an InputState without touching the keyboard.
Implemented: true

==============================================================================
Task: Add an AnimationComponent that plays sprite-sheet animations. Each animation is defined by a name, a path to a texture atlas, a list of frame rects, and a frame duration. AnimationComponent should support play(name), stop(), and looping. Hook it into the player GameObject so that idle, run-left, run-right, jump, and fall each use a distinct animation. The existing RenderComponent should be bypassed when AnimationComponent is present.
Implemented: true

==============================================================================
Task: Add a HealthComponent that tracks current and maximum hit points. Expose takeDamage(float) and heal(float) methods. When HP reaches zero emit an onDeath event (use a simple std::function callback). Add a visible HP bar rendered above the owning GameObject in world space using a RenderComponent-style draw call (no UI system needed yet).
Implemented: true

==============================================================================
Task: Add an EnemyGameObject with a simple patrol AI: it walks back and forth between two waypoints defined in the map file, turns around when it hits a wall or reaches a waypoint, and deals damage on contact with the player. It should use PhysicsComponent (PhysX), HealthComponent, and RenderComponent. Add enemy definitions to the map JSON format so that world_01.json can include a few patrolling enemies.
Implemented: true

==============================================================================
Task: Add a room/zone transition system. A map can contain named Transition zones (axis-aligned rectangles in the map JSON). When the player walks into a transition zone the game fades to black, unloads the current map, loads the target map (referenced by filename in the transition definition), and spawns the player at the target map's matching spawn point. Update world_01.json with at least one transition leading to a second map file.
Implemented: true

==============================================================================
Task: Add a collectible ability pick-up system. Define an Ability enum (e.g. DoubleJump, WallSlide, Dash). Each Ability pick-up is a static GameObject placed in the map JSON. When the player overlaps one, the pick-up is consumed and the corresponding ability is unlocked on the player's PhysicsComponent. Implement DoubleJump first (allow one extra jump while airborne). Abilities persist across room transitions in a PlayerState struct.
Implemented: true

==============================================================================
Task: Add a persistent save/load system. PlayerState (position, current map filename, unlocked abilities, HP) should be serializable to a JSON save file. Add a save slot selection screen accessible from the main menu. Auto-save on room transition. Allow manual save from the pause menu (Escape key).
Implemented: true

==============================================================================
Task: Fix the Map Editor menu bar ordering and sub-menu item spacing. The File menu should appear at the very top of the window (above the toolbar), and several sub-menu items have words mashed together with missing spaces.
Implemented: true

Details:
- File: tools/MapEditor/MainForm.cs
- In MainForm.BuildUI(), the MenuStrip is added to Controls before the ToolStrip. In WinForms, later-added DockStyle.Top controls dock higher, so the toolbar ends up above the menu bar. Fix this so the menu bar is always on top. One approach is to reverse the add order (add toolbar first, then menu strip). Another is to call Controls.SetChildIndex() to force correct Z-order after adding both.
- Audit every ToolStripMenuItem text string in BuildMenu() across the File, Edit, and View menus. Look for missing spaces between words in display text. Fix any items where words are concatenated without spaces. Also check that the \t shortcut separator and & accelerator key markers are placed correctly and do not eat adjacent spaces.
- Audit toolbar button labels and tooltip strings in BuildToolbar() for the same spacing problems.
- After making changes, build and run the editor to confirm: (1) the menu bar is visually above the toolbar, (2) all menu item text reads naturally with proper spacing, and (3) keyboard shortcuts (Ctrl+N, Ctrl+O, Ctrl+S, Del, F, G, S, D, 1, -, =) still work.

==============================================================================
Task: Extend the Map Editor data models to cover every feature in the map JSON format so that opening and re-saving a map does not silently drop enemies, transitions, ability pickups, or named spawn points.
Implemented: true

Details:
- File: tools/MapEditor/Models.cs
- The current MapData class only has Name, Bounds, SpawnPoint, and Platforms. The game engine's map JSON also contains "spawnPoints", "enemies", "transitions", and "abilityPickups" arrays/objects. When the editor opens a map and saves it back, all of these extra fields are silently lost. This task fixes that by adding model classes for every field.
- Add to MapData:
    [JsonPropertyName("spawnPoints")]
    public Dictionary<string, PointData> SpawnPoints { get; set; } = new();
- Add an EnemyData class with these JsonPropertyName-annotated properties:
    "x" (float), "y" (float),
    "waypointA" (PointData), "waypointB" (PointData),
    "speed" (float, default 100), "damage" (float, default 10), "hp" (float, default 50),
    "width" (float, default 40), "height" (float, default 40)
  Add to MapData:
    [JsonPropertyName("enemies")]
    public List<EnemyData> Enemies { get; set; } = new();
- Add a TransitionData class with these JsonPropertyName-annotated properties:
    "name" (string, default ""), "x" (float), "y" (float),
    "width" (float), "height" (float),
    "targetMap" (string), "targetSpawn" (string, default "default")
  Add to MapData:
    [JsonPropertyName("transitions")]
    public List<TransitionData> Transitions { get; set; } = new();
- Add an AbilityPickupData class with these JsonPropertyName-annotated properties:
    "id" (string), "ability" (string — one of "DoubleJump", "WallSlide", "Dash"),
    "x" (float), "y" (float),
    "width" (float, default 30), "height" (float, default 30)
  Add to MapData:
    [JsonPropertyName("abilityPickups")]
    public List<AbilityPickupData> AbilityPickups { get; set; } = new();
- Configure JsonSerializerOptions in MainForm.cs (both read and write paths) to use DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingDefault or equivalent so that empty collections are omitted from the output JSON (the C++ engine treats these arrays as optional).
- Verify round-trip: open maps/world_01.json and maps/world_02.json in the editor, save to a temp location, and diff the output against the original to confirm no data is lost and no spurious empty arrays are added.

==============================================================================
Task: Add named spawn points editing to the Map Editor. Users should be able to view, add, edit, and delete named spawn points, and see them rendered on the canvas.
Implemented: true

Details:
- Files: tools/MapEditor/MainForm.cs, tools/MapEditor/MapCanvas.cs
- Properties panel (MainForm.cs):
  - Add a "SPAWN POINTS" section below the existing map settings area (after the "Apply Map Settings" button and separator).
  - Include a ListBox that displays all named spawn point keys (e.g. "default", "from_world_02").
  - Below the ListBox add text fields for Name (the key string), X, and Y.
  - Add an "Add" button that inserts a new spawn point into MapData.SpawnPoints with the entered name and coordinates, and refreshes the ListBox.
  - Add a "Delete" button that removes the selected spawn point from MapData.SpawnPoints.
  - When a ListBox item is selected, populate the Name/X/Y fields with its values. An "Apply" or "Update" button should write changes back to the dictionary (handling key renames by removing the old key and inserting the new one).
  - All changes should call MarkDirty() and refresh the canvas.
- Canvas rendering (MapCanvas.cs):
  - In the OnPaint method, after drawing the default spawn point, iterate MapData.SpawnPoints and draw each named spawn point as a small diamond marker in a distinct color (e.g. yellow/gold) so it is visually different from the green default spawn diamond.
  - When zoom > 0.3, draw the spawn point name as a label next to the marker.
- Sync: Update SyncMapFields() in MainForm.cs to populate the spawn points ListBox when a map is loaded or created.
- The existing default spawnPoint field should continue to work as before. If a "default" key exists in SpawnPoints, it does not replace spawnPoint — they are independent fields in the JSON.

==============================================================================
Task: Add enemy editing to the Map Editor. Users should be able to view, place, select, move, resize, and delete enemies on the map canvas, and edit their properties in a side panel.
Implemented: true

Details:
- Files: tools/MapEditor/MainForm.cs, tools/MapEditor/MapCanvas.cs
- Selection model refactor (MapCanvas.cs):
  - The canvas currently tracks selection as a single PlatformData? field (_selected / SelectedPlatform). This must be extended to support selecting different entity types. Add an enum SelectableType { None, Platform, Enemy, Transition, Pickup } and track the selected type alongside the selected object. For example, add a SelectedEnemy property (EnemyData?), and set SelectedPlatform to null when an enemy is selected (and vice versa). Fire SelectionChanged for any selection change.
  - Update HitPlatform-style hit testing: add HitEnemy(PointF world) that checks if a world point falls inside any enemy's bounding box (x, y, width, height). In OnMouseDown, after checking platform hits, also check enemy hits (or check enemies first, or check whichever is on top).
- Canvas rendering (MapCanvas.cs):
  - Draw each enemy as a filled rectangle using a red/orange color (e.g. Color.FromArgb(200, 80, 60)) at their (X, Y) position with their (Width, Height).
  - Draw a dashed line from WaypointA to WaypointB to visualize the patrol path.
  - Draw a small circle or marker at each waypoint position.
  - When an enemy is selected, draw a cyan outline and resize handles (same as platforms).
  - When zoom > 0.3, label enemies with "ENEMY" or their index.
- Properties panel (MainForm.cs):
  - Add a "SELECTED ENEMY" section (similar to "SELECTED PLATFORM"). It should be a separate Panel that is enabled/disabled based on whether an enemy is selected.
  - Fields: X, Y (position), WaypointA X, WaypointA Y, WaypointB X, WaypointB Y, Speed, Damage, HP, Width, Height.
  - Include "Apply" and "Delete" buttons.
  - Wire up SelectionChanged to populate these fields when an enemy is selected and clear/disable them when deselected.
- Drawing new enemies:
  - Add EditorTool.DrawEnemy to the EditorTool enum. Add a toolbar button "Draw Enemy" for it.
  - When in DrawEnemy mode, clicking on the canvas should place a new enemy at the clicked position with default values (speed=100, damage=10, hp=50, width=40, height=40). Set waypointA to 100 units left and waypointB to 100 units right of the placement position.
- Moving/resizing enemies should work the same way as platforms (drag to move, handles to resize the bounding box). Update waypoints proportionally if the enemy is moved (shift both waypoints by the same delta).
- The Edit > Delete menu item and Del key should delete whatever is currently selected (platform or enemy).

==============================================================================
Task: Add transition zone editing to the Map Editor. Users should be able to view, place, select, move, resize, and delete transition zones on the map canvas, and edit their properties including target map and spawn point.
Implemented: true

Details:
- Files: tools/MapEditor/MainForm.cs, tools/MapEditor/MapCanvas.cs
- Canvas rendering (MapCanvas.cs):
  - Draw each transition zone as a semi-transparent rectangle (e.g. Color.FromArgb(80, 100, 100, 220) fill) with a dashed border in a blue/purple color. This should be visually distinct from platforms and enemies.
  - When zoom > 0.3, label each transition with its name and target map filename.
  - When a transition is selected, draw a cyan outline and resize handles.
- Hit testing and selection (MapCanvas.cs):
  - Add HitTransition(PointF world) to check if a click lands inside a transition zone.
  - Integrate into the existing OnMouseDown selection logic. Since transitions are overlay zones, check them after platforms and enemies (so platforms are easier to select when overlapping).
- Properties panel (MainForm.cs):
  - Add a "SELECTED TRANSITION" section with fields: Name (text), X, Y, Width, Height, Target Map (text field with a "Browse…" button that opens an OpenFileDialog pre-navigated to the maps/ directory), Target Spawn (text).
  - Include "Apply" and "Delete" buttons.
  - Wire up SelectionChanged to populate/clear these fields.
- Drawing new transitions:
  - Add EditorTool.DrawTransition to the EditorTool enum. Add a toolbar button "Draw Transition".
  - Drawing should work like platform drawing: click and drag to define the rectangle. On mouse-up, create a new TransitionData with default name "new_transition", empty targetMap, and targetSpawn "default".
- Moving/resizing transitions should use the same handle system as platforms.
- Update the Edit > Delete menu item label to be generic (e.g. "Delete Selected" instead of "Delete Platform") since it now applies to multiple entity types.

==============================================================================
Task: Add ability pickup editing to the Map Editor. Users should be able to view, place, select, move, resize, and delete ability pickups on the map canvas, and edit their properties including choosing the ability type from a dropdown.
Implemented: true

Details:
- Files: tools/MapEditor/MainForm.cs, tools/MapEditor/MapCanvas.cs
- Canvas rendering (MapCanvas.cs):
  - Draw each ability pickup as a small filled rectangle or diamond shape in a gold/yellow color (e.g. Color.FromArgb(220, 200, 50)).
  - Add a subtle glow or thicker border to distinguish pickups from other entities.
  - When zoom > 0.3, label each pickup with its ability name (e.g. "DoubleJump").
  - When selected, draw a cyan outline and resize handles.
- Hit testing and selection (MapCanvas.cs):
  - Add HitPickup(PointF world) to check if a click lands inside a pickup's bounding box.
  - Integrate into OnMouseDown selection logic (check after transitions).
- Properties panel (MainForm.cs):
  - Add a "SELECTED ABILITY PICKUP" section with fields: ID (text), Ability (ComboBox dropdown with items: "DoubleJump", "WallSlide", "Dash"), X, Y, Width, Height.
  - Include "Apply" and "Delete" buttons.
  - The ComboBox should be a DropDownList style (no free-form typing) since only the three ability values are valid.
  - Wire up SelectionChanged to populate/clear these fields. Set the ComboBox selected item to match the pickup's ability string.
- Drawing new pickups:
  - Add EditorTool.DrawPickup to the EditorTool enum. Add a toolbar button "Draw Pickup".
  - Clicking on the canvas in DrawPickup mode places a new AbilityPickupData at the click position with default values: id = "pickup_NN" (auto-increment), ability = "DoubleJump", width = 30, height = 30.
- Moving/resizing pickups should use the same handle system as platforms.
- After this task, verify the full workflow end-to-end: open world_02.json (which has an ability pickup and enemies and a transition), verify all entities render correctly, edit some properties, save, and confirm the output JSON matches the expected format.

==============================================================================
Task: In the main menu, add an option to change the resolution, it should be a combo box that lets you switch between multiple good resolutions based on the monitor of the player
Implemented: true

==============================================================================
Task: Add Controller support, so if a controller connects to the game you can use it to control the player.
Implemented: true

==============================================================================
Task: To the main menu add an option to rebind controls to different buttons. It should show the keyboard and controller bindings and allow both to be changed.
Implemented: true

==============================================================================
Task: When a menu is open, add the ability to click on menu items with the mouse. When your back in game the mouse should get hidden again
Implemented: true

==============================================================================
Task: Create a Renderer abstraction layer and an SFML-backed implementation so that all rendering flows through a single interface. This is the foundation that lets us swap in OpenGL later without touching game code.
Implemented: true

Details:
- Create src/Rendering/Renderer.h with a pure-virtual Renderer class. The interface must cover every drawing operation currently performed across the codebase. Required methods (all taking simple POD parameters, no SFML types in the public API):

    // Lifecycle
    virtual void clear(float r, float g, float b, float a = 1.f) = 0;
    virtual void display() = 0;

    // View / camera
    virtual void setView(float centerX, float centerY, float width, float height) = 0;
    virtual void resetView() = 0;   // reset to window-sized identity view
    virtual void getWindowSize(float& w, float& h) const = 0;

    // Primitives
    virtual void drawRect(float x, float y, float w, float h,
                          float r, float g, float b, float a = 1.f) = 0;
    virtual void drawRectOutlined(float x, float y, float w, float h,
                          float fillR, float fillG, float fillB, float fillA,
                          float outR, float outG, float outB, float outA,
                          float outlineThickness) = 0;
    virtual void drawCircle(float cx, float cy, float radius,
                            float r, float g, float b, float a = 1.f,
                            float outR = 0, float outG = 0, float outB = 0, float outA = 0,
                            float outlineThickness = 0.f) = 0;
    virtual void drawRoundedRect(float x, float y, float w, float h,
                                  float radius,
                                  float r, float g, float b, float a = 1.f,
                                  float outR = 0, float outG = 0, float outB = 0, float outA = 0,
                                  float outlineThickness = 0.f) = 0;

    // Textured sprites
    using TextureHandle = uint64_t;
    virtual TextureHandle loadTexture(const std::string& path) = 0;
    virtual void drawSprite(TextureHandle tex, float x, float y,
                            int frameX, int frameY, int frameW, int frameH,
                            float originX, float originY) = 0;

    // Text
    using FontHandle = uint64_t;
    virtual FontHandle loadFont(const std::string& path) = 0;
    virtual void drawText(FontHandle font, const std::string& str,
                          float x, float y, unsigned int size,
                          float r, float g, float b, float a = 1.f) = 0;
    virtual void measureText(FontHandle font, const std::string& str,
                             unsigned int size,
                             float& outWidth, float& outHeight) = 0;

    // Vertex-colored triangles (for combat arc VFX, dash trail, etc.)
    struct Vertex { float x, y, r, g, b, a; };
    virtual void drawTriangleStrip(const std::vector<Vertex>& verts) = 0;
    virtual void drawLines(const std::vector<Vertex>& verts) = 0;

- Create src/Rendering/SFMLRenderer.h and src/Rendering/SFMLRenderer.cpp that implement Renderer by delegating to sf::RenderWindow. This class owns the sf::RenderWindow and wraps every method. For textures and fonts, maintain internal maps of handle → sf::Texture / sf::Font.
- The SFMLRenderer constructor should take window title, width, height, and FPS cap, and create the sf::RenderWindow internally.
- Add SFMLRenderer.cpp to the source list in CMakeLists.txt.
- Do NOT modify any existing game code yet — just create the new files and verify the project still builds.

==============================================================================
Task: Right now in windowed mode when you resize the window, it changes the zoom. Instead of changing the zoom, it should just make it so you can see more of the map.
Implemented: true

==============================================================================
Task: I'd like the menu items to look more modern, right now the rendering of them is very basic and ugly.
Implemented: true

Details:
- Created src/UI/RoundedRectangleShape.h — custom SFML ConvexShape subclass that draws rectangles with rounded corners, used by all menu panels and buttons.
- Created src/UI/UIStyle.h — centralized color palette and drawing helpers (drawMenuItem, drawMenuRow, drawGlow, drawAccentBar) so all menus share the same modern dark theme.
- Updated PauseMenu, SaveSlotScreen, ControlsMenu, and DebugMenu to use rounded-rectangle shapes for panels and buttons, a refined dark color scheme, a subtle glow behind selected items, and a left-side accent bar on the selected row.
- All menus remain fully functional with keyboard, mouse, and controller input.

==============================================================================
Task: There are some problems with Map Transitions at the moment. When I transition between maps, you see popping because it looks like it updates the position of the player after the fade in has occured. Also when you transition from map 2 back to map 1 it takes you to the incorrect location. It goes back to the spawn inside of the from map 2 point.
Implemented: true

==============================================================================
Task: When the application closes and it tries to call m_actor->release() the game crashes, we should fix that, I think the remote actor might clean it up
Implemented: true

==============================================================================
Task: Change the sprite art of the charater too look like a small man running, not just a box with dots
Implemented: true

==============================================================================
Task: Implement the wall slide ability. It should allow you to slowly slide down the side of a wall, and reset your jump when you start sliding. Also add new animations so you look correct when sliding on the wall.
Implemented: true

==============================================================================
Task: Implement the dash ability, it should allow you to dash forward a fixed distance. This ability should be attached to the right shoulder button. You can use the ability on the ground or in the air. Add an animation for the dash forward, and allow have a glowing trail as you dash.
Implemented: true

==============================================================================
Task: Replace SFML input and event types with a custom input abstraction, preparing for the GLFW switch.
Implemented: true

Details:
- Created src/Input/InputTypes.h with KeyCode, MouseButton, GamepadButton, GamepadAxis enums and InputEvent struct.
- Created src/Input/InputSystem.h with abstract InputSystem class and static current() accessor.
- Created src/Input/SFMLInput.h/.cpp implementing InputSystem by wrapping sf::Event, sf::Keyboard, sf::Joystick, sf::Mouse.
- SFMLRenderer creates and owns SFMLInput; Renderer base class exposes getInput().
- Added setWindowSize(), setWindowPosition(), getDesktopSize() to Renderer interface.
- Migrated all event-handling code: InputComponent, MovementComponent, InputBindings, PauseMenu, SaveSlotScreen, ControlsMenu, DebugMenu, main.cpp.
- Replaced sf::Clock with std::chrono-based GameClock in main.cpp.
- No file outside src/Input/SFMLInput.cpp and src/Rendering/SFMLRenderer.cpp includes any SFML header.

==============================================================================
Task: Implement a sword slash ability so the character can attack. It should be attached to the X button. The sword should have an arcing slash that can hit enemies and damage them, for now it should take 3 hits to kill an enemy unit.
Implemented: true

==============================================================================
Task: All of the current enemies on the map should get turned into slimes. They should be animated green blobs and have the same behavior they do now. infrequently they should rapidly jitter and shoot out particles that can damage the player.
Implemented: true

==============================================================================
Task: Migrate the Component base class and all Component subclasses to render through the Renderer interface instead of sf::RenderWindow.
Implemented: true

==============================================================================
Task: Migrate the Map rendering, TransitionManager, and all UI classes to use the Renderer interface instead of sf::RenderWindow.
Implemented: true

==============================================================================
Task: Wire up the SFMLRenderer in main.cpp so the game runs through the Renderer abstraction end to end.
Implemented: true
==============================================================================
Task: Replace all SFML math types (sf::Vector2f, sf::FloatRect, sf::IntRect, sf::Color) with GLM or custom equivalents throughout the entire codebase.
Implemented: true

Details:
- This task eliminates the dependency on SFML headers in game logic files, preparing for full SFML removal.
- Use glm::vec2 for sf::Vector2f. Create a small utility header src/Math/Types.h that provides:
    - A Rect struct: { float x, y, width, height } with an intersects() method and contains(glm::vec2) method to match sf::FloatRect's API.
    - An IntRect struct: { int x, y, width, height } for sprite frame rectangles.

==============================================================================
Task: Implement OpenGL texture loading and sprite rendering in GLRenderer.
Implemented: true

Details:
- Implement loadTexture(): use stb_image to load the image file. Create a GL texture (glGenTextures, glTexImage2D with GL_RGBA). Set filtering to GL_NEAREST (pixel art). Store in an internal map<TextureHandle, GLuint>. Return the handle.

- Create a "textured" shader pair:
    Vertex shader: takes vec2 position + vec2 texcoord, applies projection, passes texcoord to fragment.
    Fragment shader: samples a texture2D uniform and outputs the texel color.

- Implement drawSprite(): given a TextureHandle, position, frame rect (source rectangle within the atlas), and origin:
    1. Bind the textured shader and the GL texture
    2. Compute UV coordinates from the frame rect and texture dimensions
    3. Build 4 vertices (position quad with UV coordinates) for the frame
    4. Upload to a dynamic VBO and draw
    5. The origin offset should shift the quad so the sprite is centered correctly

- For efficiency, use a single dynamic VBO that is re-uploaded each frame (or use a simple sprite batch that collects quads and flushes in one draw call). A sprite batch is preferred for performance — collect all drawSprite calls, sort by texture, and flush at display() or when the texture changes.

- Implement a fallback: if a texture file fails to load, create a 2x2 magenta texture (matching SFML's behavior in AnimationComponent).

- Test by temporarily instantiating GLRenderer and calling loadTexture + drawSprite with assets/player_spritesheet.png to verify a sprite frame renders correctly.
    - A Color struct: { uint8_t r, g, b, a } with a constructor and float accessors.
- Files to update (every file that uses sf::Vector2f, sf::FloatRect, sf::IntRect, or sf::Color):
    - src/Core/GameObject.h: position becomes glm::vec2
    - src/Core/SaveSystem.h/.cpp: SaveData::playerPosition becomes glm::vec2
    - src/Core/InputBindings.h/.cpp: sf::Keyboard::Key stays for now (input, not math)
    - src/Components/PhysicsComponent.h/.cpp: velocity, collisionSize become glm::vec2
    - src/Components/MovementComponent.h: velocity becomes glm::vec2
    - src/Components/EnemyAIComponent.h/.cpp: waypoints become glm::vec2
    - src/Components/RenderComponent.h/.cpp: size/color become glm::vec2/Color
    - src/Components/CombatComponent.cpp: position references become glm::vec2
    - src/Components/SlimeAttackComponent.h/.cpp: particle position/velocity become glm::vec2
    - src/Map/Platform.h: bounds becomes Rect, color becomes Color
    - src/Map/Map.h/.cpp: spawn point, bounds, collision checks use glm::vec2 and Rect
    - src/Map/MapLoader.cpp: construct Rect/Color/glm::vec2 instead of SFML types
    - src/Map/TransitionZone.h: bounds becomes Rect
    - src/Map/EnemyDefinition.h: position, waypoints, size become glm::vec2
    - src/Map/AbilityPickupDefinition.h: position, size become glm::vec2
    - src/Physics/PhysXWorld.h/.cpp: parameter types become glm::vec2
    - src/main.cpp: all sf::Vector2f usage becomes glm::vec2
- Add `#include <glm/vec2.hpp>` where needed. GLM should already be available from CMakeLists.txt (added in a later task, but this task should also add GLM to CMakeLists.txt via FetchContent if not already present).
- Add GLM to CMakeLists.txt:
    FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(glm)
    target_link_libraries(Metroidvania PRIVATE glm::glm)
- After this task, no file outside src/Rendering/SFMLRenderer.cpp should reference sf::Vector2f, sf::FloatRect, sf::IntRect, or sf::Color. The SFMLRenderer converts between glm::vec2/Rect and SFML types internally.
- The game should still build and run identically.

==============================================================================
Task: Add GLFW, GLAD, stb_image, and FreeType as project dependencies via CMake. Verify they compile and link.
Implemented: true

Details:
- Files modified: CMakeLists.txt
- Added GLFW 3.4 via FetchContent (docs/tests/examples disabled)
- Added GLAD (OpenGL 3.3 Core, generated via glad2) vendored into third_party/glad/
  - third_party/glad/include/glad/gl.h, third_party/glad/include/KHR/khrplatform.h
  - third_party/glad/src/gl.c added to executable sources
- Added stb_image vendored into third_party/stb/
  - third_party/stb/stb_image.h (header-only library)
  - third_party/stb/stb_image_impl.cpp (defines STB_IMAGE_IMPLEMENTATION)
- Added FreeType 2.13.3 via FetchContent
- Linked glfw, freetype, and opengl32 alongside existing SFML links
- All existing SFML dependencies kept intact for transition period
- Project builds and links successfully with all new dependencies

==============================================================================
Task: Implement the OpenGL renderer core-- GLRenderer with shader management, orthographic projection, and rectangle drawing.
Implemented: true

Details:
- Created src/Rendering/GLRenderer.h and src/Rendering/GLRenderer.cpp implementing the Renderer interface.
- GLRenderer constructor: calls glfwInit(), creates a GLFWwindow with OpenGL 3.3 Core profile, initializes GLAD, enables alpha blending, sets up viewport and framebuffer-size callback.
- Created src/Rendering/Shader.h and src/Rendering/Shader.cpp: compiles vertex + fragment GLSL source strings, provides use(), setMat4(), setVec4(), setInt() uniform helpers with error logging.
- Implemented flat-color shader pair (vertex: vec2 pos + mat4 projection + mat4 model; fragment: uniform vec4 color).
- Implemented clear() via glClearColor + glClear, display() via glfwSwapBuffers.
- Implemented setView() building glm::ortho from center/size (top-left origin), resetView() with identity ortho.
- Implemented getWindowSize() returning stored dimensions updated by resize callback.
- Implemented drawRect() and drawRectOutlined() using a persistent unit-quad VAO with model-transform (translate + scale) and the flat shader. Outline drawn as 4 thin quads.
- All other Renderer methods (circle, rounded-rect, texture, text, vertex-colored geometry) are stubbed for later tasks.
- Added GLRenderer.cpp and Shader.cpp to CMakeLists.txt sources.
- Verified the project compiles successfully.

==============================================================================
Task: Implement OpenGL text rendering in GLRenderer using FreeType.
Implemented: true

Details:
- Implement loadFont(): use FreeType to load the font file (e.g. C:\Windows\Fonts\arial.ttf). For each font handle, store the FT_Face.

- For rendering, use the standard glyph-atlas approach:
    1. On first use of a font at a given size, rasterize all printable ASCII glyphs (32-126) into a single texture atlas. Store glyph metrics (advance, bearing, size, UV rect in atlas) in a lookup table.
    2. Create a GL texture from the atlas bitmap (single-channel, use GL_RED format with a swizzle or a dedicated text shader that reads the red channel as alpha).

- Create a "text" shader pair:
    Vertex shader: vec2 position + vec2 texcoord, applies projection.
    Fragment shader: samples texture red channel as alpha, multiplies by a uniform vec4 textColor. This produces colored text with transparent background.

- Implement drawText(): for each character in the string, look up the glyph metrics, compute the quad position (applying bearing offsets and advance), add vertices to a batch, then draw all quads in one call with the glyph atlas bound.

- Implement measureText(): iterate the string, sum advance values for width. Height is the line height from font metrics. This replaces sf::Text::getLocalBounds().

- Handle newlines in the string by advancing the Y cursor.

- Cache glyph atlases per (font, size) pair to avoid regenerating every frame.

==============================================================================
Task: Implement OpenGL circle, rounded rectangle, triangle strip, and line drawing in GLRenderer.
Implemented: true

Details:
- Implement drawCircle():
    - Generate vertices for a triangle fan with ~32 segments (enough for smooth circles at game scale).
    - Use the flat color shader with the projection matrix.
    - For outlines: draw a second ring of triangles (outer_radius to outer_radius+thickness) in the outline color, or draw the outline as a GL_TRIANGLE_STRIP ring.
    - Reuse a single VBO, upload vertices each call.

- Implement drawRoundedRect():
    - Generate vertices for a rounded rectangle: 4 corner arcs (each ~8 segments) connected by straight edges, all as a triangle fan from the center.
    - Use the flat color shader.
    - For outlines: generate an outline strip similar to the circle approach.
    - This replaces the custom RoundedRectangleShape.h SFML shape used throughout the UI.

- Implement drawTriangleStrip():
    - Accept a vector<Renderer::Vertex> with per-vertex position and color.
    - Upload to a dynamic VBO, draw with GL_TRIANGLE_STRIP using a per-vertex-color shader.
    - Create a "vertex color" shader pair: vertex shader takes vec2 pos + vec4 color, passes color to fragment, fragment outputs interpolated color.
    - This is used by CombatComponent for the sword arc VFX.

- Implement drawLines():
    - Same vertex format as triangle strip but drawn with GL_LINES.
    - Used by CombatComponent for the bright leading-edge line.

- After this task, GLRenderer should implement every method in the Renderer interface.

==============================================================================
Task: Implement GLFWInput (the GLFW-backed InputSystem) so the game can receive input without SFML.
Implemented: true

Details:
- Create src/Input/GLFWInput.h and src/Input/GLFWInput.cpp implementing the InputSystem interface using GLFW callbacks.

- GLFW uses callbacks for input, not polling-then-queue like SFML. The implementation should:
    1. Register GLFW callbacks: glfwSetKeyCallback, glfwSetMouseButtonCallback, glfwSetCursorPosCallback, glfwSetWindowSizeCallback, glfwSetWindowCloseCallback, glfwSetJoystickCallback.
    2. Each callback pushes an InputEvent into an internal std::queue<InputEvent>.
    3. pollEvent() pops from the queue. Before popping, call glfwPollEvents() if the queue is empty.

- Implement isKeyPressed(): use glfwGetKey(window, glfwKeyCode). Create a mapping function from KeyCode enum -> GLFW key constant.
- Implement isGamepadConnected(): use glfwJoystickPresent().
- Implement getGamepadAxis(): use glfwGetGamepadState() (GLFW 3.3+ gamepad API) or glfwGetJoystickAxes().
- Implement isGamepadButtonPressed(): use glfwGetGamepadState() or glfwGetJoystickButtons().
- Implement setMouseCursorVisible(): use glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL / GLFW_CURSOR_HIDDEN).

- Map GLFW key codes to the KeyCode enum. GLFW uses GLFW_KEY_A, GLFW_KEY_SPACE, etc. Create a bidirectional mapping table.

- Map GLFW gamepad buttons to GamepadButton enum. GLFW uses GLFW_GAMEPAD_BUTTON_A, etc.

- Map GLFW gamepad axes to GamepadAxis enum. GLFW_GAMEPAD_AXIS_LEFT_X, etc.

- The GLRenderer should create and own a GLFWInput instance (since it owns the GLFWwindow). Pass the GLFWwindow* to GLFWInput's constructor.

- Wire up the WindowResized callback to also update GLRenderer's internal window size and glViewport.
- Wire up WindowClosed to set a flag that GLRenderer::isOpen() checks.

- Add GLFWInput.cpp to CMakeLists.txt sources.
- Do NOT switch to GLFWInput yet - just verify it compiles.

==============================================================================
Task: Switch the game from SFMLRenderer to GLRenderer and from SFMLInput to GLFWInput. Remove all SFML dependencies.
Implemented: true

Details:
- Files to modify: src/main.cpp, CMakeLists.txt

- In main.cpp:
    1. Replace `#include "Rendering/SFMLRenderer.h"` with `#include "Rendering/GLRenderer.h"`.
    2. Replace `SFMLRenderer renderer(...)` with `GLRenderer renderer("Metroidvania", 800, 600, 60)`.
    3. Replace any remaining sf::Clock usage with a GLFW-based timer: use glfwGetTime() to compute dt. Create a small helper or inline the logic. Alternatively, add a getTime() method to Renderer that wraps glfwGetTime().
    4. The rest of main.cpp should work unchanged since it already goes through the Renderer and InputSystem abstractions.

- In CMakeLists.txt:
    1. Remove the SFML FetchContent block entirely (FetchContent_Declare for SFML, FetchContent_MakeAvailable).
    2. Remove sfml-graphics, sfml-window, sfml-system from target_link_libraries.
    3. Remove the SFML DLL copy post-build command.
    4. Remove SFMLRenderer.cpp and SFMLInput.cpp from the source list.
    5. Delete src/Rendering/SFMLRenderer.h, src/Rendering/SFMLRenderer.cpp, src/Input/SFMLInput.h, src/Input/SFMLInput.cpp.
    6. Verify no file in the project includes any SFML header. Run a search for `#include.*SFML` to confirm.

- The file dialog in DebugMenu uses Windows API (comdlg32) directly — this should still work without SFML.

- Update the sf::VideoMode::getDesktopMode() calls in SaveSlotScreen to use GLFW:
    - glfwGetPrimaryMonitor() + glfwGetVideoMode() to get desktop resolution.
    - Window resize: glfwSetWindowSize() instead of window.setSize().
    - Window reposition: glfwSetWindowPos() instead of window.setPosition().

- Build and run the game end-to-end. Verify:
    1. Window opens at 800x600 with "Metroidvania" title
    2. Save slot screen renders correctly with text, rounded rects, glow effects
    3. Game loads and renders platforms, player sprite, enemies with animations
    4. HP bars render above entities
    5. Combat arc VFX renders correctly
    6. Slime particles render as circles
    7. Dash ghost trail renders
    8. Room transitions fade to black and back
    9. Pause menu renders with rounded rect styling
    10. Controls menu renders and rebinding works
    11. Debug menu F1 works with file dialog
    12. Controller input works
    13. Window resize shows more of the map (not zoom)
    14. Mouse cursor shows/hides appropriately

==============================================================================
Task: Clean up the codebase post-migration. Remove dead SFML code, update documentation, and verify the build is clean.
Implemented: true

Details:
- Search the entire src/ directory for any remaining references to SFML. Remove any leftover includes, comments referencing SFML types, or dead code paths.
- Update ReadMe.md:
    - Change "Game Renderer: SFML" to "Game Renderer: OpenGL 3.3 Core (via GLFW + GLAD)"
    - Update the project description to reflect the new rendering stack
- Update architecture.md:
    - Replace all SFML rendering references with OpenGL/GLFW equivalents
    - Document the new src/Rendering/ directory (Renderer, GLRenderer, Shader)
    - Document the new src/Input/ directory (InputSystem, GLFWInput, InputTypes)
    - Document the new src/Math/Types.h
    - Update the dependency table (remove SFML, add GLFW, GLAD, GLM, stb_image, FreeType)
    - Update the main loop description to reflect GLFW event handling
- Update docs/GameObjects.md if it references SFML types in component descriptions.
- Verify a clean build from scratch: delete the build directory, reconfigure with CMake, build, and run.
- Run the game and perform a final smoke test of all features listed in the previous task's verification checklist.
- Commit with a descriptive message summarizing the SFML to OpenGL migration.

Completed:
- Removed SFML references from src/Input/InputTypes.h, src/Input/InputSystem.h, src/Rendering/GLRenderer.cpp, src/Rendering/Renderer.h, src/Math/Types.h, src/main.cpp
- Updated docs/GameObjects.md: replaced sf::Vector2f, sf::RectangleShape, sf::Color, sf::RenderWindow with current types (glm::vec2, Renderer&, float RGBA)
- Updated docs/Map.md: replaced sf::FloatRect, sf::Color, sf::View with Rect, Color, Renderer::setView()
- Updated architecture.md: removed SFML migration-status language and transition references
- ReadMe.md was already up-to-date from prior tasks
- Verified clean build with zero errors
==============================================================================
Task: Add WorldData model and world file serialization to the map editor.
Implemented: true

Details:
- Added WorldMapEntry and WorldData model classes to Models.cs with
  JsonPropertyName attributes matching existing style
- Added File menu items: New World (Ctrl+Shift+N), Open World (Ctrl+Shift+O),
  Save World (Ctrl+Shift+S), Save World As
- Implemented world file I/O: LoadWorldFile() deserializes WorldData and loads
  referenced map files with paths relative to world file directory;
  WriteWorldFile() serializes WorldData and saves modified maps
- Opening a single map file auto-creates a single-map world at offset (0,0)
- World files use .world.json extension
- Updated title bar to show world file name when applicable
- Updated architecture.md with world JSON schema documentation
==============================================================================
==============================================================================
Task: Refactor MapCanvas to support a multi-map document model.
Implemented: true

Details:
- Added EditorMap class to Models.cs wrapping MapData with WorldX, WorldY,
  FilePath, and IsDirty properties
- Replaced BackgroundMap class with EditorMap (BackgroundMap removed)
- Refactored MapCanvas to hold List<EditorMap> and ActiveMap instead of a
  single MapData and separate BackgroundMap list
- Added LoadWorld(List<EditorMap>) method; LoadMap(MapData) convenience
  overload wraps into a single-entry EditorMap list
- Updated OnPaint to render ALL EditorMaps: inactive maps dimmed with dotted
  borders, active map at full opacity with cyan 2px highlighted border
- Render order: inactive maps first (back), active map last (front)
- All editing tools (select, draw, resize) continue to operate only on
  ActiveMap's data
- FitToView() computes bounding box of ALL loaded maps using world offsets
- Single-map workflow preserved: opening a single map creates a one-entry list
- Updated MainForm: LoadWorldFile now loads all maps as EditorMaps,
  WriteWorldFile saves all maps from EditorMaps, removed neighbor map
  toggle/load features (superseded by multi-map model)
- Updated architecture.md with multi-map document model documentation
==============================================================================
==============================================================================
Task: Implement map focus/activation by clicking.
Implemented: true

Details:
- In MapCanvas, when the user clicks (left mouse, Select tool) on empty
  space or on an element that belongs to a non-active map, switch the
  ActiveMap to that map.

- Add a method: EditorMap HitMap(PointF worldPoint)
    - Returns the topmost EditorMap whose world-offset bounds contain
      the given world point.
    - If the point is inside the active map's bounds, prefer the active
      map (don't switch away accidentally).
    - If multiple inactive maps overlap at the point, return the one
      rendered on top (last in list).

- Update OnMouseDown (Select tool):
    1. First, try hit-testing the ActiveMap's elements (handles,
       waypoints, spawns, platforms, enemies, transitions, pickups)
       as before.
    2. If nothing is hit on the active map, call HitMap() to check
       if the click is inside another map's bounds.
    3. If another map is found, switch ActiveMap to it, clear selection,
       fire SelectionChanged event, and repaint.
    4. If no map is hit, just clear selection as before.

- When ActiveMap changes:
    - Clear all selections
    - Fire SelectionChanged so the properties panel refreshes
    - Repaint canvas to update active/inactive highlighting
    - Update MainForm status bar to show active map name

- In MainForm, update the status bar left text to include the active
  map's name (e.g., "Active: World 1 - Starting Area | 18 platforms").

- Update the Map Settings section of the properties panel to show the
  active map's name and bounds (it should already work if it reads
  from ActiveMap.Map, but verify).

==============================================================================
Task: Add a Move Map tool for repositioning maps in world space.
Implemented: true

Details:
- Add a new EditorTool enum value: EditorTool.MoveMap

- In MainForm, add a "Move Map" toolbar button with hotkey [M]:
    - Icon/label: "Move Map [M]"
    - Positioned after the existing tool buttons
    - Add keyboard shortcut handler for 'M' key

- In MapCanvas, implement the MoveMap tool:
    OnMouseDown (MoveMap tool, left click):
      - Call HitMap() to find which map is under the cursor
      - If a map is found, start dragging:
        - Store the hit map as _draggingMap
        - Store the mouse offset from the map's world origin
        - Switch ActiveMap to the dragged map
        - Set cursor to SizeAll

    OnMouseMove (if _draggingMap != null):
      - Compute new world position from mouse position minus stored offset
      - If grid snap is enabled, snap the position to 10-unit grid
      - Update _draggingMap.WorldX and _draggingMap.WorldY
      - Mark the world as dirty
      - Repaint canvas

    OnMouseUp:
      - Clear _draggingMap
      - Reset cursor
      - Trigger auto-transition recalculation (for now, just mark a
        flag _transitionsNeedRegen = true; Task 5 will implement the
        actual recalculation)

- Visual feedback while dragging:
    - Draw the map bounds as a dashed outline during drag
    - Show coordinate tooltip near cursor: "(X, Y)"

- The map's internal coordinates do NOT change — only WorldX/WorldY

==============================================================================
Task: Implement relative positioning in the C++ game runtime transition callback.
Implemented: true

Details:
- In src/main.cpp, update the TransitionManager load callback (the lambda
  passed to transitionMgr.setLoadCallback). Currently it does:

    glm::vec2 spawn = map.getNamedSpawn(targetSpawn);
    physics->teleport(spawn);

  Change this to also receive the TransitionZone data. The cleanest
  approach: store the triggering zone's data before starting the
  transition, then use it in the callback.

  Before the transition starts (where checkTransition is called):
    - When a transition is detected, save the zone data and the
      player's current position:
        pendingZone = *zone;  // copy the TransitionZone
        pendingPlayerPos = player.position;

  In the load callback:
    - After loading the new map, compute the target position:

      glm::vec2 spawn;
      if (!pendingZone.edgeAxis.empty())
      {
          // Relative positioning mode
          if (pendingZone.edgeAxis == "vertical")
          {
              // Shared edge is vertical (left/right adjacency)
              // X is fixed (entry side), Y is relative
              spawn.x = pendingZone.targetBaseX;
              spawn.y = pendingZone.targetBaseY
                      + (pendingPlayerPos.y - pendingZone.bounds.y);
          }
          else // "horizontal"
          {
              // Shared edge is horizontal (top/bottom adjacency)
              // Y is fixed (entry side), X is relative
              spawn.x = pendingZone.targetBaseX
                      + (pendingPlayerPos.x - pendingZone.bounds.x);
              spawn.y = pendingZone.targetBaseY;
          }
      }
      else
      {
          // Legacy mode: use fixed spawn point
          spawn = map.getNamedSpawn(targetSpawn);
      }

      physics->teleport(spawn);

  Clamp the relative position to stay within the target zone's range
  so the player can't appear outside map bounds:
    - For vertical edge: clamp spawn.y to [targetBaseY, targetBaseY + zone.bounds.height - playerSize.y]
    - For horizontal edge: clamp spawn.x to [targetBaseX, targetBaseX + zone.bounds.width - playerSize.x]

- Also consider: the TransitionManager currently only passes
  (targetMap, targetSpawn) to the callback. You may need to either:
  (a) Store pendingZone/pendingPlayerPos as variables captured by the
      lambda in main.cpp (simplest, since they're in main scope), or
  (b) Extend TransitionManager to store and pass the zone data.
  Option (a) is recommended for minimal changes.

- Build the C++ game and test by running it:
    - Load world_01, walk to the right edge transition to world_02
    - Verify the player appears at the corresponding Y position on
      the other side (not at a fixed spawn point)
    - Walk back from world_02 to world_01 and verify the same behavior
    - Test vertical transitions if any exist
  (the offset in the world file) is modified.

==============================================================================

==============================================================================
Task: Implement adjacency detection and auto-transition generation engine.
Implemented: true

Details:
- Created TransitionGenerator.cs in tools/MapEditor/ with a static
  TransitionGenerator class implementing:
  - RegenerateTransitions(List<EditorMap>): clears all transitions and
    auto-generated spawn points (prefixed "from_"), then detects edge
    adjacency between all map pairs and generates transition zones and
    spawn points for each shared edge
  - Edge adjacency detection for all four orientations (A-right to B-left,
    A-left to B-right, A-bottom to B-top, A-top to B-bottom) with 0.5-unit
    tolerance and overlap segment validation
  - Horizontal transitions: 50-unit wide zones extending inward from the
    shared edge, spanning the overlap height; spawn points 60 units inward
  - Vertical transitions: 50-unit tall zones extending inward from the
    shared edge, spanning the overlap width; spawn points 60 units inward
  - Transition naming: "to_<target_filename>", spawn naming: "from_<source>"
- Added RegenerateTransitions() method to MapCanvas that calls the static
  generator when 2+ maps are loaded and repaints
- Hooked regeneration into: MoveMap mouse-up, LoadWorld, and
  MainForm.ApplyMapSettings (bounds change)
- Auto-transitions render using existing drawing code (semi-transparent
  blue rectangles with dashed borders and target map labels)

==============================================================================
==============================================================================
Task: Remove manual transition tool and integrate auto-transitions with save.
Implemented: true

Details:
- Removed manual transition drawing tool:
    - Removed EditorTool.DrawTransition enum value
    - Removed SelectableType.Transition enum value
    - Removed the "Draw Transition [T]" toolbar button
    - Removed the 'T' keyboard shortcut
    - Removed transition-specific hit-testing from Select tool
    - Removed the Transition properties panel from MainForm
    - Removed the "Delete" functionality for transitions
    - Removed SelectTransition, HitTransition, ApplyResizeTransition methods
    - Removed ApplyTransitionSettings and BrowseTargetMap methods
    - Removed all _selectedTransition field references

- Auto-transitions displayed but NOT editable:
    - Rendered in OnPaint as before (blue semi-transparent rects)
    - Added "⚡ [auto]" label to indicate auto-generated transitions
    - Transitions cannot be selected, moved, or deleted by the user

- Updated SaveFile() / SaveWorld():
    - RegenerateTransitions() called before every save
    - NormalizeTransitionPaths() converts absolute targetMap paths to
      relative paths (e.g., "maps/world_02.json") for the game runtime
    - Auto-generated spawn points preserved alongside user-created ones

- Updated LoadWorld():
    - RegenerateTransitions() called after loading all maps (already
      handled in MapCanvas.LoadWorld)
    - Old manually-placed transitions replaced by auto-generated ones

- Single map files (not world) don't generate transitions:
    - RegenerateTransitions guard requires >= 2 maps
==============================================================================

==============================================================================
Task: Add world management UI - add, create, and remove maps from the world.
Implemented: true

==============================================================================
Task 2: Normal-map-aware sprite and flat-color shaders
==============================================================================
Implemented: true

Modify the existing shaders to accept light data and optional normal maps,
performing per-pixel lighting calculations.

Shader changes (all embedded in GLRenderer.cpp):

1. Flat shader (kFlatVS/kFlatFS):
   - VS outputs vWorldPos (vec2) = (uModel * vec4(aPos,0,1)).xy
   - FS receives uniform Light array (max 32 lights), uAmbientColor (vec3),
     uNumLights (int)
   - For each light: compute distance, attenuation (quadratic falloff to
     radius), diffuse contribution using a default flat normal (0, 0, 1)
   - Final color = uColor * (ambient + sum of light contributions)
   - Spot light: add angular falloff using direction + cone angles

2. Texture shader (kTexVS/kTexFS):
   - VS outputs vWorldPos
   - FS adds uniform sampler2D uNormalMap, uniform bool uHasNormalMap,
     same Light array uniforms as flat shader
   - Sample normal from uNormalMap (if present), remap from [0,1] to [-1,1]
   - Same per-pixel lighting as flat shader but using sampled normal
   - When uHasNormalMap is false, use default (0, 0, 1) normal
   - Flip normal Y for Y-down coordinate system

3. Light struct in GLSL:
   struct Light {
       vec2  position;    // world-space XY
       vec3  color;       // RGB
       float intensity;
       float radius;      // max influence distance
       float z;           // height above 2D plane
       vec2  direction;   // for spot lights (normalized)
       float innerCone;   // cos(inner angle) -- full intensity
       float outerCone;   // cos(outer angle) -- falloff to zero
       int   type;        // 0 = point, 1 = spot
   };

GLRenderer changes:
- When drawing sprites: bind normal map texture to unit 1 if available,
  set uHasNormalMap accordingly
- Upload light array uniforms before each draw call (or use a UBO)
- Add loadNormalMap() helper that tries to load "path_n.png" for a given
  "path.png" texture. Cache result (including "not found" to avoid retrying)

==============================================================================
Task 3: Lighting compositing pass
==============================================================================
Implemented: true

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
- Ambient color defaults to (0.8, 0.8, 0.9) -- bright blueish white

Update Renderer.h:
- Add virtual setAmbientColor(float r, float g, float b)

No map/editor changes yet -- test with a hardcoded light at (0, 0) to verify
shaders compile and lighting math works.
==============================================================================

Details:
- Added a Maps panel to MainForm (left sidebar, below the dynamic
  property panels):
    - Owner-drawn ListBox showing all maps in the current world
    - Each entry shows: map name (from MapData.Name), file path, and
      a dirty asterisk (*) if the map has unsaved changes
    - Active map is highlighted with bold cyan text
    - Clicking a map entry sets it as the ActiveMap in the canvas

- Added toolbar buttons (Add… / New / Remove) and File menu items:
    "Add Map to World…":
      - Opens a file dialog to select an existing .json map file
      - Loads the map and adds it to the world
      - Default position: to the right of the rightmost existing map
        (or at origin if world is empty)
      - Calls RegenerateTransitions() after adding
      - Calls FitToView() to show the new layout

    "New Map in World…":
      - Creates a blank map (default bounds, one floor platform)
      - Prompts for a file name/path to save it
      - Adds to world at default position (right of rightmost map)
      - Calls RegenerateTransitions()

    "Remove Map from World":
      - Removes the active map from the world (does NOT delete the
        file from disk)
      - Prompts for confirmation if map has unsaved changes
      - Switches ActiveMap to next available map
      - Calls RegenerateTransitions()

- Updated the title bar:
    - Shows world file name (e.g., "my_world.world.json")
    - Shows asterisk (*) if any map or the world has unsaved changes

- Updated dirty tracking:
    - World is dirty if any EditorMap.IsDirty is true OR if the world
      structure changed (maps added/removed/repositioned)
    - ConfirmDiscard() checks world-level dirty state
    - MarkDirty() sets IsDirty on the active EditorMap
    - WriteWorldFile clears all dirty flags after successful save

- FitToView (F key) already encompasses all maps with 12% margin

- Keyboard shortcuts remain unchanged:
    - M: Move Map tool
    - Ctrl+Shift+N/O/S: World New/Open/Save
    - All existing shortcuts remain for single-map editing
==============================================================================
==============================================================================
Task: Add relative positioning fields to TransitionZone and update serialization in both C++ and C# codebases.
Implemented: true

Details:
- C++ changes (src/Map/TransitionZone.h):
    Added edgeAxis, targetBaseX, targetBaseY fields to TransitionZone struct.
- C++ changes (src/Map/MapLoader.cpp):
    Updated transition loading to read the new fields with defaults.
- C# changes (tools/MapEditor/Models.cs):
    Added EdgeAxis, TargetBaseX, TargetBaseY properties to TransitionData.
- C# changes (tools/MapEditor/TransitionGenerator.cs):
    Updated GenerateHorizontal/GenerateVertical to set edgeAxis and targetBase
    fields. Removed from_* spawn point generation (cleanup code remains).
    Set targetSpawn to "default" as fallback.
==============================================================================

==============================================================================
Task: Remove legacy from_* spawn points from existing map files and update editor to regenerate clean transitions.
Implemented: true

Details:
- Update the three existing map files to remove auto-generated spawn
  points and old transition data. The editor will regenerate clean
  transitions with the new relative positioning fields on next save.

  maps/world_01.json:
    - Remove spawn points: "from_world_02", "from_world_03"
    - Keep: "default" spawn point
    - Remove all entries from the "transitions" array (editor will
      regenerate them with edgeAxis/targetBase fields)

  maps/world_02.json:
    - Remove spawn points: "from_world_01"
    - Keep: "default" spawn point
    - Remove all entries from the "transitions" array

  maps/world_03.json:
    - Remove spawn points: "from_world_03" (this was malformed anyway)
    - Keep no spawn points if none remain (or keep "default" if it exists)
    - Remove all entries from the "transitions" array

- After editing the JSON files, open them in the map editor as a world,
  verify the maps load correctly, and save the world. This should
  regenerate the transitions with the new edgeAxis/targetBase fields
  and without from_* spawn points.

  If the editor is not runnable in this environment, manually add the
  correct transition entries to each map file based on the adjacency:

  world_01 right edge <-> world_02 left edge:
    In world_01.json add transition:
      name: "to_world_02", edgeAxis: "vertical",
      x/y/w/h: positioned at world_01's right edge spanning the overlap,
      targetMap: "maps/world_02.json",
      targetBaseX: 60 (60 units inside world_02's left edge),
      targetBaseY: top of overlap in world_02 local coords

    In world_02.json add matching transition:
      name: "to_world_01", edgeAxis: "vertical",
      x/y/w/h: positioned at world_02's left edge spanning the overlap,
      targetMap: "maps/world_01.json",
      targetBaseX: world_01 right edge - 60 (60 units inside from right),
      targetBaseY: top of overlap in world_01 local coords

  (Repeat for world_01 <-> world_03 if they share an edge)

- Update architecture.md:
    - In the Map JSON schema section, add the new transition fields
      (edgeAxis, targetBaseX, targetBaseY) with descriptions
    - Note that auto-transitions use relative positioning and manual
      transitions can still use targetSpawn for fixed landing points

- Build the C++ game and verify transitions still work correctly with
  the updated map files.
==============================================================================

==============================================================================
Task 1: Framebuffer infrastructure and light data structures
==============================================================================
Implemented: true

Add the FBO render target and light data types that the rest of the system
will build on. No visual change yet -- scene still renders identically but
now goes through the FBO path.

Files to create / modify:
  src/Rendering/Light.h          -- NEW: LightType enum (Point, Spot),
                                    Light struct (position, color, intensity,
                                    radius, z, direction, innerCone, outerCone)
  src/Rendering/Renderer.h       -- Add virtual addLight(), clearLights(),
                                    beginFrame(), endFrame() methods
  src/Rendering/GLRenderer.h     -- Add FBO members (m_fbo, m_fboColorTex,
                                    m_fboDepthRb), m_lights vector,
                                    light-map texture, fullscreen quad shader
  src/Rendering/GLRenderer.cpp   -- Implement FBO creation/resize in
                                    constructor and handleWindowResize(),
                                    beginFrame() binds FBO, endFrame() resolves
                                    to default FB with a fullscreen blit,
                                    addLight/clearLights store into m_lights

Details:
- FBO size matches window size; recreate on resize
- beginFrame(): bind FBO, clear it (same clear color)
- endFrame(): bind default FB, draw fullscreen textured quad from FBO color
  attachment (simple passthrough for now, no lighting applied yet)
- display() calls endFrame() internally if not already called
- The fullscreen blit uses a trivial passthrough shader (sample texture,
  output as-is) -- lighting compositing comes in Task 3

Update main.cpp:
- Add renderer.beginFrame() before rendering
- Add renderer.endFrame() after world rendering, before UI
- Call renderer.clearLights() at frame start

Test: game should look identical to before (passthrough FBO blit).

==============================================================================

==============================================================================
Task 4: Map light definitions and MapLoader integration
==============================================================================
Implemented: true

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

==============================================================================
Task 5: LightComponent for player dynamic light
==============================================================================
Implemented: true

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

==============================================================================
Task 6: Map editor - Place Light tool
==============================================================================
Implemented: true

Add light placement and editing to the C# WinForms map editor.

Update tools/MapEditor/Models.cs:
- Add LightData class:
    public class LightData {
        [JsonPropertyName(""name"")]       public string Name       { get; set; }
        [JsonPropertyName(""type"")]       public string Type       { get; set; } = ""point"";
        [JsonPropertyName(""x"")]          public float  X          { get; set; }
        [JsonPropertyName(""y"")]          public float  Y          { get; set; }
        [JsonPropertyName(""z"")]          public float  Z          { get; set; } = 80f;
        [JsonPropertyName(""r"")]          public float  R          { get; set; } = 1f;
        [JsonPropertyName(""g"")]          public float  G          { get; set; } = 1f;
        [JsonPropertyName(""b"")]          public float  B          { get; set; } = 1f;
        [JsonPropertyName(""intensity"")]  public float  Intensity  { get; set; } = 1f;
        [JsonPropertyName(""radius"")]     public float  Radius     { get; set; } = 200f;
        // Spot-only:
        [JsonPropertyName(""directionX"")] public float  DirectionX { get; set; } = 0f;
        [JsonPropertyName(""directionY"")] public float  DirectionY { get; set; } = 1f;
        [JsonPropertyName(""innerCone"")]  public float  InnerCone  { get; set; } = 30f;
        [JsonPropertyName(""outerCone"")]  public float  OuterCone  { get; set; } = 45f;
    }
- Add to MapData: public List<LightData> Lights { get; set; } = new();

Update tools/MapEditor/MainForm.cs:
- Add ""Place Light"" tool button to the toolbar
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
