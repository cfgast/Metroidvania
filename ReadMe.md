==============================================================================
Project Description
==============================================================================
This project is a 4-8 mutliplayer metroidvania written in C++

==============================================================================
Project Parts
==============================================================================
The game should have a component based object structure that makes is easy to create game objects that are a group of funtionality.

Game Renderer: Vulkan 1.3 (via GLFW + VMA + vk-bootstrap)
Game Physics: Nvidia PhysX (integrated via PhysXWorld singleton; PhysX SDK in third_party/PhysX-src)

==============================================================================
Key Project Files
==============================================================================
- architecture.md        — Current codebase layout and design (read this first)
- ImplementationPlan.md  — Pending tasks for agents to pick up (one at a time)
- CompletedTasks.md      — History of all previously completed tasks
- tools/run-tasks.ps1    — Automation script that parses and runs tasks

==============================================================================
ImplementationPlan.md Format (IMPORTANT - read before editing)
==============================================================================
The task automation script (tools/run-tasks.ps1) parses ImplementationPlan.md
using regex. Tasks MUST follow this exact format or the parser will not
detect them:

    ==============================================================================
    Task N: Short title here
    ==============================================================================
    Implemented: false

    Description text...

    ==============================================================================

Rules:
- Each task header must be: "Task N: <title>" on its own line, surrounded
  by ============ separator lines above and below
- "Implemented: false" must be the very first line after the closing ======
  separator (no blank line between the separator and "Implemented:")
- When completing a task, change "false" to "true" on that same line
- Do NOT rename, reformat, or restructure the task headers
- Do NOT add extra lines between the ====== separator and "Implemented:"
- The title line must start with "Task" followed by a space and a number

Example of a completed task (before moving to CompletedTasks.md):

    ==============================================================================
    Task 1: Framebuffer infrastructure and light data structures
    ==============================================================================
    Implemented: true

==============================================================================
Architecture 
==============================================================================
After any task is completed we want to make sure the change it checked into the github with a good change log.

Also execute the following steps:
1. Update the Architechure.md document so future agents can quickly understand the layout of the program
2. Update anything in this Readme.md that's been changed by task implementation
3. Do a quick pass on areas changed in the code and do a cleanup pass to make sure no junk was left behind.
4. Move the completed task from ImplementationPlan.md to CompletedTasks.md (append it at the end)