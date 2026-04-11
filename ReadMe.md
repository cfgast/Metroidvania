==============================================================================
Project Description
==============================================================================
This project is a 4-8 mutliplayer metroidvania written in C++

==============================================================================
Project Parts
==============================================================================
The game should have a component based object structure that makes is easy to create game objects that are a group of funtionality.

Game Renderer: OpenGL 3.3 Core (via GLFW + GLAD)
Game Physics: Nvidia PhysX (integrated via PhysXWorld singleton; PhysX SDK in third_party/PhysX-src)

==============================================================================
Key Project Files
==============================================================================
- architecture.md        — Current codebase layout and design (read this first)
- ImplementationPlan.md  — Pending tasks for agents to pick up (one at a time)
- CompletedTasks.md      — History of all previously completed tasks

==============================================================================
Architechture 
==============================================================================
After any task is completed we want to make sure the change it checked into the github with a good change log.

Also execute the following steps:
1. Update the Architechure.md document so future agents can quickly understand the layout of the program
2. Update anything in this Readme.md that's been changed by task implementation
3. Do a quick pass on areas changed in the code and do a cleanup pass to make sure no junk was left behind.
4. Move the completed task from ImplementationPlan.md to CompletedTasks.md (append it at the end)