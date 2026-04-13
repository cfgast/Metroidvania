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

==============================================================================
Task 1: Add Vulkan dependencies to CMake
==============================================================================
Implemented: true

Add find_package(Vulkan REQUIRED) to CMakeLists.txt. Vendor VMA (Vulkan
Memory Allocator) and vk-bootstrap via FetchContent. Add a CMake function
that compiles .glsl files to .spv using Vulkan::glslc (the glslc compiler
bundled with the Vulkan SDK). Define GLM_FORCE_DEPTH_ZERO_TO_ONE and
GLM_FORCE_RADIANS as compile definitions.

Keep all existing OpenGL dependencies (GLAD, GLFW, opengl32) intact — they
will be removed in a later task after the migration is complete.

The project should still compile and run with the OpenGL backend after this
task. No rendering code changes — build system only.

==============================================================================

==============================================================================
Task 2:Create VulkanRenderer skeleton
==============================================================================
Implemented: true

Create src/Rendering/VulkanRenderer.h and VulkanRenderer.cpp implementing
every pure-virtual method from Renderer.h as a stub (no-op or minimal
return value). The class should compile cleanly against the Renderer
interface.

Wire VulkanRenderer into main.cpp behind a #define USE_VULKAN preprocessor
guard so the project compiles and runs with either backend. When USE_VULKAN
is defined, main.cpp should create a VulkanRenderer instead of GLRenderer.
When it is not defined, behavior is unchanged.

At this point the Vulkan path will show a blank/non-functional window. The
goal is to establish the file structure and verify the interface contract.

==============================================================================

==============================================================================
Task 3: Vulkan instance, device, and swap chain
==============================================================================
Implemented: true

Using vk-bootstrap:
- Create VkInstance with validation layers enabled in debug builds
- Create VkSurfaceKHR via glfwCreateWindowSurface()
- Select a physical device (prefer discrete GPU)
- Create a logical device with graphics and present queue families
- Create a swap chain: VK_PRESENT_MODE_FIFO (v-sync),
  VK_FORMAT_B8G8R8A8_SRGB preferred, window-sized extents
- Store swap chain VkImages and create VkImageViews for each

GLFW must be initialized with glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API)
to skip OpenGL context creation when using Vulkan.

After this task, VulkanRenderer should successfully initialize Vulkan and
create a presentable swap chain, but not yet render anything.

==============================================================================

==============================================================================
Task 4: Command infrastructure and frame synchronization
==============================================================================
Implemented: true

Create a VkCommandPool and per-frame VkCommandBuffers (double-buffered,
i.e. 2 frames in flight).

Create synchronization primitives per frame:
- VkSemaphore imageAvailable — signaled when swap chain image is acquired
- VkSemaphore renderFinished — signaled when command buffer completes
- VkFence inFlight — CPU waits on this before reusing the frame's resources

Implement the core frame loop in display():
1. Wait for in-flight fence
2. Acquire next swap chain image (vkAcquireNextImageKHR)
3. Reset fence and command buffer
4. Begin command buffer recording
5. Begin dynamic rendering to the swap chain image
6. End dynamic rendering
7. Transition image for presentation
8. Submit command buffer (signal renderFinished, wait on imageAvailable)
9. Present (vkQueuePresentKHR)

After this task, the window should clear to a solid color each frame.

==============================================================================

==============================================================================
Task 5: Port shaders to Vulkan GLSL and build SPIR-V
==============================================================================
Implemented: true

Extract all 5 shader programs currently embedded as inline C++ strings in
GLRenderer.cpp into separate .glsl files under assets/shaders/:

  assets/shaders/flat.vert          — flat-color vertex shader
  assets/shaders/flat.frag          — flat-color fragment shader (with lighting)
  assets/shaders/textured.vert      — sprite vertex shader
  assets/shaders/textured.frag      — sprite fragment shader (with lighting + normal maps)
  assets/shaders/text.vert          — text vertex shader
  assets/shaders/text.frag          — text fragment shader
  assets/shaders/vertcolor.vert     — vertex-color vertex shader
  assets/shaders/vertcolor.frag     — vertex-color fragment shader
  assets/shaders/blit.vert          — fullscreen blit vertex shader
  assets/shaders/blit.frag          — fullscreen blit fragment shader
  assets/shaders/lighting.glsl      — shared lighting library (included by flat + textured frags)

Convert from GLSL 330 (OpenGL) to GLSL 450 (Vulkan):
- Change #version 330 core → #version 450
- Replace "uniform mat4 uProjection" with push constants or UBO bindings
- Use layout(set=N, binding=M) for all uniform/sampler bindings
- Use layout(push_constant) for per-draw data (projection, model, color)
- Use layout(std140, set=0, binding=0) for the lighting UBO
- Samplers use layout(set=1, binding=0) etc.

Add CMake rules using the function from Task 1 to compile each .glsl to
.spv at build time. Add a post-build step to copy .spv files to the output
directory alongside assets/shaders/.

