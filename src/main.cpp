#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <chrono>

#include <glm/vec2.hpp>

#include "Rendering/GLRenderer.h"
#include "Input/InputSystem.h"
#include "Core/GameObject.h"
#include "Core/PlayerState.h"
#include "Core/SaveSystem.h"
#include "Core/InputBindings.h"
#include "Components/InputComponent.h"
#include "Components/PhysicsComponent.h"
#include "Components/RenderComponent.h"
#include "Components/AnimationComponent.h"
#include "Components/HealthComponent.h"
#include "Components/EnemyAIComponent.h"
#include "Components/CombatComponent.h"
#include "Components/SlimeAttackComponent.h"
#include "Map/MapLoader.h"
#include "Map/TransitionManager.h"
#include "Debug/DebugMenu.h"
#include "Physics/PhysXWorld.h"
#include "UI/SaveSlotScreen.h"
#include "UI/PauseMenu.h"
#include "UI/ControlsMenu.h"
#include "Math/Types.h"

int main()
{
    PhysXWorld::instance().init();
    InputBindings::instance().load();

    GLRenderer renderer("Metroidvania", 800, 600, 60);
    renderer.setMouseCursorVisible(false);

    // --- Game state ---
    Map map;
    PlayerState playerState;
    std::string currentMapFile = "maps/world_01.json";
    int         activeSaveSlot = 1;

    const glm::vec2 playerSize{ 50.f, 50.f };
    std::vector<std::unique_ptr<GameObject>> enemies;
    GameObject player;

    // Helper: build a SaveData snapshot from current game state.
    auto buildSaveData = [&]() -> SaveData {
        SaveData sd;
        sd.playerPosition = player.position;
        sd.currentMapFile = currentMapFile;
        if (auto* hp = player.getComponent<HealthComponent>())
        {
            sd.currentHp = hp->getCurrentHp();
            sd.maxHp     = hp->getMaxHp();
        }
        sd.playerState = playerState;
        return sd;
    };

    auto performSave = [&]() {
        SaveData sd = buildSaveData();
        if (SaveSystem::save(activeSaveSlot, sd))
            std::cout << "Game saved to slot " << activeSaveSlot << '\n';
        else
            std::cerr << "Save failed for slot " << activeSaveSlot << '\n';
    };

    auto pruneConsumedPickups = [&](Map& m) {
        for (const auto& id : playerState.consumedPickups)
            m.removeAbilityPickup(id);
    };

    auto setupPlayer = [&]() {
        player.addComponent<InputComponent>();
        player.addComponent<RenderComponent>(playerSize.x, playerSize.y, 0.f, 1.f, 0.f);
        player.addComponent<PhysicsComponent>(map, playerSize, 300.f);
        if (auto* physics = player.getComponent<PhysicsComponent>())
            physics->setPlayerState(&playerState);
        auto* health = player.addComponent<HealthComponent>(100.f);
        health->onDeath = [&player, &map]() {
            if (auto* physics = player.getComponent<PhysicsComponent>())
                physics->teleport(map.getSpawnPoint());
            else
                player.position = map.getSpawnPoint();
            if (auto* hp = player.getComponent<HealthComponent>())
                hp->heal(hp->getMaxHp());
        };

        auto* anim = player.addComponent<AnimationComponent>();
        auto* combat = player.addComponent<CombatComponent>(17.f, 0.3f, 0.2f, 45.f);
        combat->setEnemies(&enemies);
        {
            const std::string atlas = "assets/player_spritesheet.png";
            const int fw = 50, fh = 50;
            auto makeFrames = [&](int row, int count) {
                std::vector<AnimationComponent::FrameRect> frames;
                for (int i = 0; i < count; ++i)
                    frames.push_back({i * fw, row * fh, fw, fh});
                return frames;
            };
            anim->addAnimation("idle",      atlas, makeFrames(0, 4), 0.20f);
            anim->addAnimation("run-right", atlas, makeFrames(1, 4), 0.10f);
            anim->addAnimation("run-left",  atlas, makeFrames(2, 4), 0.10f);
            anim->addAnimation("jump",      atlas, makeFrames(3, 2), 0.15f, false);
            anim->addAnimation("fall",      atlas, makeFrames(4, 2), 0.15f, false);
            anim->addAnimation("wall-slide-right", atlas, makeFrames(5, 2), 0.20f);
            anim->addAnimation("wall-slide-left",  atlas, makeFrames(6, 2), 0.20f);
            anim->addAnimation("dash-right",       atlas, makeFrames(7, 3), 0.05f, false);
            anim->addAnimation("dash-left",        atlas, makeFrames(8, 3), 0.05f, false);
            anim->play("idle");
        }
    };

    auto spawnEnemies = [&](Map& m, GameObject& p)
    {
        std::vector<std::unique_ptr<GameObject>> result;
        for (const auto& def : m.getEnemyDefinitions())
        {
            auto enemy = std::make_unique<GameObject>();
            enemy->position = def.position;
            enemy->addComponent<EnemyAIComponent>(p, def.waypointA, def.waypointB,
                                                  def.damage, 0.5f);
            enemy->addComponent<InputComponent>();
            enemy->addComponent<RenderComponent>(def.size.x, def.size.y, 0.f, 1.f, 0.f);
            enemy->addComponent<PhysicsComponent>(m, def.size, def.speed);
            enemy->addComponent<HealthComponent>(def.hp);
            enemy->addComponent<SlimeAttackComponent>(p, 5.f);

            auto* anim = enemy->addComponent<AnimationComponent>();
            {
                const std::string atlas = "assets/slime_spritesheet.png";
                const int fw = 40, fh = 40;
                auto makeFrames = [&](int row, int count) {
                    std::vector<AnimationComponent::FrameRect> frames;
                    for (int i = 0; i < count; ++i)
                        frames.push_back({i * fw, row * fh, fw, fh});
                    return frames;
                };
                anim->addAnimation("idle",       atlas, makeFrames(0, 4), 0.25f);
                anim->addAnimation("move-right", atlas, makeFrames(1, 4), 0.12f);
                anim->addAnimation("move-left",  atlas, makeFrames(2, 4), 0.12f);
                anim->addAnimation("jitter",     atlas, makeFrames(3, 4), 0.05f);
                anim->play("idle");
            }
            result.push_back(std::move(enemy));
        }
        return result;
    };

    auto loadMapAndSetup = [&](const std::string& mapFile, glm::vec2 spawnPos) {
        currentMapFile = mapFile;
        enemies.clear();
        map = MapLoader::loadFromFile(mapFile);
        map.registerPhysXStatics();
        pruneConsumedPickups(map);
        if (auto* physics = player.getComponent<PhysicsComponent>())
            physics->teleport(spawnPos);
        else
            player.position = spawnPos;
        enemies = spawnEnemies(map, player);
    };

    // Save-slot selection screen (shown at launch)
    SaveSlotScreen saveSlotScreen;
    saveSlotScreen.open();
    bool gameStarted = false;

    // --- Room transition manager ---
    TransitionZone pendingZone;
    glm::vec2      pendingPlayerPos{0.f, 0.f};
    float          transitionCooldown = 0.f;

    TransitionManager transitionMgr;
    transitionMgr.setLoadCallback(
        [&](const std::string& targetFile, const std::string& targetSpawn)
        {
            try
            {
                enemies.clear();
                map = MapLoader::loadFromFile(targetFile);
                currentMapFile = targetFile;
                map.registerPhysXStatics();
                pruneConsumedPickups(map);

                glm::vec2 spawn;
                if (!pendingZone.edgeAxis.empty())
                {
                    if (pendingZone.edgeAxis == "vertical")
                    {
                        spawn.x = pendingZone.targetBaseX;
                        spawn.y = pendingZone.targetBaseY
                                + (pendingPlayerPos.y - pendingZone.bounds.y);
                        spawn.y = std::clamp(spawn.y,
                                             pendingZone.targetBaseY,
                                             pendingZone.targetBaseY + pendingZone.bounds.height - playerSize.y);
                    }
                    else // "horizontal"
                    {
                        spawn.x = pendingZone.targetBaseX
                                + (pendingPlayerPos.x - pendingZone.bounds.x);
                        spawn.y = pendingZone.targetBaseY;
                        spawn.x = std::clamp(spawn.x,
                                             pendingZone.targetBaseX,
                                             pendingZone.targetBaseX + pendingZone.bounds.width - playerSize.x);
                    }
                }
                else
                {
                    spawn = map.getNamedSpawn(targetSpawn);
                }

                if (auto* physics = player.getComponent<PhysicsComponent>())
                    physics->teleport(spawn);
                else
                    player.position = spawn;

                enemies = spawnEnemies(map, player);

                // Brief cooldown so the player isn't immediately re-triggered
                // by the return transition zone they spawn inside of.
                transitionCooldown = 0.3f;

                // Auto-save on room transition.
                performSave();
            }
            catch (const std::exception& e)
            {
                std::cerr << "Transition failed: " << e.what() << '\n';
            }
        });

    float viewW = 800.f;
    float viewH = 600.f;
    float halfW = viewW / 2.f;
    float halfH = viewH / 2.f;

    // Simple chrono-based clock for frame timing
    struct GameClock {
        std::chrono::high_resolution_clock::time_point last =
            std::chrono::high_resolution_clock::now();
        float restart() {
            auto now = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration<float>(now - last).count();
            last = now;
            return dt;
        }
    };

    GameClock clock;
    DebugMenu  debugMenu;
    PauseMenu  pauseMenu;
    ControlsMenu controlsMenu;

    // Dash trail: ring buffer of recent player positions/alpha for ghost effect
    struct DashGhost {
        glm::vec2 position;
        float alpha;
    };
    std::vector<DashGhost> dashGhosts;
    float dashGhostSpawnTimer = 0.f;

    while (renderer.isOpen())
    {
        InputEvent event;
        auto& input = renderer.getInput();
        while (input.pollEvent(event))
        {
            if (event.type == InputEventType::WindowClosed)
            {
                renderer.close();
                break;
            }

            // Handle window resize: keep 1:1 pixel-to-world-unit mapping so
            // that resizing the window shows more (or less) of the map without
            // changing the zoom level.
            if (event.type == InputEventType::WindowResized)
            {
                viewW = static_cast<float>(event.width);
                viewH = static_cast<float>(event.height);
                halfW = viewW / 2.f;
                halfH = viewH / 2.f;
            }

            // --- Controls menu has highest priority when open ---
            if (controlsMenu.isOpen())
            {
                controlsMenu.handleEvent(event);
                continue;
            }

            // --- Save-slot screen has priority ---
            if (saveSlotScreen.isOpen())
            {
                SaveSlotResult slotResult = saveSlotScreen.handleEvent(event, renderer);
                if (slotResult.action == SaveSlotResult::NewGame)
                {
                    activeSaveSlot = slotResult.slot;
                    saveSlotScreen.close();
                    playerState = PlayerState{};
                    currentMapFile = "maps/world_01.json";

                    if (!gameStarted)
                    {
                        map = MapLoader::loadFromFile(currentMapFile);
                        map.registerPhysXStatics();
                        pruneConsumedPickups(map);
                        player.position = map.getSpawnPoint();
                        setupPlayer();
                        enemies = spawnEnemies(map, player);
                        gameStarted = true;
                    }
                    else
                    {
                        loadMapAndSetup(currentMapFile, map.getSpawnPoint());
                    }
                    performSave();
                    clock.restart();
                }
                else if (slotResult.action == SaveSlotResult::LoadSlot)
                {
                    activeSaveSlot = slotResult.slot;
                    SaveData sd;
                    if (SaveSystem::load(activeSaveSlot, sd))
                    {
                        playerState    = sd.playerState;
                        currentMapFile = sd.currentMapFile;
                        saveSlotScreen.close();

                        if (!gameStarted)
                        {
                            map = MapLoader::loadFromFile(currentMapFile);
                            map.registerPhysXStatics();
                            pruneConsumedPickups(map);
                            player.position = sd.playerPosition;
                            setupPlayer();
                            if (auto* hp = player.getComponent<HealthComponent>())
                            {
                                hp->heal(hp->getMaxHp());
                                hp->takeDamage(hp->getMaxHp() - sd.currentHp);
                            }
                            enemies = spawnEnemies(map, player);
                            gameStarted = true;
                        }
                        else
                        {
                            loadMapAndSetup(currentMapFile, sd.playerPosition);
                            if (auto* hp = player.getComponent<HealthComponent>())
                            {
                                hp->heal(hp->getMaxHp());
                                hp->takeDamage(hp->getMaxHp() - sd.currentHp);
                            }
                        }
                        clock.restart();
                    }
                }
                else if (slotResult.action == SaveSlotResult::Controls)
                {
                    controlsMenu.open();
                }
                continue;
            }

            // --- Pause menu ---
            if (pauseMenu.isOpen())
            {
                PauseMenu::Action pa = pauseMenu.handleEvent(event);
                if (pa == PauseMenu::Resume)
                    clock.restart();
                else if (pa == PauseMenu::Save)
                {
                    performSave();
                    pauseMenu.close();
                    clock.restart();
                }
                else if (pa == PauseMenu::Quit)
                    renderer.close();
                continue;
            }

            // Escape opens the pause menu instead of closing the window.
            if (event.type == InputEventType::KeyPressed && event.key == KeyCode::Escape)
            {
                pauseMenu.open();
                continue;
            }

            // Controller Start button also opens the pause menu.
            if (event.type == InputEventType::GamepadButtonPressed
                && event.gamepadButton == GamepadButton::Start)
            {
                pauseMenu.open();
                continue;
            }

            debugMenu.handleEvent(event);
        }

        if (!renderer.isOpen())
            break;

        // Show the mouse cursor when any menu is open, hide it during gameplay
        bool anyMenuOpen = saveSlotScreen.isOpen() || pauseMenu.isOpen()
                         || controlsMenu.isOpen() || debugMenu.isOpen();
        renderer.setMouseCursorVisible(anyMenuOpen);

        // --- Controls menu rendering ---
        if (controlsMenu.isOpen())
        {
            renderer.clear(15/255.f, 15/255.f, 30/255.f);
            controlsMenu.render(renderer);
            renderer.display();
            continue;
        }

        // --- Save-slot screen rendering ---
        if (saveSlotScreen.isOpen())
        {
            renderer.clear(15/255.f, 15/255.f, 30/255.f);
            saveSlotScreen.render(renderer);
            renderer.display();
            continue;
        }

        if (!gameStarted)
            continue;

        // Reload map if the user selected one via the debug menu
        const std::string selectedMap = debugMenu.pollSelectedMap();
        if (!selectedMap.empty())
        {
            try
            {
                loadMapAndSetup(selectedMap, map.getSpawnPoint());
            }
            catch (const std::exception& e)
            {
                std::cerr << "Failed to load map: " << e.what() << '\n';
            }
        }

        // Pause gameplay while debug or pause menu is open
        float dt = 0.f;
        if (!debugMenu.isOpen() && !pauseMenu.isOpen())
            dt = clock.restart();
        else
            clock.restart();

        // Advance room transition (skips gameplay while active).
        if (transitionMgr.isActive())
        {
            transitionMgr.update(dt);

            const Rect mapBounds = map.getBounds();
            float cx = player.position.x;
            float cy = player.position.y;
            // When the view is wider/taller than the map, center on the map;
            // otherwise clamp so the camera stays within map edges.
            if (mapBounds.width <= halfW * 2.f)
                cx = mapBounds.x + mapBounds.width / 2.f;
            else
            {
                cx = std::max(cx, mapBounds.x  + halfW);
                cx = std::min(cx, mapBounds.x  + mapBounds.width  - halfW);
            }
            if (mapBounds.height <= halfH * 2.f)
                cy = mapBounds.y + mapBounds.height / 2.f;
            else
            {
                cy = std::max(cy, mapBounds.y   + halfH);
                cy = std::min(cy, mapBounds.y   + mapBounds.height - halfH);
            }

            renderer.beginFrame();
            renderer.clearLights();
            renderer.setView(cx, cy, viewW, viewH);
            renderer.clear(30/255.f, 30/255.f, 50/255.f);
            map.render(renderer);
            player.render(renderer);

            transitionMgr.render(renderer);

            renderer.endFrame();
            renderer.resetView();
            debugMenu.render(renderer);
            renderer.display();
            continue;
        }

        PhysXWorld::instance().beginFrame();

        player.update(dt);

        // --- Dash trail: spawn ghosts and fade existing ones ---
        {
            auto* physics = player.getComponent<PhysicsComponent>();
            if (physics && physics->isDashing())
            {
                dashGhostSpawnTimer -= dt;
                if (dashGhostSpawnTimer <= 0.f)
                {
                    dashGhosts.push_back({ player.position, 200.f });
                    dashGhostSpawnTimer = 0.02f;
                }
            }
            else
            {
                dashGhostSpawnTimer = 0.f;
            }
            // Fade and remove expired ghosts
            for (auto& g : dashGhosts)
                g.alpha -= 600.f * dt;
            dashGhosts.erase(
                std::remove_if(dashGhosts.begin(), dashGhosts.end(),
                               [](const DashGhost& g) { return g.alpha <= 0.f; }),
                dashGhosts.end());
        }

        for (auto& enemy : enemies)
        {
            auto* hp = enemy->getComponent<HealthComponent>();
            if (hp && hp->isDead())
                continue;
            enemy->update(dt);
        }

        if (transitionCooldown > 0.f)
            transitionCooldown -= dt;
        else if (const TransitionZone* zone = map.checkTransition(player.position, playerSize))
        {
            pendingZone      = *zone;
            pendingPlayerPos = player.position;
            transitionMgr.startTransition(zone->targetMap, zone->targetSpawn);
        }

        if (const AbilityPickupDefinition* pickup = map.checkAbilityPickup(player.position, playerSize))
        {
            playerState.unlockAbility(pickup->ability);
            playerState.consumePickup(pickup->id);
            map.removeAbilityPickup(pickup->id);
        }

        if (auto* animComp = player.getComponent<AnimationComponent>())
        {
            auto* physics  = player.getComponent<PhysicsComponent>();
            auto* inputComp = player.getComponent<InputComponent>();
            if (physics && inputComp)
            {
                const auto& inp = inputComp->getInputState();
                if (physics->isDashing())
                {
                    if (physics->facingRight())
                        animComp->play("dash-right");
                    else
                        animComp->play("dash-left");
                }
                else if (physics->isWallSliding())
                {
                    if (physics->wallDirection() > 0)
                        animComp->play("wall-slide-right");
                    else
                        animComp->play("wall-slide-left");
                }
                else if (!physics->isGrounded() && physics->velocity.y <= 0.f)
                    animComp->play("jump");
                else if (!physics->isGrounded() && physics->velocity.y > 0.f)
                    animComp->play("fall");
                else if (inp.moveLeft)
                    animComp->play("run-left");
                else if (inp.moveRight)
                    animComp->play("run-right");
                else
                    animComp->play("idle");
            }
        }

        const Rect mapBounds = map.getBounds();
        float cx = player.position.x;
        float cy = player.position.y;
        // When the view is wider/taller than the map, center on the map;
        // otherwise clamp so the camera stays within map edges.
        if (mapBounds.width <= halfW * 2.f)
            cx = mapBounds.x + mapBounds.width / 2.f;
        else
        {
            cx = std::max(cx, mapBounds.x  + halfW);
            cx = std::min(cx, mapBounds.x  + mapBounds.width  - halfW);
        }
        if (mapBounds.height <= halfH * 2.f)
            cy = mapBounds.y + mapBounds.height / 2.f;
        else
        {
            cy = std::max(cy, mapBounds.y   + halfH);
            cy = std::min(cy, mapBounds.y   + mapBounds.height - halfH);
        }
        renderer.beginFrame();
        renderer.clearLights();
        renderer.setView(cx, cy, viewW, viewH);
        renderer.clear(30/255.f, 30/255.f, 50/255.f);
        map.render(renderer);

        for (auto& enemy : enemies)
        {
            auto* hp = enemy->getComponent<HealthComponent>();
            if (hp && hp->isDead())
                continue;
            enemy->render(renderer);
        }

        // --- Render dash trail ghosts ---
        for (const auto& ghost : dashGhosts)
        {
            float a = std::max(0.f, ghost.alpha) / 255.f;
            renderer.drawRect(ghost.position.x - playerSize.x * 0.5f,
                              ghost.position.y - playerSize.y * 0.5f,
                              playerSize.x, playerSize.y,
                              100/255.f, 200/255.f, 1.f, a);
        }

        player.render(renderer);

        transitionMgr.render(renderer);

        renderer.endFrame();
        renderer.resetView();
        pauseMenu.render(renderer);
        debugMenu.render(renderer);
        renderer.display();
    }

    PhysXWorld::instance().shutdown();
    return 0;
}
