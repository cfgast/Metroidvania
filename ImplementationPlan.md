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
Task: Implement the OpenGL renderer core— GLRenderer with shader management, orthographic projection, and rectangle drawing.
Implemented: true

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