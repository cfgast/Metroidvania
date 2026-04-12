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
Task: Implement relative positioning in the C++ game runtime transition callback.
Implemented: false

Details:
- In src/main.cpp, update the TransitionManager load callback (the lambda
  passed to transitionMgr.setLoadCallback). Currently it does:

    glm::vec2 spawn = map.getNamedSpawn(targetSpawn);
    physics->teleport(spawn);

  Change this to also receive the TransitionZone data. The cleanest
  approach: store the triggering zone's data before starting the
  transition, then use it in the callback.

  Before the transition starts (where checkTransition is called):
    - When a transition is detected, save the zone data and the
      player's current position:
        pendingZone = *zone;  // copy the TransitionZone
        pendingPlayerPos = player.position;

  In the load callback:
    - After loading the new map, compute the target position:

      glm::vec2 spawn;
      if (!pendingZone.edgeAxis.empty())
      {
          // Relative positioning mode
          if (pendingZone.edgeAxis == "vertical")
          {
              // Shared edge is vertical (left/right adjacency)
              // X is fixed (entry side), Y is relative
              spawn.x = pendingZone.targetBaseX;
              spawn.y = pendingZone.targetBaseY
                      + (pendingPlayerPos.y - pendingZone.bounds.y);
          }
          else // "horizontal"
          {
              // Shared edge is horizontal (top/bottom adjacency)
              // Y is fixed (entry side), X is relative
              spawn.x = pendingZone.targetBaseX
                      + (pendingPlayerPos.x - pendingZone.bounds.x);
              spawn.y = pendingZone.targetBaseY;
          }
      }
      else
      {
          // Legacy mode: use fixed spawn point
          spawn = map.getNamedSpawn(targetSpawn);
      }

      physics->teleport(spawn);

  Clamp the relative position to stay within the target zone's range
  so the player can't appear outside map bounds:
    - For vertical edge: clamp spawn.y to [targetBaseY, targetBaseY + zone.bounds.height - playerSize.y]
    - For horizontal edge: clamp spawn.x to [targetBaseX, targetBaseX + zone.bounds.width - playerSize.x]

- Also consider: the TransitionManager currently only passes
  (targetMap, targetSpawn) to the callback. You may need to either:
  (a) Store pendingZone/pendingPlayerPos as variables captured by the
      lambda in main.cpp (simplest, since they're in main scope), or
  (b) Extend TransitionManager to store and pass the zone data.
  Option (a) is recommended for minimal changes.

- Build the C++ game and test by running it:
    - Load world_01, walk to the right edge transition to world_02
    - Verify the player appears at the corresponding Y position on
      the other side (not at a fixed spawn point)
    - Walk back from world_02 to world_01 and verify the same behavior
    - Test vertical transitions if any exist

==============================================================================
Task: Remove legacy from_* spawn points from existing map files and update editor to regenerate clean transitions.
Implemented: false

Details:
- Update the three existing map files to remove auto-generated spawn
  points and old transition data. The editor will regenerate clean
  transitions with the new relative positioning fields on next save.

  maps/world_01.json:
    - Remove spawn points: "from_world_02", "from_world_03"
    - Keep: "default" spawn point
    - Remove all entries from the "transitions" array (editor will
      regenerate them with edgeAxis/targetBase fields)

  maps/world_02.json:
    - Remove spawn points: "from_world_01"
    - Keep: "default" spawn point
    - Remove all entries from the "transitions" array

  maps/world_03.json:
    - Remove spawn points: "from_world_03" (this was malformed anyway)
    - Keep no spawn points if none remain (or keep "default" if it exists)
    - Remove all entries from the "transitions" array

- After editing the JSON files, open them in the map editor as a world,
  verify the maps load correctly, and save the world. This should
  regenerate the transitions with the new edgeAxis/targetBase fields
  and without from_* spawn points.

  If the editor is not runnable in this environment, manually add the
  correct transition entries to each map file based on the adjacency:

  world_01 right edge <-> world_02 left edge:
    In world_01.json add transition:
      name: "to_world_02", edgeAxis: "vertical",
      x/y/w/h: positioned at world_01's right edge spanning the overlap,
      targetMap: "maps/world_02.json",
      targetBaseX: 60 (60 units inside world_02's left edge),
      targetBaseY: top of overlap in world_02 local coords

    In world_02.json add matching transition:
      name: "to_world_01", edgeAxis: "vertical",
      x/y/w/h: positioned at world_02's left edge spanning the overlap,
      targetMap: "maps/world_01.json",
      targetBaseX: world_01 right edge - 60 (60 units inside from right),
      targetBaseY: top of overlap in world_01 local coords

  (Repeat for world_01 <-> world_03 if they share an edge)

- Update architecture.md:
    - In the Map JSON schema section, add the new transition fields
      (edgeAxis, targetBaseX, targetBaseY) with descriptions
    - Note that auto-transitions use relative positioning and manual
      transitions can still use targetSpawn for fixed landing points

- Build the C++ game and verify transitions still work correctly with
  the updated map files.

==============================================================================