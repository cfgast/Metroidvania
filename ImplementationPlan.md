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
==  OPENGL MIGRATION — Replace SFML with GLFW + OpenGL 3.3 Core + GLM     ==
==============================================================================
==============================================================================

The following tasks migrate the renderer from SFML's built-in drawing to a
custom OpenGL 3.3 Core pipeline. SFML is fully removed — GLFW handles
windowing/input, GLM replaces SFML vector/rect types, GLAD loads GL
functions, stb_image loads textures, and FreeType renders text.

The migration is ordered so the project builds and runs after every task.

==============================================================================
Task: Migrate the Component base classand all Component subclasses to render through the Renderer interface instead of sf::RenderWindow.
Implemented: false

Details:
- Files to modify: src/Core/Component.h, src/Core/GameObject.h, src/Core/GameObject.cpp, src/Components/RenderComponent.h, src/Components/RenderComponent.cpp, src/Components/AnimationComponent.h, src/Components/AnimationComponent.cpp, src/Components/HealthComponent.h, src/Components/HealthComponent.cpp, src/Components/CombatComponent.h, src/Components/CombatComponent.cpp, src/Components/SlimeAttackComponent.h, src/Components/SlimeAttackComponent.cpp
- In Component.h: forward-declare `class Renderer;` (from src/Rendering/Renderer.h). Change the virtual render signature from `virtual void render(sf::RenderWindow& window)` to `virtual void render(Renderer& renderer)`. Remove the sf::RenderWindow forward declaration.
- In GameObject.h/.cpp: change `void render(sf::RenderWindow&)` to `void render(Renderer&)`. Add the Renderer forward-declare / include.
- RenderComponent: replace `window.draw(shape)` with `renderer.drawRect()`. The shape's position, size, and fill color should be passed as floats. Remove sf::RectangleShape member — store position/size/color as plain floats or a small struct. Remove SFML includes.
- AnimationComponent: store texture handles (Renderer::TextureHandle) instead of sf::Texture. Replace `window.draw(m_sprite)` with `renderer.drawSprite(handle, ...)`. Remove sf::Sprite member. The frame rects (sf::IntRect) should become plain {x,y,w,h} structs. Remove SFML includes.
- HealthComponent: replace the two sf::RectangleShape members (bar background + fill) with direct `renderer.drawRect()` / `renderer.drawRectOutlined()` calls. Remove SFML includes.
- CombatComponent: replace the sf::VertexArray arc rendering with `renderer.drawTriangleStrip()` and `renderer.drawLines()` using Renderer::Vertex. Remove SFML includes from the .cpp.
- SlimeAttackComponent: replace sf::CircleShape particle rendering with `renderer.drawCircle()`. Remove SFML includes.
- After all changes, verify the project builds. The game will not run yet until main.cpp is updated (next task).

==============================================================================
Task: Migrate the Map rendering, TransitionManager, and all UI classes to use the Renderer interface instead of sf::RenderWindow.
Implemented: false

Details:
- Files to modify: src/Map/Map.h, src/Map/Map.cpp, src/Map/TransitionManager.h, src/Map/TransitionManager.cpp, src/UI/UIStyle.h, src/UI/RoundedRectangleShape.h, src/UI/PauseMenu.h, src/UI/PauseMenu.cpp, src/UI/SaveSlotScreen.h, src/UI/SaveSlotScreen.cpp, src/UI/ControlsMenu.h, src/UI/ControlsMenu.cpp, src/Debug/DebugMenu.h, src/Debug/DebugMenu.cpp

- Map::render(): change parameter to `Renderer&`. Replace sf::RectangleShape platform drawing with `renderer.drawRect()`. Replace pickup drawing with `renderer.drawRectOutlined()`. Remove SFML shape includes.

- TransitionManager::render(): change parameter to `Renderer&`. The fade overlay should use `renderer.resetView()` to draw in screen space, then `renderer.drawRect()` for the black overlay. Replace `window.getSize()` with `renderer.getWindowSize()`. Remove the sf::RectangleShape member. Remove SFML includes.

