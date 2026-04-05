#include <SFML/Graphics.hpp>

#include "Core/GameObject.h"
#include "Components/MovementComponent.h"
#include "Components/RenderComponent.h"

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "Metroidvania");
    window.setFramerateLimit(60);

    GameObject player;
    player.position = { 400.f, 300.f };
    player.addComponent<RenderComponent>(sf::Vector2f(50.f, 50.f), sf::Color::Green);
    player.addComponent<MovementComponent>(300.f);

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

        window.clear(sf::Color(30, 30, 30));
        player.render(window);
        window.display();
    }

    return 0;
}
