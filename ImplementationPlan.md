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