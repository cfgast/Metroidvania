==============================================================================
Implementation Notes
==============================================================================
This implementation plan contains a series of tasks. Each task should be picked up and implementated by an individual agent. Every Task contains the task description and if it has been implemented already. When an implementation is completed, change it to true. When implementations are updated, update any corresponding documentation. Also add new built files to .gitignore, and check in and push the new change to the remote along with the description of the change.

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
Implemented: false

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
Implemented: false

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
Implemented: false

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
Implemented: false

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