- UIStyle.h: This is the most complex file. Every helper function currently takes `sf::RenderWindow&` — change to `Renderer&`. Replace all sf::Color usage with inline float parameters or a small Color struct. Replace RoundedRectangleShape draws with `renderer.drawRoundedRect()`. Replace sf::Text draws with `renderer.drawText()` and `renderer.measureText()`. The sf::FloatRect return from drawMenuItem/drawMenuRow should become a simple struct {x, y, w, h}.

- RoundedRectangleShape.h: This file can be deleted entirely once UIStyle.h uses renderer.drawRoundedRect() instead. Remove it from includes.

- PauseMenu: replace sf::Font with Renderer::FontHandle. Remove sf::Text members — call renderer.drawText() directly. Replace sf::RectangleShape overlay with renderer.drawRect(). Replace layout() to use renderer.getWindowSize() instead of window.getSize(). The handleEvent() method still uses sf::Event — this is fine for now, it will be migrated in a later GLFW task. Change render() to take Renderer&. Remove SFML rendering includes (keep sf::Event, sf::Keyboard, sf::Joystick, sf::Mouse for now).

- SaveSlotScreen: same pattern as PauseMenu. Replace all sf::Font, sf::Text, sf::RectangleShape rendering with Renderer calls. Keep sf::Event/input includes for now. Change render() and handleEvent() rendering portions to use Renderer.

- ControlsMenu: same pattern as PauseMenu.

- DebugMenu: same pattern. Replace sf::Font, sf::Text, RoundedRectangleShape rendering with Renderer calls. Keep the Windows file dialog code unchanged.

- After all changes, the game code should exclusively call Renderer methods for drawing. No file outside src/Rendering/ should include SFML graphics headers (sf::Event, sf::Keyboard, sf::Joystick, sf::Mouse are still allowed temporarily).

==============================================================================
Task: Wire up the SFMLRenderer in main.cpp so the game runs through the Renderer abstraction end to end.
Implemented: false

Details:
- Files to modify: src/main.cpp
- Replace `sf::RenderWindow window(...)` with `SFMLRenderer renderer("Metroidvania", 800, 600, 60)`. Include src/Rendering/SFMLRenderer.h.
- Replace every `window.clear(...)` with `renderer.clear(r, g, b)` using float colors (0-1 range).
- Replace every `window.display()` with `renderer.display()`.
- Replace `window.setView(gameView)` / view manipulation with `renderer.setView(cx, cy, w, h)`.
- Replace the screen-space view resets (`window.setView(sf::View(sf::FloatRect(...)))`) with `renderer.resetView()`.
- Replace the dash ghost sf::RectangleShape rendering with `renderer.drawRect()`.
- Pass `renderer` instead of `window` to all render() calls: `player.render(renderer)`, `enemy->render(renderer)`, `map.render(renderer)`, `transitionMgr.render(renderer)`, `pauseMenu.render(renderer)`, `debugMenu.render(renderer)`, etc.
- For sf::Event handling: the SFMLRenderer should expose `bool pollEvent(sf::Event&)`, `bool isOpen()`, `void close()`, `void setMouseCursorVisible(bool)`, `sf::Vector2u getSize()` (these are window operations the renderer owns). Add these methods to the Renderer base class as needed (or add a getWindow() escape hatch on SFMLRenderer temporarily).
- For window resize events: the SFMLRenderer should update its internal view size.
- The sf::Clock usage can remain as-is for now (it comes from sfml-system, not sfml-graphics).
- After this task, the game should build, run, and be fully playable — but still using SFML under the hood via SFMLRenderer.
- This is the critical validation checkpoint: verify all rendering works identically (platforms, sprites, HP bars, combat arcs, slime particles, menus, transitions, dash ghosts).

==============================================================================
Task: Replace all SFML math types (sf::Vector2f, sf::FloatRect, sf::IntRect, sf::Color) with GLM or custom equivalents throughout the entire codebase.
Implemented: false

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

==============================================================================
Task: Replace SFML input and event types with a custom input abstraction, preparing for the GLFW switch.
Implemented: false

