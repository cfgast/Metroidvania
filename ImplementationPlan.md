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

See CompletedTasks.md for the history of all previously completed work.
See architecture.md for the current codebase layout.

==============================================================================
==  EXPERIENCE & LEVELING SYSTEM                                            ==
==============================================================================

Add an experience and leveling system. The player earns XP by killing
enemies and levels from 1 to 5. Each level unlocks a visual armor piece
on the player sprite. At level 3, the player unlocks a charged spin-slash
attack (hold X to charge, release to execute).

Design overview:
- PlayerState gains xp (uint32_t) and level (uint32_t, 1-5)
- 5 kills per level (linear): each enemy awards 1 kill's worth of XP
  (xpPerKill = 1, xpToLevel = 5 at every level)
- XP bar rendered as HUD element (screen-space, below HP or at bottom)
- Level text displayed next to XP bar ("Lv 3")
- On level-up: update player sprite to the next armor tier
- Armor visuals: 5 rows of sprites in a new spritesheet (or extend the
  existing player_spritesheet.png with armor overlay rows)
- Level and XP persist via SaveSystem (add to SaveData JSON)

Armor progression (cosmetic, one piece added per level):
  Level 1: Base character (no armor) -- current sprites
  Level 2: Boots -- new sprite row set
  Level 3: Boots + Legs -- new sprite row set + unlocks spin slash
  Level 4: Boots + Legs + Chest -- new sprite row set
  Level 5: Boots + Legs + Chest + Arms + Helm (full armor) -- new sprite set

Charged spin-slash attack (unlocked at level 3):
- Hold the attack key (X / gamepad X) to charge
- Visual charge indicator (glow or particle effect on player)
- On release after minimum charge time (0.6s), execute a forward spin slash
- Spin slash has wider reach (1.5x normal) and higher damage (2x normal)
- Non-looping "spin-slash" animation plays during the attack
- If released before minimum charge time, do a normal attack instead
- Cannot charge while already attacking or during cooldown

Architecture notes for agents:
- PlayerState stores xp/level -- central place for progression data
- CombatComponent needs a charge mechanic: track hold duration, detect
  release edge, switch between normal and spin-slash attacks
- InputComponent.InputState needs no changes -- just use the existing
  `attack` bool and detect hold vs tap in CombatComponent
- XP award happens in main.cpp's dead-enemy detection loop -- when an
  enemy first becomes dead, award XP to PlayerState
- Sprite switching: AnimationComponent already supports multiple named
  animations. On level-up, swap to the new armor tier's animation set
  by re-calling addAnimation() with the new sprite atlas rows
- SaveSystem: add "xp" and "level" fields to the JSON, with defaults
  for backward compatibility (missing = level 1, xp 0)

==============================================================================
Task 28: Charged spin-slash attack
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