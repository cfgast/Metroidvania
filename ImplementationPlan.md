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
Task 9: Texture loading pipeline
==============================================================================
Implemented: false

Implement loadTexture(const std::string& path) → TextureHandle:
1. Check texture cache (path → handle map) to avoid reloading
2. Load image via stbi_load(path, &w, &h, &channels, 4) — force RGBA
3. If load fails, use 2×2 magenta fallback texture
4. Create VkImage (R8G8B8A8_SRGB, usage: SAMPLED | TRANSFER_DST) via VMA
5. Create staging buffer, memcpy pixel data, submit copy command
6. Transition image layout: UNDEFINED → TRANSFER_DST → SHADER_READ_ONLY
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
Implemented: false

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
Implemented: false

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
Task 12: FreeType glyph atlas as VkImage
==============================================================================
Implemented: false

Port glyph atlas creation from GLRenderer to Vulkan:
1. Keep FreeType initialization (FT_Init_FreeType, FT_New_Face) unchanged
2. Keep the per-font-size atlas building logic: iterate ASCII 32–126,
   measure glyph bitmaps, compute atlas dimensions
3. Instead of glTexImage2D + glTexSubImage2D:
   - Create VkImage (R8_UNORM, usage: SAMPLED | TRANSFER_DST) via VMA
   - Create staging buffer sized to atlasWidth × atlasHeight
   - Rasterize glyphs into the staging buffer (memcpy each glyph bitmap)
   - Submit copy command (vkCmdCopyBufferToImage)
   - Transition layout to SHADER_READ_ONLY_OPTIMAL
   - Create VkImageView and VkSampler (linear filtering, clamp-to-edge)
4. Store atlas texture handle and GlyphInfo array (UV coords, bearing,
   advance) in FontData — same data structures as GLRenderer
5. Cache atlases per (font, size) pair

Implement loadFont(const std::string& path) → FontHandle matching the
existing pattern (FT_New_Face, assign handle, store in font map).

==============================================================================
Task 13: Text rendering pipeline
==============================================================================
Implemented: false

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
Task 14: Vertex-color pipeline and primitives
==============================================================================
Implemented: false

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
  (outline) — convert from GLRenderer's triangle-fan approach
- Upload to dynamic buffer, draw

==============================================================================
Task 15: Off-screen render target
==============================================================================
Implemented: false

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
Task 16: Lighting uniform buffer
==============================================================================
Implemented: false

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
Task 17: Lighting shader integration and blit pass
==============================================================================
Implemented: false

Integrate the lighting UBO into the flat and textured fragment shaders:
- Both shaders #include lighting.glsl (or inline the shared code)
- Read ambient + lights from the UBO bound at set 0, binding 0
- calcLighting() function: per-pixel Lambertian diffuse, quadratic
  attenuation, spot-light angular falloff — same math as GLRenderer

Implement endFrame():
1. End the off-screen dynamic rendering pass
2. Transition off-screen image: COLOR_ATTACHMENT → SHADER_READ_ONLY
3. Transition swap chain image: UNDEFINED → COLOR_ATTACHMENT
4. Begin dynamic rendering targeting the swap chain image
5. Bind the blit pipeline (fullscreen quad)
6. Bind descriptor set with the off-screen image as sampled texture
7. Draw fullscreen quad (6 vertices in NDC)
8. Set m_worldPass = false (subsequent draws go directly to swap chain
   without lighting — for UI overlay)

After endFrame(), any drawRect/drawText/etc. calls (UI) should render
directly to the swap chain image with full-brightness lighting (ambient=1,
numLights=0), matching GLRenderer's behavior.

display() must:
- End the swap chain dynamic rendering (if still active)
- Transition swap chain image for presentation
- Submit command buffer and present

==============================================================================
Task 18: Window and input management
==============================================================================
Implemented: false

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
Task 19: Camera and view system
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