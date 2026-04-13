==============================================================================
Implementation Notes
==============================================================================
This file contains pending tasks for agents to pick up. Each task should be
implemented by an individual agent, one at a time, in order.

When a task is completed:
1. Change "Implemented: false" to "Implemented: true" on the task
2. Move the completed task (the full block from ============ to the next
   ============) to CompletedTasks.md, appending it at the end of that file
3. If the task is the LAST task in a milestone section (the large banner
   blocks like "== VULKAN MIGRATION =="), also move the milestone header
   and its overview text to CompletedTasks.md. Nothing with
   "Implemented: true" or a fully-completed milestone should remain here.
4. Update architecture.md so future agents understand the current layout
5. Update anything in ReadMe.md that was changed by the task
6. Add new build artifacts to .gitignore
7. Commit and push with a descriptive message

IMPORTANT: When you finish, verify no completed tasks or finished milestone
sections remain in this file. This file should only contain pending work.

IMPORTANT: Never move a completed task back from CompletedTasks.md into
this file. Completed tasks are permanent history. If a previous
implementation needs to be changed, updated, or extended, create a NEW
task in this file that describes the desired changes. Reference the
original task if helpful (e.g., "Replaces behavior from Task 12").

See CompletedTasks.md for the history of all previously completed work.
See architecture.md for the current codebase layout.


