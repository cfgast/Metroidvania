#include "InputComponent.h"

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Joystick.hpp>

#include "../Core/InputBindings.h"

#include <cmath>

void InputComponent::update(float /*dt*/)
{
    if (m_useExternal)
        return; // external code already set the state

    const auto& bindings = InputBindings::instance();

    // Keyboard input (uses configurable bindings)
    m_state.moveLeft  = sf::Keyboard::isKeyPressed(bindings.moveLeftKey)
                     || sf::Keyboard::isKeyPressed(bindings.moveLeftAlt);
    m_state.moveRight = sf::Keyboard::isKeyPressed(bindings.moveRightKey)
                     || sf::Keyboard::isKeyPressed(bindings.moveRightAlt);
    m_state.jump      = sf::Keyboard::isKeyPressed(bindings.jumpKey);
    m_state.dash      = sf::Keyboard::isKeyPressed(bindings.dashKey);

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

        // Controller jump button (configurable)
        if (sf::Joystick::isButtonPressed(0, bindings.controllerJumpButton))
            m_state.jump = true;

        // Controller dash button (configurable, default RB)
        if (sf::Joystick::isButtonPressed(0, bindings.controllerDashButton))
            m_state.dash = true;
    }
}
