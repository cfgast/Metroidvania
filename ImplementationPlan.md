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
Task 25: XP award on enemy kill
==============================================================================
Implemented: false

In main.cpp's enemy update loop (the dead-enemy detection block), award
XP when an enemy is first detected as dead:

Currently, the code detects isDead() and queues respawns. Add XP award
BEFORE the respawn queue check, but only once per death. Use the existing
`alreadyQueued` check -- if the enemy is dead and NOT yet in the respawn
queue, it's a fresh kill:

  if (hp && hp->isDead())
  {
      bool alreadyQueued = false;
      for (const auto& entry : respawnQueue)
          if (entry.definitionIndex == i) { alreadyQueued = true; break; }
      if (!alreadyQueued && i < defs.size())
      {
          playerState.awardXP(1);   // <-- NEW: award 1 XP per kill
          respawnQueue.push_back({i, ENEMY_RESPAWN_TIME});
      }
      continue;
  }

This awards exactly 1 XP per enemy death. With xpToLevel = 5, the player
levels up every 5 kills. Respawned enemies award XP again when re-killed.

==============================================================================
Task 26: XP bar and level HUD
==============================================================================
Implemented: false

Render an XP bar and level indicator in the screen-space UI overlay (after
endFrame() / resetView(), alongside existing UI elements).

XP bar design:
- Position: bottom-left of screen (e.g., x=20, y=windowH-40)
- Size: 200x10 pixels
- Background: dark gray with black outline (drawRectOutlined)
- Fill: gold/yellow color, width = (xp / xpToLevel) * barWidth
- When at max level (5), bar is full and a different color (e.g. cyan)

Level indicator:
- Text "Lv N" rendered to the left of or above the XP bar
- Use the existing font handle and drawText()
- Font size ~18, white or gold color

Implementation location: add to main.cpp's UI rendering section, after
the existing HUD elements. Use renderer.drawRectOutlined() for the bar
background, renderer.drawRect() for the fill, and renderer.drawText()
for the level label.

Only render when gameStarted is true and not on the save-slot screen.

==============================================================================
Task 27: Knight armor sprite generation
==============================================================================
Implemented: false

Generate 4 additional player sprite sheets (armor tiers 2-5) that show
the character wearing progressively more pieces of knight armor. Each
tier must be visually distinct from the others -- not just color tints.

Current sprite layout:
- assets/player_spritesheet.png: 50x50 pixel frames, 9 rows:
    Row 0: idle (4 frames)
    Row 1: run-right (4 frames)
    Row 2: run-left (4 frames)
    Row 3: jump (2 frames)
    Row 4: fall (2 frames)
    Row 5: wall-slide-right (2 frames)
    Row 6: wall-slide-left (2 frames)
    Row 7: dash-right (3 frames) / spin-slash-right
    Row 8: dash-left (3 frames) / spin-slash-left

Create a Python or C++ tool (tools/generate_armor_sprites.py recommended)
that loads the base player_spritesheet.png and generates 4 new sheets:
  assets/player_armor_2.png  -- Boots: draw armored boot shapes on the
    character's lower legs/feet area. Use a metallic gray with darker
    outlines. The boots should be clearly visible as distinct armor.
  assets/player_armor_3.png  -- Boots + Leg plates: add leg armor
    (greaves/cuisses) covering the thigh and shin areas on top of boots.
    Slightly different shade or added rivets/plate lines.
  assets/player_armor_4.png  -- Boots + Legs + Chestplate: add a
    breastplate/cuirass covering the torso with a distinct plate mail
    look (horizontal plate lines, shoulder guards).
  assets/player_armor_5.png  -- Full knight armor: add gauntlets on arms
    and a visored helm on the head. The character should look like a
    fully armored knight. Use brighter/shinier metallic tones.

Sprite generation approach:
- Load base sheet with stb_image (already in the project)
- For each frame in each row, identify the character silhouette pixels
  (non-transparent pixels) and overlay armor shapes in the appropriate
  body regions (feet, legs, torso, arms, head)
- Armor shapes can be simple: colored rectangles/regions overlaid with
  darker border pixels to suggest plate edges
- The key is that each tier looks DIFFERENT, not just more tinted
- Save each sheet with stb_image_write

The tool should be runnable standalone and also integrated into CMake so
the sheets are regenerated if the base sheet changes.

Update main.cpp to select the correct sprite sheet based on player level:
- Add a helper function updatePlayerArmorSprites(AnimationComponent*,
  PlayerState&) that re-registers all animations using the correct
  atlas path for the current level:
    Level 1: "assets/player_spritesheet.png"
    Level 2: "assets/player_armor_2.png"
    Level 3: "assets/player_armor_3.png"
    Level 4: "assets/player_armor_4.png"
    Level 5: "assets/player_armor_5.png"
- Call this function on level-up (after awardXP()) and on game load

==============================================================================
Task 28: Max-level particle effect
==============================================================================
Implemented: false

When the player reaches level 5 (full knight armor), add a persistent
ambient particle effect -- small glowing sparks or embers that drift
off the player character, giving a "legendary" feel.

Create a simple ParticleEmitter component or a lightweight particle
system embedded in main.cpp that:
- Spawns 2-4 small particles per second around the player position
- Each particle:
  - Starts at a random offset within the player bounding box
  - Drifts upward and slightly sideways (randomized velocity)
  - Fades from bright (gold/white, alpha=0.8) to transparent over 1-2s
  - Size: 2-4 pixels (small sparkle)
- Uses the existing drawRect() or drawCircle() API for rendering
  (no new textures needed)
- Particles are rendered in world space (before endFrame(), so they
  benefit from lighting) at the player's position

Implementation:
- Add a simple Particle struct: { vec2 pos, vec2 vel, float life,
  float maxLife, float size }
- Store a vector<Particle> in main.cpp (or a new ParticleComponent)
- In the update loop, if playerState.level >= 5:
  - Spawn new particles at a timed interval
  - Update existing particles (move, age, remove dead ones)
- In the render loop, draw surviving particles as small colored circles
  or rects with alpha based on remaining life

Particle colors (cycle/randomize between):
- Warm gold: (1.0, 0.85, 0.3)
- White-hot: (1.0, 1.0, 0.9)
- Ember orange: (1.0, 0.6, 0.2)

The effect should be subtle but noticeable -- a visual reward for
reaching max level, not an overwhelming particle storm.

Disable particles when the player is below level 5 (including after
death/respawn if level is preserved). Re-enable on game load if the
saved level is 5.

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
