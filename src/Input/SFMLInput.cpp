#include "SFMLInput.h"

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Joystick.hpp>
#include <SFML/Window/Mouse.hpp>

// ── KeyCode ↔ sf::Keyboard::Key mapping ──────────────────────────────────────

struct KeyMap { KeyCode code; sf::Keyboard::Key sfKey; };

static const KeyMap KEY_MAP[] = {
    { KeyCode::A, sf::Keyboard::A }, { KeyCode::B, sf::Keyboard::B },
    { KeyCode::C, sf::Keyboard::C }, { KeyCode::D, sf::Keyboard::D },
    { KeyCode::E, sf::Keyboard::E }, { KeyCode::F, sf::Keyboard::F },
    { KeyCode::G, sf::Keyboard::G }, { KeyCode::H, sf::Keyboard::H },
    { KeyCode::I, sf::Keyboard::I }, { KeyCode::J, sf::Keyboard::J },
    { KeyCode::K, sf::Keyboard::K }, { KeyCode::L, sf::Keyboard::L },
    { KeyCode::M, sf::Keyboard::M }, { KeyCode::N, sf::Keyboard::N },
    { KeyCode::O, sf::Keyboard::O }, { KeyCode::P, sf::Keyboard::P },
    { KeyCode::Q, sf::Keyboard::Q }, { KeyCode::R, sf::Keyboard::R },
    { KeyCode::S, sf::Keyboard::S }, { KeyCode::T, sf::Keyboard::T },
    { KeyCode::U, sf::Keyboard::U }, { KeyCode::V, sf::Keyboard::V },
    { KeyCode::W, sf::Keyboard::W }, { KeyCode::X, sf::Keyboard::X },
    { KeyCode::Y, sf::Keyboard::Y }, { KeyCode::Z, sf::Keyboard::Z },

    { KeyCode::Num0, sf::Keyboard::Num0 }, { KeyCode::Num1, sf::Keyboard::Num1 },
    { KeyCode::Num2, sf::Keyboard::Num2 }, { KeyCode::Num3, sf::Keyboard::Num3 },
    { KeyCode::Num4, sf::Keyboard::Num4 }, { KeyCode::Num5, sf::Keyboard::Num5 },
    { KeyCode::Num6, sf::Keyboard::Num6 }, { KeyCode::Num7, sf::Keyboard::Num7 },
    { KeyCode::Num8, sf::Keyboard::Num8 }, { KeyCode::Num9, sf::Keyboard::Num9 },

    { KeyCode::F1,  sf::Keyboard::F1  }, { KeyCode::F2,  sf::Keyboard::F2  },
    { KeyCode::F3,  sf::Keyboard::F3  }, { KeyCode::F4,  sf::Keyboard::F4  },
    { KeyCode::F5,  sf::Keyboard::F5  }, { KeyCode::F6,  sf::Keyboard::F6  },
    { KeyCode::F7,  sf::Keyboard::F7  }, { KeyCode::F8,  sf::Keyboard::F8  },
    { KeyCode::F9,  sf::Keyboard::F9  }, { KeyCode::F10, sf::Keyboard::F10 },
    { KeyCode::F11, sf::Keyboard::F11 }, { KeyCode::F12, sf::Keyboard::F12 },

    { KeyCode::Left,  sf::Keyboard::Left  }, { KeyCode::Right, sf::Keyboard::Right },
    { KeyCode::Up,    sf::Keyboard::Up    }, { KeyCode::Down,  sf::Keyboard::Down  },
    { KeyCode::Home,  sf::Keyboard::Home  }, { KeyCode::End,   sf::Keyboard::End   },
    { KeyCode::PageUp,   sf::Keyboard::PageUp   },
    { KeyCode::PageDown, sf::Keyboard::PageDown },
    { KeyCode::Insert, sf::Keyboard::Insert }, { KeyCode::Delete, sf::Keyboard::Delete },

    { KeyCode::LShift,   sf::Keyboard::LShift   },
    { KeyCode::RShift,   sf::Keyboard::RShift   },
    { KeyCode::LControl, sf::Keyboard::LControl },
    { KeyCode::RControl, sf::Keyboard::RControl },
    { KeyCode::LAlt,     sf::Keyboard::LAlt     },
    { KeyCode::RAlt,     sf::Keyboard::RAlt     },

    { KeyCode::Space,     sf::Keyboard::Space     },
    { KeyCode::Enter,     sf::Keyboard::Enter     },
    { KeyCode::Backspace, sf::Keyboard::Backspace },
    { KeyCode::Tab,       sf::Keyboard::Tab       },
    { KeyCode::Escape,    sf::Keyboard::Escape    },

    { KeyCode::LBracket,  sf::Keyboard::LBracket  },
    { KeyCode::RBracket,  sf::Keyboard::RBracket  },
    { KeyCode::Semicolon, sf::Keyboard::Semicolon },
    { KeyCode::Comma,     sf::Keyboard::Comma     },
    { KeyCode::Period,    sf::Keyboard::Period    },
    { KeyCode::Quote,     sf::Keyboard::Quote     },
    { KeyCode::Slash,     sf::Keyboard::Slash     },
    { KeyCode::Backslash, sf::Keyboard::Backslash },
    { KeyCode::Tilde,     sf::Keyboard::Tilde     },
    { KeyCode::Equal,     sf::Keyboard::Equal     },
    { KeyCode::Hyphen,    sf::Keyboard::Hyphen    },

    { KeyCode::Numpad0, sf::Keyboard::Numpad0 }, { KeyCode::Numpad1, sf::Keyboard::Numpad1 },
    { KeyCode::Numpad2, sf::Keyboard::Numpad2 }, { KeyCode::Numpad3, sf::Keyboard::Numpad3 },
    { KeyCode::Numpad4, sf::Keyboard::Numpad4 }, { KeyCode::Numpad5, sf::Keyboard::Numpad5 },
    { KeyCode::Numpad6, sf::Keyboard::Numpad6 }, { KeyCode::Numpad7, sf::Keyboard::Numpad7 },
    { KeyCode::Numpad8, sf::Keyboard::Numpad8 }, { KeyCode::Numpad9, sf::Keyboard::Numpad9 },
};

