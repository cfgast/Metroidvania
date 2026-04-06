#include "InputComponent.h"

#include <SFML/Window/Keyboard.hpp>

void InputComponent::update(float /*dt*/)
{
    if (m_useExternal)
        return; // external code already set the state

    m_state.moveLeft  = sf::Keyboard::isKeyPressed(sf::Keyboard::A)
                     || sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
    m_state.moveRight = sf::Keyboard::isKeyPressed(sf::Keyboard::D)
                     || sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
    m_state.jump      = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);
}
