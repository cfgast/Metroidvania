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
Task: Implement map focus/activation by clicking.
Implemented: true

Details:
- In MapCanvas, when the user clicks (left mouse, Select tool) on empty
  space or on an element that belongs to a non-active map, switch the
  ActiveMap to that map.

- Add a method: EditorMap HitMap(PointF worldPoint)
    - Returns the topmost EditorMap whose world-offset bounds contain
      the given world point.
    - If the point is inside the active map's bounds, prefer the active
      map (don't switch away accidentally).
    - If multiple inactive maps overlap at the point, return the one
      rendered on top (last in list).

- Update OnMouseDown (Select tool):
    1. First, try hit-testing the ActiveMap's elements (handles,
       waypoints, spawns, platforms, enemies, transitions, pickups)
       as before.
    2. If nothing is hit on the active map, call HitMap() to check
       if the click is inside another map's bounds.
    3. If another map is found, switch ActiveMap to it, clear selection,
       fire SelectionChanged event, and repaint.
    4. If no map is hit, just clear selection as before.

- When ActiveMap changes:
    - Clear all selections
    - Fire SelectionChanged so the properties panel refreshes
    - Repaint canvas to update active/inactive highlighting
    - Update MainForm status bar to show active map name

- In MainForm, update the status bar left text to include the active
  map's name (e.g., "Active: World 1 - Starting Area | 18 platforms").

- Update the Map Settings section of the properties panel to show the
  active map's name and bounds (it should already work if it reads
  from ActiveMap.Map, but verify).

==============================================================================
Task: Add a "Move Map" tool for repositioning maps in world space.
Implemented: false

Details:
- Add a new EditorTool enum value: EditorTool.MoveMap

- In MainForm, add a "Move Map" toolbar button with hotkey [M]:
    - Icon/label: "Move Map [M]"
    - Positioned after the existing tool buttons
    - Add keyboard shortcut handler for 'M' key

- In MapCanvas, implement the MoveMap tool:
    OnMouseDown (MoveMap tool, left click):
      - Call HitMap() to find which map is under the cursor
      - If a map is found, start dragging:
        - Store the hit map as _draggingMap
        - Store the mouse offset from the map's world origin
        - Switch ActiveMap to the dragged map
        - Set cursor to SizeAll

    OnMouseMove (if _draggingMap != null):
      - Compute new world position from mouse position minus stored offset
      - If grid snap is enabled, snap the position to 10-unit grid
      - Update _draggingMap.WorldX and _draggingMap.WorldY
      - Mark the world as dirty
      - Repaint canvas

    OnMouseUp:
      - Clear _draggingMap
      - Reset cursor
      - Trigger auto-transition recalculation (for now, just mark a
        flag _transitionsNeedRegen = true; Task 5 will implement the
        actual recalculation)

- Visual feedback while dragging:
    - Draw the map bounds as a dashed outline during drag
    - Show coordinate tooltip near cursor: "(X, Y)"

- The map's internal coordinates do NOT change — only WorldX/WorldY
  (the offset in the world file) is modified.

==============================================================================
Task: Implement adjacency detection and auto-transition generation engine.
Implemented: false

Details:
- Create a new static class or method: TransitionGenerator (can be in
  Models.cs or a new file TransitionGenerator.cs).

- Implement edge adjacency detection:
    Given two EditorMaps (with their world offsets), detect if their
    bounds share an exact edge. A shared edge means:

    Right-Left adjacency (mapA's right edge touches mapB's left edge):
      - mapA.WorldX + mapA.Bounds.X + mapA.Bounds.Width == mapB.WorldX + mapB.Bounds.X
      - AND their Y ranges overlap: compute the overlapping vertical
        segment between [mapA.Bounds.Y, mapA.Bounds.Y + mapA.Bounds.Height]
        and [mapB.Bounds.Y, mapB.Bounds.Y + mapB.Bounds.Height]
        (all offset by respective WorldY)
      - Overlap segment must be > 0 height

    Top-Bottom adjacency (mapA's bottom edge touches mapB's top edge):
      - mapA.WorldY + mapA.Bounds.Y + mapA.Bounds.Height == mapB.WorldY + mapB.Bounds.Y
      - AND their X ranges overlap: compute the overlapping horizontal
        segment similarly
      - Overlap segment must be > 0 width

    Check all four orientations (A-right↔B-left, A-left↔B-right,
    A-bottom↔B-top, A-top↔B-bottom).