static constexpr size_t KEY_MAP_SIZE = sizeof(KEY_MAP) / sizeof(KEY_MAP[0]);

static sf::Keyboard::Key toSfKey(KeyCode code)
{
    for (size_t i = 0; i < KEY_MAP_SIZE; ++i)
        if (KEY_MAP[i].code == code) return KEY_MAP[i].sfKey;
    return sf::Keyboard::Unknown;
}

static KeyCode fromSfKey(sf::Keyboard::Key sfKey)
{
    for (size_t i = 0; i < KEY_MAP_SIZE; ++i)
        if (KEY_MAP[i].sfKey == sfKey) return KEY_MAP[i].code;
    return KeyCode::Unknown;
}

// ── GamepadAxis ↔ sf::Joystick::Axis mapping ────────────────────────────────

static sf::Joystick::Axis toSfAxis(GamepadAxis axis)
{
    switch (axis)
    {
        case GamepadAxis::LeftX:  return sf::Joystick::X;
        case GamepadAxis::LeftY:  return sf::Joystick::Y;
        case GamepadAxis::RightX: return sf::Joystick::U;
        case GamepadAxis::RightY: return sf::Joystick::V;
        case GamepadAxis::DPadX:  return sf::Joystick::PovX;
        case GamepadAxis::DPadY:  return sf::Joystick::PovY;
    }
    return sf::Joystick::X;
}

static GamepadAxis fromSfAxis(sf::Joystick::Axis sfAxis)
{
    switch (sfAxis)
    {
        case sf::Joystick::X:    return GamepadAxis::LeftX;
        case sf::Joystick::Y:    return GamepadAxis::LeftY;
        case sf::Joystick::U:    return GamepadAxis::RightX;
        case sf::Joystick::V:    return GamepadAxis::RightY;
        case sf::Joystick::PovX: return GamepadAxis::DPadX;
        case sf::Joystick::PovY: return GamepadAxis::DPadY;
        default:                 return GamepadAxis::LeftX;
    }
}