==============================================================================
Task 6: Create flat-color graphics pipeline
==============================================================================
Implemented: true

Create a VkPipelineLayout with:
- Push constant range for projection matrix (mat4), model matrix (mat4),
  and color (vec4) — 144 bytes total

Create a VkPipeline (graphics) for flat-color rendering:
- Vertex input: single vec2 attribute (position) at location 0
- Input assembly: VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
- Viewport and scissor: dynamic state (VK_DYNAMIC_STATE_VIEWPORT,
  VK_DYNAMIC_STATE_SCISSOR)
- Rasterizer: polygon fill, no culling (2D sprites face camera)
- Color blending: src-alpha / one-minus-src-alpha (matching GL setup)
- Dynamic rendering: specify color attachment format matching swap chain
- Load flat.vert.spv and flat.frag.spv as shader stages

Create a helper to load .spv files from disk and create VkShaderModules.

==============================================================================
Task 7: Vertex buffer infrastructure with VMA
==============================================================================
Implemented: true

Initialize the VMA allocator (VmaAllocator) during VulkanRenderer
construction, after device creation.

Create a persistent GPU-local vertex buffer for the unit quad (4 vertices,
6 indices or 6 vertices for two triangles) — equivalent to GLRenderer's
m_quadVAO/VBO. Use a one-time staging buffer upload.

Create a per-frame dynamic vertex buffer system for geometry that changes
every frame (circles, rounded rects, sprites, text, vertex-colored
primitives). Options:
- Ring buffer with per-frame offsets, or
- Per-frame host-visible buffer with VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT

The dynamic buffer should support multiple sub-allocations per frame (one
per draw call) and reset each frame.

Clean up VMA allocator and all buffers in VulkanRenderer destructor.

==============================================================================
Task 8: Implement drawRect and drawRectOutlined
==============================================================================
Implemented: true

Implement drawRect(float x, y, w, h, float r, g, b, a):
- Bind the flat-color graphics pipeline
- Set dynamic viewport and scissor
- Push projection matrix, model matrix (translate + scale), and color
  as push constants
- Bind the unit-quad vertex buffer
- vkCmdDraw(6 vertices)

Implement drawRectOutlined(float x, y, w, h, float r, g, b, a, outR, outG,
outB, outA, float thickness):
- Draw filled inner rect via drawRect
- Draw 4 thin border rects (top, bottom, left, right) at the given thickness

Implement clear(r, g, b, a) by storing the clear color and using it as the
dynamic rendering clear value in beginFrame/display.

After this task, the game's platforms, HP bars, and solid-color UI elements
should render correctly under the Vulkan backend.

==============================================================================
Task 9: Texture loading pipeline
==============================================================================
Implemented: true

Implement loadTexture(const std::string& path) -> TextureHandle:
1. Check texture cache (path -> handle map) to avoid reloading
2. Load image via stbi_load(path, &w, &h, &channels, 4) -- force RGBA
3. If load fails, use 2x2 magenta fallback texture
4. Create VkImage (R8G8B8A8_SRGB, usage: SAMPLED | TRANSFER_DST) via VMA
5. Create staging buffer, memcpy pixel data, submit copy command
6. Transition image layout: UNDEFINED -> TRANSFER_DST -> SHADER_READ_ONLY
7. Create VkImageView (2D, RGBA, full mip range)
8. Create VkSampler: nearest filtering (pixel art), clamp-to-edge wrapping
9. Assign incrementing TextureHandle, store in cache
10. Free staging buffer and stbi data

Create a VkDescriptorSetLayout for texture binding:
- Binding 0: combined image sampler (diffuse texture)
- Binding 1: combined image sampler (normal map)

Create a VkDescriptorPool sized for expected texture count.

For per-draw texture binding, allocate or update descriptor sets. Consider
a simple descriptor set cache keyed by (diffuseHandle, normalHandle).

==============================================================================
Task 10: Port textured sprite pipeline
==============================================================================
Implemented: true

Create a VkPipeline for textured sprite rendering:
- Vertex input: vec2 position (location 0) + vec2 texcoord (location 1)
- Push constants: projection matrix (mat4)
- Descriptor set 0: lighting UBO (placeholder for now, bind dummy buffer)
- Descriptor set 1: diffuse sampler (binding 0) + normal map sampler (binding 1)
- Alpha blending enabled, dynamic viewport/scissor
- Dynamic rendering with swap chain color format

Implement drawSprite(TextureHandle tex, float x, y, int frameX, frameY,
frameW, frameH, float originX, originY):
- Build 6-vertex quad with UV coordinates calculated from frame rect
  and full texture dimensions (same math as GLRenderer)
- Upload vertices to dynamic buffer
- Bind textured pipeline
- Push projection matrix
- Bind texture descriptor set
- vkCmdDraw(6)

==============================================================================
Task 11: Normal map support
==============================================================================
Implemented: true

