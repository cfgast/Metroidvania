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
