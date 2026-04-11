#pragma once

#include "InputTypes.h"

// Abstract input interface. Concrete implementations wrap a specific backend
// (SFML, GLFW, …).  The Renderer that owns the window also owns the
// InputSystem so that window-level events reach the input layer.
class InputSystem
{
public:
    virtual ~InputSystem() = default;

    virtual bool  pollEvent(InputEvent& event) = 0;
    virtual bool  isKeyPressed(KeyCode key) const = 0;
    virtual bool  isGamepadConnected(int id = 0) const = 0;
    virtual float getGamepadAxis(int id, GamepadAxis axis) const = 0;
    virtual bool  isGamepadButtonPressed(int id, GamepadButton btn) const = 0;
    virtual void  setMouseCursorVisible(bool visible) = 0;

    // Global accessor so components can reach the active InputSystem without
    // threading a pointer through every constructor (matches existing singleton
    // patterns like InputBindings and PhysXWorld).
    static InputSystem& current() { return *s_instance; }

protected:
    static inline InputSystem* s_instance = nullptr;
};
