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
==  VULKAN MIGRATION                                                        ==
==============================================================================
==============================================================================

Migrate the rendering backend from OpenGL 3.3 Core to Vulkan 1.3. The
existing Renderer pure-virtual interface isolates all OpenGL usage behind
GLRenderer — only main.cpp references the concrete class. All game code,
components, UI, and map rendering use the Renderer abstraction, making the
migration clean.

Design decisions:
- Full replacement: GLRenderer + GLAD + opengl32 removed after migration
- Vulkan 1.3: dynamic rendering (no VkRenderPass), push descriptors
- Helper libraries: VMA (memory allocation), vk-bootstrap (instance/device)
- Offline shader compilation: GLSL 450 → SPIR-V via glslc at CMake build time
- GLFW retained: already supports Vulkan surfaces natively
- GLM retained: add GLM_FORCE_DEPTH_ZERO_TO_ONE for Vulkan 0..1 depth range

Current rendering inventory (to be ported):
- 5 shader programs: flat-color (lit), textured sprite (lit + normal maps),
  text, vertex-color, fullscreen blit — all inline GLSL in GLRenderer.cpp
- 6 VAO/VBO pairs: quad, sprite, text, dynamic, vertex-color, fullscreen
- FBO pipeline: off-screen render → lighting compositing → blit to screen
- 32-light system: point + spot lights, quadratic attenuation, normal maps
- FreeType glyph atlas text rendering
- Texture cache with auto-loaded _n.png normal maps

Architecture notes for agents:
- Create src/Rendering/VulkanRenderer.h/.cpp implementing Renderer interface
- All shaders go to assets/shaders/*.glsl, compiled to .spv by CMake
- Use VMA for all VkImage/VkBuffer allocations (no raw vkAllocateMemory)
- Use vk-bootstrap for instance, physical device, logical device, swap chain
- Push constants for per-draw uniforms (projection, model, color)
- UBO or SSBO for light array (up to 32 lights + ambient)
- Dynamic rendering (VK_KHR_dynamic_rendering, core in 1.3) — no render pass
  or framebuffer objects needed
- Double-buffered command buffers with semaphore + fence synchronization
- Swap chain recreation on window resize
- GLFWInput can be reused directly (depends on GLFWwindow*, not OpenGL)

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
Task 19:Camera and view system
==============================================================================
Implemented: false

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
Task 20: Input system integration
==============================================================================
Implemented: false

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
Task 21: Remove OpenGL backend
==============================================================================
Implemented: false

With all Renderer methods fully implemented in VulkanRenderer:

1. Delete src/Rendering/GLRenderer.h and GLRenderer.cpp
2. Delete src/Rendering/Shader.h and Shader.cpp
3. Remove third_party/glad/ directory entirely
4. Update CMakeLists.txt:
   - Remove GLRenderer.cpp, Shader.cpp from add_executable sources
   - Remove third_party/glad/src/gl.c from sources
   - Remove third_party/glad/include from include directories
   - Remove $<$<PLATFORM_ID:Windows>:opengl32> from link libraries
5. Remove the #define USE_VULKAN guard from main.cpp — VulkanRenderer
   becomes the only renderer
6. Remove any remaining #include <glad/gl.h> from the codebase
7. Verify the project compiles and links without any OpenGL references

==============================================================================
Task 22: Validation and testing
==============================================================================
Implemented: false

Run the game with Vulkan validation layers enabled (VK_LAYER_KHRONOS_validation)
and verify zero validation errors during normal gameplay.

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
Task 23: Update documentation and architecture
==============================================================================
Implemented: false

Update architecture.md:
- Change "OpenGL 3.3 Core" references to "Vulkan 1.3"
- Update the Rendering section: VulkanRenderer replaces GLRenderer,
  describe new pipeline (swap chain, dynamic rendering, push constants,
  descriptor sets, VMA, SPIR-V shaders)
- Update the Build & Dependencies table: remove GLAD and opengl32,
  add Vulkan SDK, VMA, vk-bootstrap
- Update shader information: external .glsl/.spv files in assets/shaders/
  instead of inline GLSL strings
- Note the offline SPIR-V compilation step in the build instructions

Update ReadMe.md:
- Add Vulkan SDK as a build requirement
- Update build instructions if any CMake steps changed

Update .gitignore:
- Add *.spv if compiled shaders are not committed
- Add any Vulkan-specific build artifacts

==============================================================================