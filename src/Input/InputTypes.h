#pragma once

#include <cstdint>

// Backend-agnostic key codes. Values are arbitrary integers that can be mapped
// from GLFW key codes via a translation table.
enum class KeyCode : int
{
    Unknown = -1,

    // Letters
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    // Digits
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

    // Function keys
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    // Navigation
    Left, Right, Up, Down,
    Home, End, PageUp, PageDown, Insert, Delete,

    // Modifiers
    LShift, RShift, LControl, RControl, LAlt, RAlt,

    // Whitespace / editing
    Space, Enter, Backspace, Tab, Escape,

    // Punctuation
    LBracket, RBracket, Semicolon, Comma, Period,
    Quote, Slash, Backslash, Tilde, Equal, Hyphen,

    // Numpad
    Numpad0, Numpad1, Numpad2, Numpad3, Numpad4,
    Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,

    KeyCount
};

enum class MouseButton : int
{
    Left,
    Right,
    Middle
};

enum class GamepadButton : int
{
    A,          // 0  — Xbox A / PS Cross
    B,          // 1  — Xbox B / PS Circle
    X,          // 2  — Xbox X / PS Square
    Y,          // 3  — Xbox Y / PS Triangle
    LB,         // 4  — Left bumper
    RB,         // 5  — Right bumper
    Back,       // 6  — Back / Select / Share
    Start,      // 7  — Start / Menu / Options
    LeftStick,  // 8  — L3
    RightStick, // 9  — R3
    DPadUp,     // 10
    DPadDown,   // 11
    DPadLeft,   // 12
    DPadRight,  // 13
    Guide,      // 14 — Xbox / PS button

    ButtonCount
};

enum class GamepadAxis : int
{
    LeftX,
    LeftY,
    RightX,
    RightY,
    DPadX,
    DPadY
};

// Unified input event produced by any backend.
enum class InputEventType : int
{
    KeyPressed,
    KeyReleased,
    MouseMoved,
    MouseButtonPressed,
    MouseButtonReleased,
    GamepadButtonPressed,
    GamepadButtonReleased,
    GamepadAxisMoved,
    WindowClosed,
    WindowResized
};

struct InputEvent
{
    InputEventType type{};

    // Key events
    KeyCode key = KeyCode::Unknown;

    // Mouse events
    MouseButton mouseButton = MouseButton::Left;
    int mouseX = 0;
    int mouseY = 0;

    // Gamepad events
    int gamepadId = 0;
    GamepadButton gamepadButton = GamepadButton::A;
    GamepadAxis gamepadAxis = GamepadAxis::LeftX;
    float axisPosition = 0.f;

    // Window resize
    unsigned int width  = 0;
    unsigned int height = 0;
};