// ── Constructor ──────────────────────────────────────────────────────────────

SFMLInput::SFMLInput(sf::RenderWindow& window)
    : m_window(window)
{
    s_instance = this;
}

// ── pollEvent ────────────────────────────────────────────────────────────────

bool SFMLInput::pollEvent(InputEvent& event)
{
    sf::Event sfEvent;
    while (m_window.pollEvent(sfEvent))
    {
        if (translateEvent(sfEvent, event))
            return true;
    }
    return false;
}

bool SFMLInput::translateEvent(const sf::Event& sf, InputEvent& out) const
{
    switch (sf.type)
    {
        case sf::Event::Closed:
            out = {};
            out.type = InputEventType::WindowClosed;
            return true;

        case sf::Event::Resized:
            out = {};
            out.type   = InputEventType::WindowResized;
            out.width  = sf.size.width;
            out.height = sf.size.height;
            return true;

        case sf::Event::KeyPressed:
            out = {};
            out.type = InputEventType::KeyPressed;
            out.key  = fromSfKey(sf.key.code);
            return true;

        case sf::Event::KeyReleased:
            out = {};
            out.type = InputEventType::KeyReleased;
            out.key  = fromSfKey(sf.key.code);
            return true;

        case sf::Event::MouseMoved:
            out = {};
            out.type   = InputEventType::MouseMoved;
            out.mouseX = sf.mouseMove.x;
            out.mouseY = sf.mouseMove.y;
            return true;

        case sf::Event::MouseButtonPressed:
        case sf::Event::MouseButtonReleased:
        {
            out = {};
            out.type = (sf.type == sf::Event::MouseButtonPressed)
                           ? InputEventType::MouseButtonPressed
                           : InputEventType::MouseButtonReleased;
            switch (sf.mouseButton.button)
            {
                case sf::Mouse::Left:   out.mouseButton = MouseButton::Left;   break;
                case sf::Mouse::Right:  out.mouseButton = MouseButton::Right;  break;
                case sf::Mouse::Middle: out.mouseButton = MouseButton::Middle; break;
                default:                out.mouseButton = MouseButton::Left;   break;
            }
            out.mouseX = sf.mouseButton.x;
            out.mouseY = sf.mouseButton.y;
            return true;
        }

        case sf::Event::JoystickButtonPressed:
            out = {};
            out.type = InputEventType::GamepadButtonPressed;
            out.gamepadId = sf.joystickButton.joystickId;
            out.gamepadButton = static_cast<GamepadButton>(sf.joystickButton.button);
            return true;

        case sf::Event::JoystickButtonReleased:
            out = {};
            out.type = InputEventType::GamepadButtonReleased;
            out.gamepadId = sf.joystickButton.joystickId;
            out.gamepadButton = static_cast<GamepadButton>(sf.joystickButton.button);
            return true;

        case sf::Event::JoystickMoved:
            out = {};
            out.type         = InputEventType::GamepadAxisMoved;
            out.gamepadId    = sf.joystickMove.joystickId;
            out.gamepadAxis  = fromSfAxis(sf.joystickMove.axis);
            out.axisPosition = sf.joystickMove.position;
            return true;

        default:
            return false;
    }
}

// ── Polling queries ──────────────────────────────────────────────────────────

bool SFMLInput::isKeyPressed(KeyCode key) const
{
    sf::Keyboard::Key sfKey = toSfKey(key);
    if (sfKey == sf::Keyboard::Unknown)
        return false;
    return sf::Keyboard::isKeyPressed(sfKey);
}

bool SFMLInput::isGamepadConnected(int id) const
{
    return sf::Joystick::isConnected(id);
}

float SFMLInput::getGamepadAxis(int id, GamepadAxis axis) const
{
    return sf::Joystick::getAxisPosition(id, toSfAxis(axis));
}

bool SFMLInput::isGamepadButtonPressed(int id, GamepadButton btn) const
{
    return sf::Joystick::isButtonPressed(id, static_cast<unsigned int>(btn));
}

void SFMLInput::setMouseCursorVisible(bool visible)
{
    m_window.setMouseCursorVisible(visible);
}