Implement companion _n.png auto-loading matching GLRenderer's pattern:
- When loading "assets/foo.png", check for "assets/foo_n.png"
- If found, load as a separate VkImage (same pipeline as loadTexture)
- Cache in a normal map table (keyed by diffuse TextureHandle)
- If not found, store a sentinel (0) to avoid re-checking on every draw

When binding the textured pipeline's descriptor set:
- Binding 0: diffuse texture
- Binding 1: normal map if available, else a default flat-normal texture
  (1-pixel texture with RGBA = (128, 128, 255, 255) representing the
  flat normal (0, 0, 1) in tangent space)

Port the uHasNormalMap flag: either as a push constant bool or a
specialization constant. The fragment shader uses this to decide whether
to sample the normal map or use the flat (0, 0, 1) default.

==============================================================================

==============================================================================
Task 12: FreeType glyph atlas as VkImage
==============================================================================
Implemented: true

Port glyph atlas creation from GLRenderer to Vulkan:
1. Keep FreeType initialization (FT_Init_FreeType, FT_New_Face) unchanged
2. Keep the per-font-size atlas building logic: iterate ASCII 32-126,
   measure glyph bitmaps, compute atlas dimensions
3. Instead of glTexImage2D + glTexSubImage2D:
   - Create VkImage (R8_UNORM, usage: SAMPLED | TRANSFER_DST) via VMA
   - Create staging buffer sized to atlasWidth x atlasHeight
   - Rasterize glyphs into the staging buffer (memcpy each glyph bitmap)
   - Submit copy command (vkCmdCopyBufferToImage)
   - Transition layout to SHADER_READ_ONLY_OPTIMAL
   - Create VkImageView and VkSampler (linear filtering, clamp-to-edge)
4. Store atlas texture handle and GlyphInfo array (UV coords, bearing,
   advance) in FontData -- same data structures as GLRenderer
5. Cache atlases per (font, size) pair

Implement loadFont(const std::string& path) -> FontHandle matching the
existing pattern (FT_New_Face, assign handle, store in font map).

==============================================================================

==============================================================================
Task 13: Text rendering pipeline
==============================================================================
Implemented: true

Create a VkPipeline for text rendering:
- Vertex input: vec2 position (location 0) + vec2 texcoord (location 1)
- Push constants: projection matrix (mat4) + text color (vec4)
- Descriptor set: glyph atlas combined image sampler (binding 0)
- Alpha blending enabled, dynamic viewport/scissor
- Dynamic rendering with swap chain color format
- Load text.vert.spv + text.frag.spv

Implement drawText(FontHandle font, const std::string& str, float x, y,
unsigned int size, float r, g, b, float a):
- Get or build glyph atlas for the requested size
- Build vertex array: for each character, create a textured quad using
  the glyph's atlas UV, bearing, and advance (same math as GLRenderer)
- Upload batched vertices to dynamic buffer
- Bind text pipeline, push projection + color, bind atlas descriptor
- vkCmdDraw(vertexCount)

Implement measureText(FontHandle font, const std::string& str,
unsigned int size, float& outWidth, float& outHeight):
- CPU-only calculation using glyph advance/bearing — no Vulkan calls

==============================================================================
==============================================================================
Task 14: Vertex-color pipeline and primitives
==============================================================================
Implemented: true

Create VkPipeline(s) for vertex-colored geometry:
- Vertex input: vec2 position (location 0) + vec4 color (location 1)
- Push constants: projection matrix (mat4)
- Alpha blending enabled, dynamic viewport/scissor

Vulkan does not support GL_TRIANGLE_FAN. Options:
- Create separate pipelines for TRIANGLE_LIST, TRIANGLE_STRIP, and
  LINE_LIST topologies, OR
- Use VK_EXT_extended_dynamic_state (core in 1.3) for dynamic topology

Implement drawTriangleStrip(const vector<Vertex>& verts):
- Upload vertices to dynamic buffer, bind vertex-color pipeline with
  triangle-strip topology, draw

Implement drawLines(const vector<Vertex>& verts):
- Same but with line-list topology

Implement drawCircle(float cx, cy, radius, r, g, b, a, bool filled):
- Generate circle vertices (32 segments) as a triangle list (not fan)
  for filled circles, or as a triangle-strip ring for outlines
- Upload to dynamic buffer, draw

Implement drawRoundedRect(float x, y, w, h, float radius, r, g, b, a,
bool filled):
- Generate rounded rect vertices as triangle list (filled) or strip
  (outline) - convert from GLRenderer's triangle-fan approach
- Upload to dynamic buffer, draw

==============================================================================

==============================================================================
Task 15: Off-screen render target
==============================================================================
Implemented: true