Details:
- Create src/Input/InputTypes.h with:
    - An enum class KeyCode mapping all keys currently used: A, D, W, S, Left, Right, Up, Down, Space, Escape, Return, Delete, F1, X, LShift, RShift, and all letter keys needed by InputBindings. Values should be simple integers that can be mapped from both SFML and GLFW key codes.
    - An enum class MouseButton { Left, Right, Middle }.
    - An enum class GamepadButton with values for the ~15 Xbox buttons used.
    - An enum class GamepadAxis { LeftX, LeftY, RightX, RightY, DPadX, DPadY }.
    - A struct InputEvent with a type enum { KeyPressed, KeyReleased, MouseMoved, MouseButtonPressed, MouseButtonReleased, GamepadButtonPressed, GamepadButtonReleased, GamepadAxisMoved, WindowClosed, WindowResized } and a union/struct of relevant data fields.

- Create src/Input/InputSystem.h with an abstract class:
    - virtual bool pollEvent(InputEvent& event) = 0;
    - virtual bool isKeyPressed(KeyCode key) const = 0;
    - virtual bool isGamepadConnected(int id = 0) const = 0;
    - virtual float getGamepadAxis(int id, GamepadAxis axis) const = 0;
    - virtual bool isGamepadButtonPressed(int id, GamepadButton btn) const = 0;
    - virtual void setMouseCursorVisible(bool visible) = 0;

- Create src/Input/SFMLInput.h/.cpp implementing InputSystem by wrapping sf::Event, sf::Keyboard, sf::Joystick, sf::Mouse. The SFMLRenderer should create and own the SFMLInput instance (since it owns the sf::RenderWindow). Add an accessor `InputSystem& getInput()` to Renderer.

- Migrate all event-handling code:
    - src/Components/InputComponent.cpp: replace sf::Keyboard::isKeyPressed() and sf::Joystick calls with InputSystem methods. Replace sf::Keyboard::Key references with KeyCode.
    - src/Core/InputBindings.h/.cpp: replace sf::Keyboard::Key with KeyCode in all bindings and the key-to-string mapping table. Update save/load to use the new key names.
    - src/UI/PauseMenu.h/.cpp: replace sf::Event with InputEvent in handleEvent().
    - src/UI/SaveSlotScreen.h/.cpp: same.
    - src/UI/ControlsMenu.h/.cpp: same.
    - src/Debug/DebugMenu.h/.cpp: same.
    - src/main.cpp: replace sf::Event polling with InputSystem::pollEvent(). Replace sf::Keyboard/sf::Joystick constants with KeyCode/GamepadButton.

- After this task, no file outside src/Input/SFMLInput.cpp and src/Rendering/SFMLRenderer.cpp should include any SFML header. The game should still build and run identically.

==============================================================================
Task: Add GLFW, GLAD, stb_image, and FreeType as project dependencies via CMake. Verify they compile and link.
Implemented: false

Details:
- Files to modify: CMakeLists.txt
- Add GLFW via FetchContent:
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(glfw)

- Add GLAD (OpenGL 3.3 Core). Generate the loader from https://glad.dav1d.de/ or use a FetchContent-compatible source. The simplest approach is to vendor the generated glad.c and glad.h / KHR/khrplatform.h into third_party/glad/. Add glad.c to the source list.
    target_include_directories(Metroidvania PRIVATE third_party/glad/include)
    Add third_party/glad/src/glad.c to the executable sources.

- Add stb_image: vendor stb_image.h into third_party/stb/. Create a third_party/stb/stb_image_impl.cpp with:
    #define STB_IMAGE_IMPLEMENTATION
    #include "stb_image.h"
    Add this .cpp to the source list. Add third_party/stb/ to include dirs.

