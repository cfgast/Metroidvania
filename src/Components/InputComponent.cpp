#include "InputComponent.h"

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Joystick.hpp>

#include <cmath>

void InputComponent::update(float /*dt*/)
{
    if (m_useExternal)
        return; // external code already set the state

    // Keyboard input
    m_state.moveLeft  = sf::Keyboard::isKeyPressed(sf::Keyboard::A)
                     || sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
    m_state.moveRight = sf::Keyboard::isKeyPressed(sf::Keyboard::D)
                     || sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
    m_state.jump      = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);

    // Controller / gamepad input (first connected joystick)
    if (sf::Joystick::isConnected(0))
    {
        constexpr float deadZone = 25.f;

        // Left stick horizontal
        float stickX = sf::Joystick::getAxisPosition(0, sf::Joystick::X);
        if (stickX < -deadZone) m_state.moveLeft  = true;
        if (stickX >  deadZone) m_state.moveRight = true;

        // D-pad horizontal
        if (sf::Joystick::hasAxis(0, sf::Joystick::PovX))
        {
            float povX = sf::Joystick::getAxisPosition(0, sf::Joystick::PovX);
            if (povX < -deadZone) m_state.moveLeft  = true;
            if (povX >  deadZone) m_state.moveRight = true;
        }

        // A button (button 0) for jump
        if (sf::Joystick::isButtonPressed(0, 0))
            m_state.jump = true;
    }
}