Create a VkImage to serve as the off-screen world render target (equivalent
to GLRenderer's m_fboColorTex):
- Format: same as swap chain (B8G8R8A8_SRGB)
- Usage: COLOR_ATTACHMENT | SAMPLED (will be sampled during blit pass)
- Create VkImageView and VkSampler for the blit pass to read from

Implement beginFrame():
- Set m_worldPass = true
- Transition off-screen image to COLOR_ATTACHMENT_OPTIMAL
- Begin dynamic rendering targeting the off-screen image
- Set viewport and scissor to off-screen image dimensions
- Apply clear color

The off-screen image must be recreated when the window resizes (same
dimensions as the swap chain).

Store the off-screen image, view, sampler, and VMA allocation as members.
Clean up in destructor and before recreation on resize.

==============================================================================
==============================================================================
Task 16: Lighting uniform buffer
==============================================================================
Implemented: true

Create a per-frame uniform buffer (UBO) for lighting data:

Layout (matching the GLSL lighting.glsl struct):
  struct LightUBO {
      vec3 ambientColor;       // 12 bytes + 4 padding
      int  numLights;          // 4 bytes
      Light lights[32];        // each Light: position(vec2), color(vec3),
                               //   intensity(float), radius(float), z(float),
                               //   direction(vec2), innerCone(float),
                               //   outerCone(float), type(int)
                               //   → pack to std140 alignment
  };

Use a host-visible, host-coherent buffer (VMA) per frame in flight.

Implement addLight(const Light& light): append to CPU-side vector
Implement clearLights(): clear the vector
Implement setAmbientColor(r, g, b): store ambient color

Before each draw that uses lighting (flat, textured), memcpy the light
data to the mapped UBO and bind it to descriptor set 0, binding 0.

When m_worldPass is false (UI rendering after endFrame), upload
ambient=(1,1,1) and numLights=0 so UI renders at full brightness.

==============================================================================

==============================================================================
Task 17: Lighting shader integration and blit pass
==============================================================================
Implemented: true

Integrate the lighting UBO into the flat and textured fragment shaders:
- Both shaders #include lighting.glsl (or inline the shared code)
- Read ambient + lights from the UBO bound at set 0, binding 0
- calcLighting() function: per-pixel Lambertian diffuse, quadratic
  attenuation, spot-light angular falloff -- same math as GLRenderer

Implement endFrame():
1. End the off-screen dynamic rendering pass
2. Transition off-screen image: COLOR_ATTACHMENT -> SHADER_READ_ONLY
3. Transition swap chain image: UNDEFINED -> COLOR_ATTACHMENT
4. Begin dynamic rendering targeting the swap chain image
5. Bind the blit pipeline (fullscreen quad)
6. Bind descriptor set with the off-screen image as sampled texture
7. Draw fullscreen quad (6 vertices in NDC)
8. Set m_worldPass = false (subsequent draws go directly to swap chain
   without lighting -- for UI overlay)

After endFrame(), any drawRect/drawText/etc. calls (UI) should render
directly to the swap chain image with full-brightness lighting (ambient=1,
numLights=0), matching GLRenderer's behavior.

display() must:
- End the swap chain dynamic rendering (if still active)
- Transition swap chain image for presentation
- Submit command buffer and present

==============================================================================

==============================================================================
Task 18: Window and input management
==============================================================================
Implemented: true

Implement window management methods (mostly GLFW calls, same as GLRenderer):
- isOpen(): return !glfwWindowShouldClose(m_window)
- close(): glfwSetWindowShouldClose(m_window, GLFW_TRUE)
- setMouseCursorVisible(bool): glfwSetInputMode cursor
- setWindowSize(w, h): glfwSetWindowSize + trigger swap chain recreation
- setWindowPosition(x, y): glfwSetWindowPos
- getDesktopSize(&w, &h): glfwGetVideoMode(glfwGetPrimaryMonitor())
- getWindowSize(&w, &h): return stored window dimensions

Handle swap chain recreation on window resize:
1. vkDeviceWaitIdle()
2. Destroy old swap chain image views
3. Destroy old off-screen render target (image, view, sampler)
4. Recreate swap chain with new extents (vk-bootstrap)
5. Recreate swap chain image views
6. Recreate off-screen render target at new size
7. Update stored window dimensions

Register glfwSetFramebufferSizeCallback to trigger recreation.
Handle minimization (width=0 or height=0) by skipping frames.

==============================================================================
==============================================================================
Task 19:Camera and view system
==============================================================================
Implemented: true

Implement setView(float centerX, centerY, width, height):
- Compute orthographic projection using glm::ortho with Vulkan's 0..1
  depth range (enabled by GLM_FORCE_DEPTH_ZERO_TO_ONE):
    left   = centerX - width/2
    right  = centerX + width/2
    top    = centerY - height/2
    bottom = centerY + height/2
- Store the projection matrix for use in push constants

Implement resetView():
- Set projection to a screen-space orthographic matrix:
    left=0, right=windowWidth, top=0, bottom=windowHeight
- Used for UI rendering after endFrame()

Implement getWindowSize(float& w, float& h):
- Return current window dimensions as floats

All draw calls should use the current projection matrix in their push
constants. The camera-follows-player behavior in main.cpp calls setView()
each frame with the player's position — no changes needed in main.cpp.

==============================================================================

==============================================================================
Task 20: Input system integration
==============================================================================
Implemented: true

GLFWInput depends on GLFWwindow* and GLFW callbacks, not on OpenGL. However,
GLFWInput.h includes <glad/gl.h> which will be removed later.

For this task:
- Refactor GLFWInput.h to remove the #include <glad/gl.h> if it is not
  actually needed (check if any GL types or functions are used — likely not,
  since GLFWInput only uses GLFW functions)
- If glad/gl.h is needed for a type, replace with a forward declaration or
  the appropriate GLFW-only header
- Have VulkanRenderer own a GLFWInput instance, same as GLRenderer does
- Implement getInput(): return reference to the owned GLFWInput

After this task, the input system (keyboard, mouse, gamepad) should work
identically under both backends.

==============================================================================

==============================================================================
Task 21: Remove OpenGL backend
==============================================================================
Implemented: true

With all Renderer methods fully implemented in VulkanRenderer:

1. Deleted src/Rendering/GLRenderer.h and GLRenderer.cpp
2. Deleted src/Rendering/Shader.h and Shader.cpp
3. Removed third_party/glad/ directory entirely
4. Updated CMakeLists.txt:
   - Removed GLRenderer.cpp, Shader.cpp from add_executable sources
   - Removed third_party/glad/src/gl.c from sources
   - Removed third_party/glad/include from include directories
   - Removed $<$<PLATFORM_ID:Windows>:opengl32> from link libraries
5. Removed the #define USE_VULKAN guard from main.cpp — VulkanRenderer
   is now the only renderer
6. Removed any remaining #include <glad/gl.h> from the codebase
7. Verified the project compiles and links without any OpenGL references

==============================================================================

==============================================================================
Task 22: Validation and testing
==============================================================================
Implemented: true

Run the game with Vulkan validation layers enabled (VK_LAYER_KHRONOS_validation)
and verify zero validation errors during normal gameplay.

Implementation:
1. Custom debug messenger callback (vulkanDebugCallback) with categorized output
   and atomic error/warning counters — replaces vk-bootstrap default messenger
2. GPU timestamp queries (VK_QUERY_TYPE_TIMESTAMP) for per-frame GPU timing —
   writes timestamps at top-of-pipe and bottom-of-pipe, reads back after fence
3. Diagnostics API on Renderer: getValidationErrorCount(), getValidationWarningCount(),
   getGpuFrameTimeMs(), getGpuDeviceName() — virtual with defaults, overridden in VulkanRenderer
4. Debug menu (F1) expanded to show GPU device name, GPU frame time (ms),
   validation error count (red if >0), and warning count (yellow if >0)
5. Shutdown summary log prints total validation errors/warnings

Test all rendering features:
- Flat-color rectangles: platforms, HP bars, transition fade overlays
- Outlined rectangles: HP bar outlines, UI elements
- Textured sprites: player animation, slime animation, sprite sheets
- Normal-mapped lighting: point lights, spot lights, player dynamic light,
  normal map interaction on sprites and flat surfaces
- Text rendering: UI labels, menu text, debug text at multiple sizes
- UI menus: save slot screen, pause menu, controls menu
- Circles: slime attack particles (drawCircle)
- Rounded rectangles: UI buttons and panels
- Vertex-colored geometry: combat arc VFX (drawTriangleStrip), debug lines
- Transition fades: room transitions with fade-out/fade-in
- Debug menu: F1 overlay with map loader dialog
- Window resize: verify swap chain recreation, no artifacts
- V-sync: verify smooth 60 FPS frame pacing
- Camera: verify player-following camera, setView/resetView transitions

Profile GPU frame time and compare against the old OpenGL backend to ensure
no significant performance regression.
==============================================================================

==============================================================================
Task 23: Update documentation and architecture
==============================================================================
Implemented: true

Updated all project documentation to reflect the completed Vulkan 1.3 migration:

1. ReadMe.md:
   - Fixed typos, added Build Requirements section (Vulkan SDK, CMake, C++17,
     PhysX) and detailed Build Instructions (configure, build, shader
     compilation, run)
   - Noted that GLSL→SPIR-V compilation happens automatically at build time
     via glslc from the Vulkan SDK

2. CMakePresets.json:
   - Removed stale SFML cache variables (SFML_BUILD_EXAMPLES, SFML_BUILD_DOC,
     SFML_BUILD_TEST_SUITE) left over from the pre-OpenGL era

3. architecture.md:
   - Already up to date from prior migration tasks — verified Vulkan 1.3
     references, VulkanRenderer documentation, Build & Dependencies table,
     shader pipeline description, and removal of all OpenGL/GLAD references

4. .gitignore:
   - Already contains *.spv for compiled SPIR-V shaders — no changes needed

==============================================================================

==============================================================================
Task 24: XP and leveling data model
==============================================================================
Implemented: true

Add experience and level tracking to PlayerState (src/Core/PlayerState.h):

  uint32_t xp    = 0;     // Current XP within the current level
  uint32_t level = 1;     // Current level (1-5, capped)

Add methods:
  void awardXP(uint32_t amount);  // Adds XP points; auto-levels when
                                   // xp reaches xpToLevel (5). Caps at
                                   // level 5. Resets xp to 0 on level-up.
  uint32_t getXPToLevel() const;   // Returns 5 (constant, linear)
  bool isMaxLevel() const;         // Returns level >= 5

Update SaveSystem (src/Core/SaveSystem.h/.cpp):
- Add "xp" and "level" fields to the JSON save format
- Write them in SaveSystem::save()
- Read them in SaveSystem::load() with defaults (0 and 1) for backward
  compatibility with existing save files that lack these fields

Update main.cpp:
- Include xp/level in the buildSaveData() lambda
- Restore xp/level from loaded SaveData when starting/loading a game

==============================================================================

==============================================================================
Task 25: XP award on enemy kill
==============================================================================
Implemented: true

In main.cpp's enemy update loop (the dead-enemy detection block), award
XP when an enemy is first detected as dead:

Currently, the code detects isDead() and queues respawns. Add XP award
BEFORE the respawn queue check, but only once per death. Use the existing
`alreadyQueued` check -- if the enemy is dead and NOT yet in the respawn
queue, it's a fresh kill:

  if (hp && hp->isDead())
  {
      bool alreadyQueued = false;
      for (const auto& entry : respawnQueue)
          if (entry.definitionIndex == i) { alreadyQueued = true; break; }
      if (!alreadyQueued && i < defs.size())
      {
          playerState.awardXP(1);   // <-- NEW: award 1 XP per kill
          respawnQueue.push_back({i, ENEMY_RESPAWN_TIME});
      }
      continue;
  }

This awards exactly 1 XP per enemy death. With xpToLevel = 5, the player
levels up every 5 kills. Respawned enemies award XP again when re-killed.

==============================================================================

==============================================================================
Task 26: XP bar and level HUD
==============================================================================
Implemented: true

Render an XP bar and level indicator in the screen-space UI overlay (after
endFrame() / resetView(), alongside existing UI elements).

XP bar design:
- Position: bottom-left of screen (e.g., x=20, y=windowH-40)
- Size: 200x10 pixels
- Background: dark gray with black outline (drawRectOutlined)
- Fill: gold/yellow color, width = (xp / xpToLevel) * barWidth
- When at max level (5), bar is full and a different color (e.g. cyan)

Level indicator:
- Text "Lv N" rendered to the left of or above the XP bar
- Use the existing font handle and drawText()
- Font size ~18, white or gold color

Implementation location: add to main.cpp's UI rendering section, after
the existing HUD elements. Use renderer.drawRectOutlined() for the bar
background, renderer.drawRect() for the fill, and renderer.drawText()
for the level label.

Only render when gameStarted is true and not on the save-slot screen.

==============================================================================

==============================================================================
Task 27: Armor sprite progression
==============================================================================
Implemented: true

Create or extend the player sprite sheet to include 5 armor tiers. Each
tier has the same animation set as the base character (idle, run-right,
run-left, jump, fall, wall-slide-right, wall-slide-left, dash-right,
dash-left) but with progressively more armor drawn on the character.

Sprite sheet layout (assets/player_spritesheet.png):
- The current sheet has 9 rows (rows 0-8) for the base character
- Add 4 more sets of 9 rows each for armor tiers 2-5
  OR create separate sprite sheets per tier:
    assets/player_armor_1.png (existing base)
    assets/player_armor_2.png (boots)
    assets/player_armor_3.png (boots + legs)
    assets/player_armor_4.png (boots + legs + chest)
    assets/player_armor_5.png (full armor)

Since this is a code task (not art), create placeholder sprites:
- Tier 2-5: copy the base frames but tint them with progressively
  stronger color overlays to visually indicate armor level:
    Tier 2: slight bronze tint
    Tier 3: silver tint
    Tier 4: gold tint
    Tier 5: bright platinum/white tint
- Use stb_image to load the base sheet, apply per-pixel color multiply
  in a build-time or startup tool, and save the tinted versions
  OR simply reuse the same sprite sheet and change the draw tint

Simpler alternative (recommended for now): add a tint/color overlay to
the AnimationComponent or drawSprite system, so the same sprites render
with a level-based color wash. This avoids creating new art assets.

Add a function in main.cpp (or a helper) that updates the player's
visual appearance based on playerState.level. Call it on level-up and
on game load. If using the tint approach, store the tint color per level:
  Level 1: no tint (1.0, 1.0, 1.0)
  Level 2: bronze (1.0, 0.85, 0.6)
  Level 3: silver (0.8, 0.85, 0.9)
  Level 4: gold (1.0, 0.9, 0.4)
  Level 5: platinum (0.9, 0.95, 1.0) with slight glow

==============================================================================

==============================================================================
==  EXPERIENCE & LEVELING SYSTEM                                            ==
==============================================================================

Add an experience and leveling system. The player earns XP by killing
enemies and levels from 1 to 5. Each level unlocks a visual armor piece
on the player sprite. At level 3, the player unlocks a charged spin-slash
attack (hold X to charge, release to execute).

Design overview:
- PlayerState gains xp (uint32_t) and level (uint32_t, 1-5)
- 5 kills per level (linear): each enemy awards 1 kill's worth of XP
  (xpPerKill = 1, xpToLevel = 5 at every level)
- XP bar rendered as HUD element (screen-space, below HP or at bottom)
- Level text displayed next to XP bar ("Lv 3")
- On level-up: update player sprite to the next armor tier
- Armor visuals: 5 rows of sprites in a new spritesheet (or extend the
  existing player_spritesheet.png with armor overlay rows)
- Level and XP persist via SaveSystem (add to SaveData JSON)

Armor progression (cosmetic, one piece added per level):
  Level 1: Base character (no armor) -- current sprites
  Level 2: Boots -- new sprite row set
  Level 3: Boots + Legs -- new sprite row set + unlocks spin slash
  Level 4: Boots + Legs + Chest -- new sprite row set
  Level 5: Boots + Legs + Chest + Arms + Helm (full armor) -- new sprite set

Charged spin-slash attack (unlocked at level 3):
- Hold the attack key (X / gamepad X) to charge
- Visual charge indicator (glow or particle effect on player)
- On release after minimum charge time (0.6s), execute a forward spin slash
- Spin slash has wider reach (1.5x normal) and higher damage (2x normal)
- Non-looping "spin-slash" animation plays during the attack
- If released before minimum charge time, do a normal attack instead
- Cannot charge while already attacking or during cooldown

Architecture notes for agents:
- PlayerState stores xp/level -- central place for progression data
- CombatComponent needs a charge mechanic: track hold duration, detect
  release edge, switch between normal and spin-slash attacks
- InputComponent.InputState needs no changes -- just use the existing
  `attack` bool and detect hold vs tap in CombatComponent
- XP award happens in main.cpp's dead-enemy detection loop -- when an
  enemy first becomes dead, award XP to PlayerState
- Sprite switching: AnimationComponent already supports multiple named
  animations. On level-up, swap to the new armor tier's animation set
  by re-calling addAnimation() with the new sprite atlas rows
- SaveSystem: add "xp" and "level" fields to the JSON, with defaults
  for backward compatibility (missing = level 1, xp 0)

==============================================================================
Task 28: Charged spin-slash attack
==============================================================================
Implemented: true

Add a charged spin-slash attack to CombatComponent, unlocked at level 3.

CombatComponent changes (src/Components/CombatComponent.h/.cpp):

Add new members:
  bool  m_chargeUnlocked = false;   // Set to true at level 3+
  bool  m_charging       = false;   // Currently holding attack button
  float m_chargeTimer    = 0.f;     // How long attack has been held
  float m_minChargeTime  = 0.6f;    // Minimum hold for spin slash
  float m_spinDamageMult = 2.0f;    // 2x normal damage
  float m_spinReachMult  = 1.5f;    // 1.5x normal reach

Add method:
  void setChargeUnlocked(bool unlocked);  // Called when player reaches lv3

Update the attack flow in update(dt):
1. When attack button is pressed (rising edge) and not on cooldown:
   - If charge is unlocked: enter charging state, start m_chargeTimer
   - If charge is NOT unlocked: start normal attack (existing behavior)
2. While charging (button held): increment m_chargeTimer each frame
3. When attack button is released (falling edge) while charging:
   - If m_chargeTimer >= m_minChargeTime: execute spin slash
     (wider hitbox, more damage, spin-slash animation)
   - If m_chargeTimer < m_minChargeTime: execute normal attack
4. If the player takes damage while charging, cancel the charge

Spin-slash hitbox:
- Same AABB approach as normal attack but reach *= m_spinReachMult
- Vertical size *= 1.2 (same as normal)
- Damage = m_damage * m_spinDamageMult

Spin-slash visual:
- Render a wider, more dramatic arc sweep than the normal attack
- Use a brighter color or thicker triangle strip
- Duration slightly longer than normal (0.4s vs 0.3s)

Charge visual feedback:
- While charging, render a growing glow/pulse around the player
- Could use drawCircle with increasing radius and pulsing alpha
- Or flash the player sprite at an increasing frequency

Add a "spin-slash-right" and "spin-slash-left" animation to the player's
AnimationComponent (can reuse existing frames with faster timing, or add
new placeholder frames). Play this animation during the spin slash.

Integration in main.cpp:
- After awardXP(), check if playerState.level >= 3
- If so, call combat->setChargeUnlocked(true)
- Also check on game load (in case saved at level 3+)

==============================================================================
Task 25: XP award on enemy kill
==============================================================================
Implemented: true

In main.cpp's enemy update loop (the dead-enemy detection block), award
XP when an enemy is first detected as dead:

Currently, the code detects isDead() and queues respawns. Add XP award
BEFORE the respawn queue check, but only once per death. Use the existing
`alreadyQueued` check -- if the enemy is dead and NOT yet in the respawn
queue, it's a fresh kill:

  if (hp && hp->isDead())
  {
      bool alreadyQueued = false;
      for (const auto& entry : respawnQueue)
          if (entry.definitionIndex == i) { alreadyQueued = true; break; }
      if (!alreadyQueued && i < defs.size())
      {
          playerState.awardXP(1);   // <-- NEW: award 1 XP per kill
          respawnQueue.push_back({i, ENEMY_RESPAWN_TIME});
      }
      continue;
  }

This awards exactly 1 XP per enemy death. With xpToLevel = 5, the player
levels up every 5 kills. Respawned enemies award XP again when re-killed.

==============================================================================
==============================================================================
Task 26: XP bar and level HUD
==============================================================================
Implemented: true

Render an XP bar and level indicator in the screen-space UI overlay (after
endFrame() / resetView(), alongside existing UI elements).

XP bar design:
- Position: bottom-left of screen (e.g., x=20, y=windowH-40)
- Size: 200x10 pixels
- Background: dark gray with black outline (drawRectOutlined)
- Fill: gold/yellow color, width = (xp / xpToLevel) * barWidth
- When at max level (5), bar is full and a different color (e.g. cyan)

Level indicator:
- Text "Lv N" rendered to the left of or above the XP bar
- Use the existing font handle and drawText()
- Font size ~18, white or gold color

Implementation location: add to main.cpp's UI rendering section, after
the existing HUD elements. Use renderer.drawRectOutlined() for the bar
background, renderer.drawRect() for the fill, and renderer.drawText()
for the level label.

Only render when gameStarted is true and not on the save-slot screen.

==============================================================================

==============================================================================
Task 27: Knight armor sprite generation
==============================================================================
Implemented: true

Generate 4 additional player sprite sheets (armor tiers 2-5) that show
the character wearing progressively more pieces of knight armor. Each
tier must be visually distinct from the others -- not just color tints.

Created tools/generate_armor_sprites.py (Python/Pillow) that loads the base
player_spritesheet.png and overlays armor shapes per frame based on the
character silhouette. Each tier progressively adds armor pieces:
  Tier 2: Boots (metallic gray on lower legs/feet)
  Tier 3: Boots + leg plates (greaves with plate lines and rivets)
  Tier 4: Boots + legs + chestplate (torso plate mail with shoulder guards)
  Tier 5: Full knight armor (helm with visor slit, gauntlets, gold accents)

CMake integration regenerates sheets when base spritesheet or script changes.

Updated main.cpp with updatePlayerArmorSprites() helper that re-registers
all player animations with the correct armor-tier atlas path. Called on
level-up (after awardXP()) and on initial player setup (which handles game
load). Replaced the tint-only approach with actual sprite sheet switching.
==============================================================================

==============================================================================
Task 28: Max-level particle effect
==============================================================================
Implemented: true

When the player reaches level 5 (full knight armor), add a persistent
ambient particle effect -- small glowing sparks or embers that drift
off the player character, giving a "legendary" feel.

Create a simple ParticleEmitter component or a lightweight particle
system embedded in main.cpp that:
- Spawns 2-4 small particles per second around the player position
- Each particle:
  - Starts at a random offset within the player bounding box
  - Drifts upward and slightly sideways (randomized velocity)
  - Fades from bright (gold/white, alpha=0.8) to transparent over 1-2s
  - Size: 2-4 pixels (small sparkle)
- Uses the existing drawRect() or drawCircle() API for rendering
  (no new textures needed)
- Particles are rendered in world space (before endFrame(), so they
  benefit from lighting) at the player's position

Implementation:
- Add a simple Particle struct: { vec2 pos, vec2 vel, float life,
  float maxLife, float size }
- Store a vector<Particle> in main.cpp (or a new ParticleComponent)
- In the update loop, if playerState.level >= 5:
  - Spawn new particles at a timed interval
  - Update existing particles (move, age, remove dead ones)
- In the render loop, draw surviving particles as small colored circles
  or rects with alpha based on remaining life

Particle colors (cycle/randomize between):
- Warm gold: (1.0, 0.85, 0.3)
- White-hot: (1.0, 1.0, 0.9)
- Ember orange: (1.0, 0.6, 0.2)

The effect should be subtle but noticeable -- a visual reward for
reaching max level, not an overwhelming particle storm.

Disable particles when the player is below level 5 (including after
death/respawn if level is preserved). Re-enable on game load if the
saved level is 5.

==============================================================================