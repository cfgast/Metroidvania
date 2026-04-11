#pragma once

#include <string>

#include "../Input/InputTypes.h"

// Singleton that holds user-configurable keyboard and controller bindings.
// Persists to saves/controls.json.
class InputBindings
{
public:
    static InputBindings& instance();

    // Keyboard bindings
    KeyCode moveLeftKey    = KeyCode::A;
    KeyCode moveLeftAlt    = KeyCode::Left;
    KeyCode moveRightKey   = KeyCode::D;
    KeyCode moveRightAlt   = KeyCode::Right;
    KeyCode jumpKey        = KeyCode::Space;
    KeyCode dashKey        = KeyCode::LShift;
    KeyCode attackKey      = KeyCode::X;

    // Controller bindings
    unsigned int controllerJumpButton   = 0;
    unsigned int controllerDashButton   = 5; // RB
    unsigned int controllerAttackButton = 2; // X

    void resetDefaults();
    void save() const;
    void load();

    // Human-readable names for display and serialization
    static std::string keyToString(KeyCode key);
    static KeyCode stringToKey(const std::string& name);
    static std::string buttonToString(unsigned int button);

private:
    InputBindings() = default;
};
