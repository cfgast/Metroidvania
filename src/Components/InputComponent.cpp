#include "InputComponent.h"

#include "../Input/InputSystem.h"
#include "../Core/InputBindings.h"

#include <cmath>

void InputComponent::update(float /*dt*/)
{
    if (m_useExternal)
        return; // external code already set the state

    const auto& bindings = InputBindings::instance();
    auto& input = InputSystem::current();

    // Keyboard input (uses configurable bindings)
    m_state.moveLeft  = input.isKeyPressed(bindings.moveLeftKey)
                     || input.isKeyPressed(bindings.moveLeftAlt);
    m_state.moveRight = input.isKeyPressed(bindings.moveRightKey)
                     || input.isKeyPressed(bindings.moveRightAlt);
    m_state.jump      = input.isKeyPressed(bindings.jumpKey);
    m_state.dash      = input.isKeyPressed(bindings.dashKey);
    m_state.attack    = input.isKeyPressed(bindings.attackKey);

    // Controller / gamepad input (first connected joystick)
    if (input.isGamepadConnected(0))
    {
        constexpr float deadZone = 25.f;

        // Left stick horizontal
        float stickX = input.getGamepadAxis(0, GamepadAxis::LeftX);
        if (stickX < -deadZone) m_state.moveLeft  = true;
        if (stickX >  deadZone) m_state.moveRight = true;

        // D-pad horizontal
        float povX = input.getGamepadAxis(0, GamepadAxis::DPadX);
        if (povX < -deadZone) m_state.moveLeft  = true;
        if (povX >  deadZone) m_state.moveRight = true;

        // Controller jump button (configurable)
        if (input.isGamepadButtonPressed(0, static_cast<GamepadButton>(bindings.controllerJumpButton)))
            m_state.jump = true;

        // Controller dash button (configurable, default RB)
        if (input.isGamepadButtonPressed(0, static_cast<GamepadButton>(bindings.controllerDashButton)))
            m_state.dash = true;

        // Controller attack button (configurable, default X)
        if (input.isGamepadButtonPressed(0, static_cast<GamepadButton>(bindings.controllerAttackButton)))
            m_state.attack = true;
    }
}