- For each detected adjacency, generate:

    For horizontal adjacency (left/right):
      - In mapA: a TransitionData zone along the shared edge
        - Width: 50 units (extends inward from the edge into mapA)
        - Height: the length of the overlapping segment
        - Position: at the shared edge, spanning the overlap
        - Coordinates in mapA's local space (subtract mapA's world offset)
        - Name: "to_<mapB_filename_without_extension>"
        - TargetMap: mapB's file path (relative)
        - TargetSpawn: "from_<mapA_filename_without_extension>"
      - In mapB: a matching TransitionData zone (mirror of above)
      - In mapA: a named SpawnPoint "from_<mapB_filename>" positioned
        just inside the transition zone (e.g., 60 units from the edge)
      - In mapB: a matching named SpawnPoint

    For vertical adjacency (top/bottom):
      - Same logic but oriented horizontally
      - Width: overlapping segment length
      - Height: 50 units (extends inward)
      - Spawn points 60 units from the edge vertically

- Implement RegenerateTransitions(List<EditorMap> allMaps):
    - Clear all existing transitions and auto-generated spawn points
      from every map (since auto-transitions replace manual ones)
    - Check all pairs of maps for adjacency
    - Generate transition zones and spawn points for each adjacency
    - Store generated data directly in each map's MapData.Transitions
      and MapData.SpawnPoints

- Call RegenerateTransitions() from MapCanvas whenever:
    - A map is moved (MoveMap tool mouse-up)
    - A map's bounds are changed (Apply in properties panel)
    - A map is added to or removed from the world

- In MapCanvas.OnPaint, render auto-transitions the same way as
  before (semi-transparent blue rectangles with dashed borders and
  labels showing the target map name).

==============================================================================
Task: Remove manual transition tool and integrate auto-transitions with save.
Implemented: false

Details:
- Remove the manual transition drawing tool:
    - Remove EditorTool.DrawTransition enum value (or leave it but
      remove it from the toolbar)
    - Remove the "Draw Transition [T]" toolbar button
    - Remove the 'T' keyboard shortcut
    - Remove transition-specific hit-testing from Select tool (users
      cannot select/move/resize auto-transitions)
    - Remove the Transition properties panel from MainForm
    - Remove the "Delete" functionality for transitions

- Auto-transitions should be displayed but NOT editable:
    - Render them in OnPaint as before (blue semi-transparent rects)
    - Add a small label or icon indicating they are auto-generated
    - They cannot be selected, moved, or deleted by the user

- Update SaveFile() / SaveWorld():
    - When saving each map file, the auto-generated transitions and
      spawn points are already in MapData (placed there by
      RegenerateTransitions), so they serialize naturally.
    - Ensure targetMap paths are relative to the maps/ directory
      (e.g., "maps/world_02.json") for consistency with the game
      runtime's MapLoader expectations.
    - Ensure auto-generated spawn points don't overwrite user-created
      spawn points with different names.

- Update LoadWorld():
    - After loading all maps, call RegenerateTransitions() to ensure
      transitions match the current map positions.
    - This means any manually-placed transitions from old map files
      are replaced by auto-generated ones on load.

- When opening a single map file (not a world), don't generate
  transitions (there's only one map, no neighbors).

==============================================================================
Task: Add world management UI — add, create, and remove maps from the world.
Implemented: false

Details:
- Add a Maps panel to MainForm (left sidebar, below the properties
  panel, or as a collapsible section):
    - ListBox or ListView showing all maps in the current world
    - Each entry shows: map name (from MapData.Name) and file path
    - Active map is highlighted/bold
    - Clicking a map entry sets it as the ActiveMap in the canvas

- Add toolbar buttons and/or menu items:
    "Add Map..." button:
      - Opens a file dialog to select an existing .json map file
      - Loads the map and adds it to the world
      - Default position: to the right of the rightmost existing map
        (or at origin if world is empty)
      - Calls RegenerateTransitions() after adding
      - Calls FitToView() to show the new layout

    "New Map" button:
      - Creates a blank map (default bounds, one floor platform)
      - Prompts for a file name/path to save it
      - Adds to world at default position (right of rightmost map)
      - Calls RegenerateTransitions()

    "Remove Map" button:
      - Removes the active map from the world (does NOT delete the
        file from disk)
      - Prompts for confirmation if map has unsaved changes
      - Switches ActiveMap to next available map
      - Calls RegenerateTransitions()

- Update the title bar:
    - Show world file name (e.g., "my_world.world.json")
    - Show asterisk (*) if any map or the world has unsaved changes

- Update dirty tracking:
    - World is dirty if any EditorMap.IsDirty is true OR if the world
      structure changed (maps added/removed/repositioned)
    - ConfirmDiscard() checks world-level dirty state

- Update "Fit to View" (F key):
    - Encompasses all maps in the world with their world offsets
    - 12% margin around the total bounding box

- Keyboard shortcut summary update:
    - M: Move Map tool
    - Ctrl+Shift+N/O/S: World New/Open/Save
    - All existing shortcuts remain for single-map editing

==============================================================================