#include <SFML/Graphics.hpp>

#include "Core/GameObject.h"
#include "Components/PhysicsComponent.h"
#include "Components/RenderComponent.h"
#include "Map/Map.h"

static Map buildStartingMap()
{
    Map map;
    map.setBounds({ -200.f, -500.f, 3600.f, 1200.f });

    const sf::Color groundColor  { 80,  80,  80 };
    const sf::Color platformColor{ 120, 80,  40 };

    // Ground — full width
    map.addPlatform({ { -200.f, 500.f, 3600.f, 40.f }, groundColor });

    // Raised platforms
    map.addPlatform({ {  250.f, 380.f,  200.f, 20.f }, platformColor });
    map.addPlatform({ {  550.f, 300.f,  150.f, 20.f }, platformColor });
    map.addPlatform({ {  800.f, 220.f,  180.f, 20.f }, platformColor });
    map.addPlatform({ { 1050.f, 340.f,  200.f, 20.f }, platformColor });
    map.addPlatform({ { 1350.f, 260.f,  150.f, 20.f }, platformColor });
    map.addPlatform({ { 1600.f, 180.f,  200.f, 20.f }, platformColor });
    map.addPlatform({ { 1900.f, 340.f,  180.f, 20.f }, platformColor });
    map.addPlatform({ { 2150.f, 260.f,  150.f, 20.f }, platformColor });
    map.addPlatform({ { 2400.f, 180.f,  200.f, 20.f }, platformColor });

    // Spawn just above ground (player half-height = 25)
    map.setSpawnPoint({ 150.f, 475.f });

    return map;
}

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "Metroidvania");
    window.setFramerateLimit(60);

    Map map = buildStartingMap();

    const sf::Vector2f playerSize{ 50.f, 50.f };

    GameObject player;
    player.position = map.getSpawnPoint();
    player.addComponent<RenderComponent>(playerSize, sf::Color::Green);
    player.addComponent<PhysicsComponent>(map, playerSize, 300.f);

    // Game-world view: 800x600 window onto the larger map
    sf::View gameView(sf::FloatRect(0.f, 0.f, 800.f, 600.f));

    const sf::FloatRect& mapBounds = map.getBounds();
    const float halfW = 400.f;
    const float halfH = 300.f;

    sf::Clock clock;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
                window.close();
        }

        float dt = clock.restart().asSeconds();

        player.update(dt);

        // Camera: follow player, clamp to map bounds
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
        window.display();
    }

    return 0;
}
