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
Implemented: true

==============================================================================