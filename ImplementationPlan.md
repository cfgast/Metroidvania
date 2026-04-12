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
==  MAP EDITOR — Multi-Map World Editing with Auto-Transitions             ==
==============================================================================
==============================================================================

The following tasks upgrade the C# WinForms map editor (tools/MapEditor/) to
support editing multiple maps simultaneously in a shared "world" view. Maps
can be placed next to each other and moved around. Transitions between maps
are automatically generated based on edge adjacency (bounds sharing an exact
edge). A new "world file" (JSON) stores which maps are loaded and their
global positions. Individual map files keep their own local coordinates.

Key design decisions:
- World file stores map paths + global offsets; map files unchanged
- Click a map to make it active (full opacity), others dimmed
- Auto-transitions REPLACE manual transition placement
- Adjacency = bounds share an exact edge (left/right or top/bottom)
- Both horizontal and vertical transitions supported

The tasks are ordered so the editor builds and runs after every task.

==============================================================================