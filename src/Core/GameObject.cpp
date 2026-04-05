#include "GameObject.h"

#include <SFML/Graphics/RenderWindow.hpp>

void GameObject::update(float dt)
{
    for (auto& component : m_components)
        component->update(dt);
}

void GameObject::render(sf::RenderWindow& window)
{
    for (auto& component : m_components)
        component->render(window);
}