- Add FreeType via FetchContent:
    FetchContent_Declare(freetype GIT_REPOSITORY https://github.com/freetype/freetype.git GIT_TAG VER-2-13-3 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(freetype)
    target_link_libraries(Metroidvania PRIVATE freetype)

- Link GLFW and OpenGL:
    target_link_libraries(Metroidvania PRIVATE glfw opengl32)

- Do NOT link sfml-graphics, sfml-window, or sfml-system yet (still needed by SFMLRenderer/SFMLInput during transition). Keep all existing SFML links.

- Create a minimal test: in a temporary main() or a separate test .cpp, verify:
    1. GLFW initializes and creates a window
    2. GLAD loads GL functions
    3. glGetString(GL_VERSION) returns 3.3+
    4. stb_image loads a PNG (e.g. assets/player_spritesheet.png)
    5. FreeType initializes and loads a font face
  Remove the test code after verification. The actual project should build with both SFML and the new deps linked.

==============================================================================
Task: Implement the OpenGL renderer core — GLRenderer with shader management, orthographic projection, and rectangle drawing.
Implemented: false

Details:
- Create src/Rendering/GLRenderer.h and src/Rendering/GLRenderer.cpp implementing the Renderer interface.
- The GLRenderer constructor should:
    1. Call glfwInit() and create a GLFWwindow with the given title, width, height
    2. Set OpenGL 3.3 Core profile hints before window creation
    3. Make the context current and call gladLoadGLLoader()
    4. Enable blending: glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    5. Store window dimensions, set up viewport

- Create src/Rendering/Shader.h/.cpp:
    - A Shader class that compiles vertex + fragment shader source strings
    - Methods: use(), setMat4(name, mat4), setVec4(name, vec4), setInt(name, int)
    - Error reporting via std::cerr with shader compilation log

- Implement a "flat color" shader pair:
    Vertex shader: takes vec2 position, applies a uniform mat4 projection, outputs gl_Position.
    Fragment shader: outputs a uniform vec4 color.

- Implement clear(): glClearColor + glClear(GL_COLOR_BUFFER_BIT)
- Implement display(): glfwSwapBuffers
- Implement setView(): build a glm::ortho projection matrix from (left, right, bottom, top) derived from center and size. Store as the current projection.
- Implement resetView(): set projection to glm::ortho(0, windowW, windowH, 0) (top-left origin to match SFML's convention).
- Implement getWindowSize(): return stored dimensions, update on resize callback.

- Implement drawRect(): create (or reuse) a VAO/VBO for a unit quad, bind the flat shader, set projection and a model transform (translate + scale), set color uniform, draw. Use a single persistent quad VAO that is reused for all rect draws.

- Implement drawRectOutlined(): draw the filled rect, then draw 4 thin quads for the outline edges (or use glLineWidth + GL_LINE_LOOP but line width > 1 is deprecated in core profile, so prefer quads).

- Add GLRenderer.cpp and Shader.cpp to CMakeLists.txt sources.
- Do NOT switch main.cpp to use GLRenderer yet. Just verify it compiles and the class can be instantiated.

==============================================================================
Task: Implement OpenGL texture loading and sprite rendering in GLRenderer.
Implemented: false

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

==============================================================================
Task: Implement OpenGL text rendering in GLRenderer using FreeType.
Implemented: false

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
Implemented: false

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
Implemented: false

Details:
- Create src/Input/GLFWInput.h and src/Input/GLFWInput.cpp implementing the InputSystem interface using GLFW callbacks.

- GLFW uses callbacks for input, not polling-then-queue like SFML. The implementation should:
    1. Register GLFW callbacks: glfwSetKeyCallback, glfwSetMouseButtonCallback, glfwSetCursorPosCallback, glfwSetWindowSizeCallback, glfwSetWindowCloseCallback, glfwSetJoystickCallback.
    2. Each callback pushes an InputEvent into an internal std::queue<InputEvent>.
    3. pollEvent() pops from the queue. Before popping, call glfwPollEvents() if the queue is empty.

- Implement isKeyPressed(): use glfwGetKey(window, glfwKeyCode). Create a mapping function from KeyCode enum → GLFW key constant.
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
- Do NOT switch to GLFWInput yet — just verify it compiles.

==============================================================================
Task: Switch the game from SFMLRenderer to GLRenderer and from SFMLInput to GLFWInput. Remove all SFML dependencies.
Implemented: false

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
Implemented: false

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
- Commit with a descriptive message summarizing the SFML→OpenGL migration.