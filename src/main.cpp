#include <iostream>
#include <vector>
#include <memory>

#include <SFML/Graphics.hpp>

#include "Core/GameObject.h"
#include "Components/InputComponent.h"
#include "Components/PhysicsComponent.h"
#include "Components/RenderComponent.h"
#include "Components/AnimationComponent.h"
#include "Components/HealthComponent.h"
#include "Components/EnemyAIComponent.h"
#include "Map/MapLoader.h"
#include "Debug/DebugMenu.h"
#include "Physics/PhysXWorld.h"

int main()
{
    PhysXWorld::instance().init();

    sf::RenderWindow window(sf::VideoMode(800, 600), "Metroidvania");
    window.setFramerateLimit(60);

    Map map;
    try
    {
        map = MapLoader::loadFromFile("maps/world_01.json");
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        PhysXWorld::instance().shutdown();
        return -1;
    }

    map.registerPhysXStatics();

    const sf::Vector2f playerSize{ 50.f, 50.f };

    GameObject player;
    player.position = map.getSpawnPoint();
    player.addComponent<InputComponent>();
    player.addComponent<RenderComponent>(playerSize, sf::Color::Green);
    player.addComponent<PhysicsComponent>(map, playerSize, 300.f);
    auto* health = player.addComponent<HealthComponent>(100.f);
    health->onDeath = [&player, &map]() {
        player.position = map.getSpawnPoint();
        if (auto* physics = player.getComponent<PhysicsComponent>())
            physics->velocity = { 0.f, 0.f };
        if (auto* hp = player.getComponent<HealthComponent>())
            hp->heal(hp->getMaxHp());
    };

    // --- Helper: create enemy GameObjects from map definitions ---
    auto spawnEnemies = [&](Map& m, GameObject& p)
    {
        std::vector<std::unique_ptr<GameObject>> enemies;
        for (const auto& def : m.getEnemyDefinitions())
        {
            auto enemy = std::make_unique<GameObject>();
            enemy->position = def.position;

            // AI must update before PhysicsComponent reads the InputState.
            enemy->addComponent<EnemyAIComponent>(p, def.waypointA, def.waypointB,
                                                  def.damage, 0.5f);
            enemy->addComponent<InputComponent>();
            enemy->addComponent<RenderComponent>(def.size, sf::Color::Red);
            enemy->addComponent<PhysicsComponent>(m, def.size, def.speed);
            enemy->addComponent<HealthComponent>(def.hp);
            enemies.push_back(std::move(enemy));
        }
        return enemies;
    };

    auto enemies = spawnEnemies(map, player);

    // --- Sprite-sheet animation setup ---
    auto* anim = player.addComponent<AnimationComponent>();
    {
        const std::string atlas = "assets/player_spritesheet.png";
        const int fw = 50, fh = 50;
        auto makeFrames = [&](int row, int count) {
            std::vector<sf::IntRect> frames;
            for (int i = 0; i < count; ++i)
                frames.emplace_back(i * fw, row * fh, fw, fh);
            return frames;
        };
        anim->addAnimation("idle",      atlas, makeFrames(0, 4), 0.20f);
        anim->addAnimation("run-right", atlas, makeFrames(1, 4), 0.10f);
        anim->addAnimation("run-left",  atlas, makeFrames(2, 4), 0.10f);
        anim->addAnimation("jump",      atlas, makeFrames(3, 2), 0.15f, false);
        anim->addAnimation("fall",      atlas, makeFrames(4, 2), 0.15f, false);
        anim->play("idle");
    }

    // Game-world view: 800x600 window onto the larger map
    sf::View gameView(sf::FloatRect(0.f, 0.f, 800.f, 600.f));

    const float halfW = 400.f;
    const float halfH = 300.f;

    sf::Clock clock;
    DebugMenu debugMenu;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
                window.close();

            debugMenu.handleEvent(event, window);
        }

        // Reload map if the user selected one via the debug menu
        const std::string selectedMap = debugMenu.pollSelectedMap();
        if (!selectedMap.empty())
        {
            try
            {
                enemies.clear(); // destroy old enemy actors before clearing statics
                map = MapLoader::loadFromFile(selectedMap);
                map.registerPhysXStatics();
                player.position = map.getSpawnPoint();
                if (auto* physics = player.getComponent<PhysicsComponent>())
                    physics->velocity = { 0.f, 0.f };
                enemies = spawnEnemies(map, player);
            }
            catch (const std::exception& e)
            {
                std::cerr << "Failed to load map: " << e.what() << '\n';
            }
        }

        // Pause gameplay while debug menu is open
        float dt = 0.f;
        if (!debugMenu.isOpen())
            dt = clock.restart().asSeconds();
        else
            clock.restart();

        // Mark the start of a new physics frame (allows one PhysX step).
        PhysXWorld::instance().beginFrame();

        player.update(dt);

        for (auto& enemy : enemies)
        {
            auto* hp = enemy->getComponent<HealthComponent>();
            if (hp && hp->isDead())
                continue;
            enemy->update(dt);
        }

        // Drive animation state from physics / input
        if (auto* animComp = player.getComponent<AnimationComponent>())
        {
            auto* physics  = player.getComponent<PhysicsComponent>();
            auto* inputComp = player.getComponent<InputComponent>();
            if (physics && inputComp)
            {
                const auto& inp = inputComp->getInputState();
                if (!physics->isGrounded() && physics->velocity.y <= 0.f)
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

        // Camera: follow player, clamp to map bounds
        const sf::FloatRect& mapBounds = map.getBounds();
        float cx = player.position.x;
        float cy = player.position.y;
        cx = std::max(cx, mapBounds.left  + halfW);
        cx = std::min(cx, mapBounds.left  + mapBounds.width  - halfW);
        cy = std::max(cy, mapBounds.top   + halfH);
        cy = std::min(cy, mapBounds.top   + mapBounds.height - halfH);
        gameView.setCenter(cx, cy);

        window.setView(gameView);
        window.clear(sf::Color(30, 30, 50));
        map.render(window);

        for (auto& enemy : enemies)
        {
            auto* hp = enemy->getComponent<HealthComponent>();
            if (hp && hp->isDead())
                continue;
            enemy->render(window);
        }

        player.render(window);

        window.setView(window.getDefaultView());
        debugMenu.render(window);
        window.display();
    }

    PhysXWorld::instance().shutdown();
    return 0;
}
