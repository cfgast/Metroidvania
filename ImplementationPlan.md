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
Implemented: false

==============================================================================
Task: Add a room/zone transition system. A map can contain named Transition zones (axis-aligned rectangles in the map JSON). When the player walks into a transition zone the game fades to black, unloads the current map, loads the target map (referenced by filename in the transition definition), and spawns the player at the target map's matching spawn point. Update world_01.json with at least one transition leading to a second map file.
Implemented: false

==============================================================================
Task: Add a collectible ability pick-up system. Define an Ability enum (e.g. DoubleJump, WallSlide, Dash). Each Ability pick-up is a static GameObject placed in the map JSON. When the player overlaps one, the pick-up is consumed and the corresponding ability is unlocked on the player's PhysicsComponent. Implement DoubleJump first (allow one extra jump while airborne). Abilities persist across room transitions in a PlayerState struct.
Implemented: false

==============================================================================
Task: Add a persistent save/load system. PlayerState (position, current map filename, unlocked abilities, HP) should be serializable to a JSON save file. Add a save slot selection screen accessible from the main menu. Auto-save on room transition. Allow manual save from the pause menu (Escape key).
Implemented: false