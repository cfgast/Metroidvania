#include <iostream>

#include <SFML/Graphics.hpp>

#include "Core/GameObject.h"
#include "Components/InputComponent.h"
#include "Components/PhysicsComponent.h"
#include "Components/RenderComponent.h"
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
                map = MapLoader::loadFromFile(selectedMap);
                map.registerPhysXStatics();
                player.position = map.getSpawnPoint();
                if (auto* physics = player.getComponent<PhysicsComponent>())
                    physics->velocity = { 0.f, 0.f };
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

        player.update(dt);

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
        player.render(window);

        window.setView(window.getDefaultView());
        debugMenu.render(window);
        window.display();
    }

    PhysXWorld::instance().shutdown();
    return 0;
}
