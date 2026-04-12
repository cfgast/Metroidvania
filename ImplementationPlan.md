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
==  RELATIVE POSITION TRANSITIONS                                          ==
==============================================================================
==============================================================================

Currently, transitions teleport the player to a fixed named spawn point in
the target map. These tasks change auto-generated transitions to use
RELATIVE POSITIONING: the player's offset along the shared edge is preserved
so they appear at the corresponding position on the adjacent map.

Manual (non-auto) transitions still use targetSpawn for point-to-point
teleportation. The targetSpawn mechanism remains functional for backward
compatibility and for cases where a designer wants a fixed landing point.

Key design:
- New TransitionZone fields: edgeAxis, targetBaseX, targetBaseY
- edgeAxis = "vertical" (left/right adjacency) or "horizontal" (top/bottom)
- targetBaseX/Y = position in target map where the source zone origin maps to
- Runtime: target_pos[along_edge] = targetBase[along_edge] + (player[along_edge] - zone.origin[along_edge])
- Runtime: target_pos[perpendicular] = targetBase[perpendicular] (fixed entry side)
- If edgeAxis is empty/missing, falls back to targetSpawn (backward compat)
- Editor auto-transitions stop generating "from_*" spawn points
- Existing "from_*" spawn points removed from current map files

==============================================================================