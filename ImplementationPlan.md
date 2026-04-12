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
Task: Add world management UI - add, create, and remove maps from the world.
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