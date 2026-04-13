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

==============================================================================
==  EXPERIENCE & LEVELING SYSTEM                                            ==
==============================================================================

Add an experience and leveling system. The player earns XP by killing
enemies and levels from 1 to 5. Each level unlocks a visually distinct
piece of knight armor on the player character. At level 3, the player
unlocks a charged spin-slash attack. At level 5 (full armor), cool
ambient particles drift off the player.

Design overview:
- PlayerState gains xp (uint32_t) and level (uint32_t, 1-5)
- 5 kills per level (linear): each enemy awards 1 XP, 5 XP to level
- XP bar rendered as HUD element in screen-space
- Level text displayed next to XP bar ("Lv 3")
- On level-up: swap player sprites to the next armor tier
- Level and XP persist via SaveSystem (add to SaveData JSON)

Armor progression (each level adds a visually distinct knight armor piece):
  Level 1: Base character (no armor) -- current sprites
  Level 2: Boots -- armored greaves on lower legs
  Level 3: Boots + Leg armor -- full leg plates + unlocks spin slash
  Level 4: Boots + Legs + Chestplate -- torso covered in plate mail
  Level 5: Full knight armor (boots + legs + chest + gauntlets + helm)
           + ambient particle effect (sparks/embers drifting off)

Each armor tier needs its own complete set of sprite animations (idle,
run-right, run-left, jump, fall, wall-slide-right, wall-slide-left,
dash-right, dash-left, spin-slash-right, spin-slash-left). The sprites
should be visually distinct -- not just tinted versions of the base
character, but showing actual armor pieces drawn onto the character.

For the code implementation, generate the armor sprites procedurally at
build time or load time using the base sprite sheet as a template,
drawing armor overlays (colored rectangles/shapes) onto the character
pixels to approximate the look of each armor piece. This gives agents a
concrete visual result without requiring hand-drawn art.

==============================================================================
Task 29: Charged spin-slash attack
==============================================================================
Implemented: false

Add a charged spin-slash attack to CombatComponent, unlocked at level 3.

CombatComponent changes (src/Components/CombatComponent.h/.cpp):

Add new members:
  bool  m_chargeUnlocked = false;   // Set to true at level 3+
  bool  m_charging       = false;   // Currently holding attack button
  float m_chargeTimer    = 0.f;     // How long attack has been held
  float m_minChargeTime  = 0.6f;    // Minimum hold for spin slash
  float m_spinDamageMult = 2.0f;    // 2x normal damage
  float m_spinReachMult  = 1.5f;    // 1.5x normal reach

Add method:
  void setChargeUnlocked(bool unlocked);  // Called when player reaches lv3

Update the attack flow in update(dt):
1. When attack button is pressed (rising edge) and not on cooldown:
   - If charge is unlocked: enter charging state, start m_chargeTimer
   - If charge is NOT unlocked: start normal attack (existing behavior)
2. While charging (button held): increment m_chargeTimer each frame
3. When attack button is released (falling edge) while charging:
   - If m_chargeTimer >= m_minChargeTime: execute spin slash
     (wider hitbox, more damage, spin-slash animation)
   - If m_chargeTimer < m_minChargeTime: execute normal attack
4. If the player takes damage while charging, cancel the charge

Spin-slash hitbox:
- Same AABB approach as normal attack but reach *= m_spinReachMult
- Vertical size *= 1.2 (same as normal)
- Damage = m_damage * m_spinDamageMult

Spin-slash visual:
- Render a wider, more dramatic arc sweep than the normal attack
- Use a brighter color or thicker triangle strip
- Duration slightly longer than normal (0.4s vs 0.3s)

Charge visual feedback:
- While charging, render a growing glow/pulse around the player
- Could use drawCircle with increasing radius and pulsing alpha
- Or flash the player sprite at an increasing frequency

Add a "spin-slash-right" and "spin-slash-left" animation to the player's
AnimationComponent (can reuse existing frames with faster timing, or add
new placeholder frames). Play this animation during the spin slash.

Integration in main.cpp:
- After awardXP(), check if playerState.level >= 3
- If so, call combat->setChargeUnlocked(true)
- Also check on game load (in case saved at level 3+)

==============================================================================
