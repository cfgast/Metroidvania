#include "GLFWInput.h"
#include "../Rendering/GLRenderer.h"

#include <cstring>

// ── Static instance for joystick callback (no window pointer available) ──────

GLFWInput* GLFWInput::s_joystickInstance = nullptr;

// ── KeyCode ↔ GLFW key mapping ───────────────────────────────────────────────

struct GlfwKeyMap { KeyCode code; int glfwKey; };

static const GlfwKeyMap GLFW_KEY_MAP[] = {
    // Letters
    { KeyCode::A, GLFW_KEY_A }, { KeyCode::B, GLFW_KEY_B },
    { KeyCode::C, GLFW_KEY_C }, { KeyCode::D, GLFW_KEY_D },
    { KeyCode::E, GLFW_KEY_E }, { KeyCode::F, GLFW_KEY_F },
    { KeyCode::G, GLFW_KEY_G }, { KeyCode::H, GLFW_KEY_H },
    { KeyCode::I, GLFW_KEY_I }, { KeyCode::J, GLFW_KEY_J },
    { KeyCode::K, GLFW_KEY_K }, { KeyCode::L, GLFW_KEY_L },
    { KeyCode::M, GLFW_KEY_M }, { KeyCode::N, GLFW_KEY_N },
    { KeyCode::O, GLFW_KEY_O }, { KeyCode::P, GLFW_KEY_P },
    { KeyCode::Q, GLFW_KEY_Q }, { KeyCode::R, GLFW_KEY_R },
    { KeyCode::S, GLFW_KEY_S }, { KeyCode::T, GLFW_KEY_T },
    { KeyCode::U, GLFW_KEY_U }, { KeyCode::V, GLFW_KEY_V },
    { KeyCode::W, GLFW_KEY_W }, { KeyCode::X, GLFW_KEY_X },
    { KeyCode::Y, GLFW_KEY_Y }, { KeyCode::Z, GLFW_KEY_Z },

    // Digits
    { KeyCode::Num0, GLFW_KEY_0 }, { KeyCode::Num1, GLFW_KEY_1 },
    { KeyCode::Num2, GLFW_KEY_2 }, { KeyCode::Num3, GLFW_KEY_3 },
    { KeyCode::Num4, GLFW_KEY_4 }, { KeyCode::Num5, GLFW_KEY_5 },
    { KeyCode::Num6, GLFW_KEY_6 }, { KeyCode::Num7, GLFW_KEY_7 },
    { KeyCode::Num8, GLFW_KEY_8 }, { KeyCode::Num9, GLFW_KEY_9 },

    // Function keys
    { KeyCode::F1,  GLFW_KEY_F1  }, { KeyCode::F2,  GLFW_KEY_F2  },
    { KeyCode::F3,  GLFW_KEY_F3  }, { KeyCode::F4,  GLFW_KEY_F4  },
    { KeyCode::F5,  GLFW_KEY_F5  }, { KeyCode::F6,  GLFW_KEY_F6  },
    { KeyCode::F7,  GLFW_KEY_F7  }, { KeyCode::F8,  GLFW_KEY_F8  },
    { KeyCode::F9,  GLFW_KEY_F9  }, { KeyCode::F10, GLFW_KEY_F10 },
    { KeyCode::F11, GLFW_KEY_F11 }, { KeyCode::F12, GLFW_KEY_F12 },

    // Navigation
    { KeyCode::Left,     GLFW_KEY_LEFT     }, { KeyCode::Right,    GLFW_KEY_RIGHT    },
    { KeyCode::Up,       GLFW_KEY_UP       }, { KeyCode::Down,     GLFW_KEY_DOWN     },
    { KeyCode::Home,     GLFW_KEY_HOME     }, { KeyCode::End,      GLFW_KEY_END      },
    { KeyCode::PageUp,   GLFW_KEY_PAGE_UP  }, { KeyCode::PageDown, GLFW_KEY_PAGE_DOWN },
    { KeyCode::Insert,   GLFW_KEY_INSERT   }, { KeyCode::Delete,   GLFW_KEY_DELETE   },

    // Modifiers
    { KeyCode::LShift,   GLFW_KEY_LEFT_SHIFT    },
    { KeyCode::RShift,   GLFW_KEY_RIGHT_SHIFT   },
    { KeyCode::LControl, GLFW_KEY_LEFT_CONTROL  },
    { KeyCode::RControl, GLFW_KEY_RIGHT_CONTROL },
    { KeyCode::LAlt,     GLFW_KEY_LEFT_ALT      },
    { KeyCode::RAlt,     GLFW_KEY_RIGHT_ALT     },

    // Whitespace / editing
    { KeyCode::Space,     GLFW_KEY_SPACE     },
    { KeyCode::Enter,     GLFW_KEY_ENTER     },
    { KeyCode::Backspace, GLFW_KEY_BACKSPACE },
    { KeyCode::Tab,       GLFW_KEY_TAB       },
    { KeyCode::Escape,    GLFW_KEY_ESCAPE    },

    // Punctuation
    { KeyCode::LBracket,  GLFW_KEY_LEFT_BRACKET  },
    { KeyCode::RBracket,  GLFW_KEY_RIGHT_BRACKET },
    { KeyCode::Semicolon, GLFW_KEY_SEMICOLON     },
    { KeyCode::Comma,     GLFW_KEY_COMMA         },
    { KeyCode::Period,    GLFW_KEY_PERIOD        },
    { KeyCode::Quote,     GLFW_KEY_APOSTROPHE    },
    { KeyCode::Slash,     GLFW_KEY_SLASH         },
    { KeyCode::Backslash, GLFW_KEY_BACKSLASH     },
    { KeyCode::Tilde,     GLFW_KEY_GRAVE_ACCENT  },
    { KeyCode::Equal,     GLFW_KEY_EQUAL         },
    { KeyCode::Hyphen,    GLFW_KEY_MINUS         },

    // Numpad
    { KeyCode::Numpad0, GLFW_KEY_KP_0 }, { KeyCode::Numpad1, GLFW_KEY_KP_1 },
    { KeyCode::Numpad2, GLFW_KEY_KP_2 }, { KeyCode::Numpad3, GLFW_KEY_KP_3 },
    { KeyCode::Numpad4, GLFW_KEY_KP_4 }, { KeyCode::Numpad5, GLFW_KEY_KP_5 },
    { KeyCode::Numpad6, GLFW_KEY_KP_6 }, { KeyCode::Numpad7, GLFW_KEY_KP_7 },
    { KeyCode::Numpad8, GLFW_KEY_KP_8 }, { KeyCode::Numpad9, GLFW_KEY_KP_9 },
};

static constexpr size_t GLFW_KEY_MAP_SIZE = sizeof(GLFW_KEY_MAP) / sizeof(GLFW_KEY_MAP[0]);

KeyCode GLFWInput::fromGlfwKey(int glfwKey)
{
    for (size_t i = 0; i < GLFW_KEY_MAP_SIZE; ++i)
        if (GLFW_KEY_MAP[i].glfwKey == glfwKey) return GLFW_KEY_MAP[i].code;
    return KeyCode::Unknown;
}

int GLFWInput::toGlfwKey(KeyCode code)
{
    for (size_t i = 0; i < GLFW_KEY_MAP_SIZE; ++i)
        if (GLFW_KEY_MAP[i].code == code) return GLFW_KEY_MAP[i].glfwKey;
    return GLFW_KEY_UNKNOWN;
}

// ── Mouse button mapping ─────────────────────────────────────────────────────

MouseButton GLFWInput::fromGlfwMouseButton(int glfwBtn)
{
    switch (glfwBtn)
    {
        case GLFW_MOUSE_BUTTON_LEFT:   return MouseButton::Left;
        case GLFW_MOUSE_BUTTON_RIGHT:  return MouseButton::Right;
        case GLFW_MOUSE_BUTTON_MIDDLE: return MouseButton::Middle;
        default:                       return MouseButton::Left;
    }
}

// ── Gamepad axis mapping ─────────────────────────────────────────────────────

// Returns the GLFW gamepad axis index for a GamepadAxis enum value.
GamepadAxis GLFWInput::toGlfwGamepadAxis(GamepadAxis axis)
{
    // GamepadAxis enum values map directly to GLFW_GAMEPAD_AXIS_* constants
    // for the first four axes; DPadX/DPadY are not natively axes in GLFW's
    // gamepad API (they're buttons), so we handle them separately.
    return axis;
}

// ── Constructor ──────────────────────────────────────────────────────────────

GLFWInput::GLFWInput(GLFWwindow* window, GLRenderer& renderer)
    : m_window(window)
    , m_renderer(renderer)
{
    s_instance = this;
    s_joystickInstance = this;

    glfwSetKeyCallback(m_window, keyCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetCursorPosCallback(m_window, cursorPosCallback);
    glfwSetWindowSizeCallback(m_window, windowSizeCallback);
    glfwSetWindowCloseCallback(m_window, windowCloseCallback);
    glfwSetJoystickCallback(joystickCallback);
}

// ── pollEvent ────────────────────────────────────────────────────────────────

bool GLFWInput::pollEvent(InputEvent& event)
{
    // If the queue is empty, pump GLFW events so callbacks can fill it.
    if (m_eventQueue.empty())
        glfwPollEvents();

    if (!m_eventQueue.empty())
    {
        event = m_eventQueue.front();
        m_eventQueue.pop();
        return true;
    }
    return false;
}

// ── Polling queries ──────────────────────────────────────────────────────────

bool GLFWInput::isKeyPressed(KeyCode key) const
{
    int glfwKey = toGlfwKey(key);
    if (glfwKey == GLFW_KEY_UNKNOWN)
        return false;
    return glfwGetKey(m_window, glfwKey) == GLFW_PRESS;
}

bool GLFWInput::isGamepadConnected(int id) const
{
    return glfwJoystickPresent(id) == GLFW_TRUE;
}

float GLFWInput::getGamepadAxis(int id, GamepadAxis axis) const
{
    GLFWgamepadstate state;
    if (glfwGetGamepadState(id, &state))
    {
        switch (axis)
        {
            case GamepadAxis::LeftX:  return state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
            case GamepadAxis::LeftY:  return state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
            case GamepadAxis::RightX: return state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
            case GamepadAxis::RightY: return state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
            case GamepadAxis::DPadX:
            {
                // GLFW models D-pad as buttons, so synthesize an axis value
                float val = 0.f;
                if (state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT])  val -= 1.f;
                if (state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT]) val += 1.f;
                return val;
            }
            case GamepadAxis::DPadY:
            {
                float val = 0.f;
                if (state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP])   val -= 1.f;
                if (state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN]) val += 1.f;
                return val;
            }
        }
    }

    // Fallback: try raw joystick axes
    int count = 0;
    const float* axes = glfwGetJoystickAxes(id, &count);
    if (axes)
    {
        int idx = -1;
        switch (axis)
        {
            case GamepadAxis::LeftX:  idx = 0; break;
            case GamepadAxis::LeftY:  idx = 1; break;
            case GamepadAxis::RightX: idx = 2; break;
            case GamepadAxis::RightY: idx = 3; break;
            default: break;
        }
        if (idx >= 0 && idx < count)
            return axes[idx];
    }

    return 0.f;
}

bool GLFWInput::isGamepadButtonPressed(int id, GamepadButton btn) const
{
    GLFWgamepadstate state;
    if (glfwGetGamepadState(id, &state))
    {
        // GamepadButton enum values map directly to GLFW_GAMEPAD_BUTTON_* constants
        int glfwBtn = -1;
        switch (btn)
        {
            case GamepadButton::A:          glfwBtn = GLFW_GAMEPAD_BUTTON_A; break;
            case GamepadButton::B:          glfwBtn = GLFW_GAMEPAD_BUTTON_B; break;
            case GamepadButton::X:          glfwBtn = GLFW_GAMEPAD_BUTTON_X; break;
            case GamepadButton::Y:          glfwBtn = GLFW_GAMEPAD_BUTTON_Y; break;
            case GamepadButton::LB:         glfwBtn = GLFW_GAMEPAD_BUTTON_LEFT_BUMPER; break;
            case GamepadButton::RB:         glfwBtn = GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER; break;
            case GamepadButton::Back:       glfwBtn = GLFW_GAMEPAD_BUTTON_BACK; break;
            case GamepadButton::Start:      glfwBtn = GLFW_GAMEPAD_BUTTON_START; break;
            case GamepadButton::LeftStick:  glfwBtn = GLFW_GAMEPAD_BUTTON_LEFT_THUMB; break;
            case GamepadButton::RightStick: glfwBtn = GLFW_GAMEPAD_BUTTON_RIGHT_THUMB; break;
            case GamepadButton::DPadUp:     glfwBtn = GLFW_GAMEPAD_BUTTON_DPAD_UP; break;
            case GamepadButton::DPadDown:   glfwBtn = GLFW_GAMEPAD_BUTTON_DPAD_DOWN; break;
            case GamepadButton::DPadLeft:   glfwBtn = GLFW_GAMEPAD_BUTTON_DPAD_LEFT; break;
            case GamepadButton::DPadRight:  glfwBtn = GLFW_GAMEPAD_BUTTON_DPAD_RIGHT; break;
            case GamepadButton::Guide:      glfwBtn = GLFW_GAMEPAD_BUTTON_GUIDE; break;
            default: return false;
        }
        return state.buttons[glfwBtn] == GLFW_PRESS;
    }

    // Fallback: raw joystick buttons
    int count = 0;
    const unsigned char* buttons = glfwGetJoystickButtons(id, &count);
    int idx = static_cast<int>(btn);
    if (buttons && idx < count)
        return buttons[idx] == GLFW_PRESS;

    return false;
}

void GLFWInput::setMouseCursorVisible(bool visible)
{
    glfwSetInputMode(m_window, GLFW_CURSOR,
                     visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
}

// ── GLFW callbacks ───────────────────────────────────────────────────────────

void GLFWInput::keyCallback(GLFWwindow* win, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (action == GLFW_REPEAT)
        return; // ignore key repeat; game uses isKeyPressed() for held keys

    GLFWInput* self = static_cast<GLFWInput*>(
        static_cast<GLRenderer*>(glfwGetWindowUserPointer(win))->getInputPtr());

    InputEvent ev{};
    ev.type = (action == GLFW_PRESS) ? InputEventType::KeyPressed
                                     : InputEventType::KeyReleased;
    ev.key = fromGlfwKey(key);
    self->m_eventQueue.push(ev);
}

void GLFWInput::mouseButtonCallback(GLFWwindow* win, int button, int action, int /*mods*/)
{
    GLFWInput* self = static_cast<GLFWInput*>(
        static_cast<GLRenderer*>(glfwGetWindowUserPointer(win))->getInputPtr());

    double mx, my;
    glfwGetCursorPos(win, &mx, &my);

    InputEvent ev{};
    ev.type = (action == GLFW_PRESS) ? InputEventType::MouseButtonPressed
                                     : InputEventType::MouseButtonReleased;
    ev.mouseButton = fromGlfwMouseButton(button);
    ev.mouseX = static_cast<int>(mx);
    ev.mouseY = static_cast<int>(my);
    self->m_eventQueue.push(ev);
}

void GLFWInput::cursorPosCallback(GLFWwindow* win, double xpos, double ypos)
{
    GLFWInput* self = static_cast<GLFWInput*>(
        static_cast<GLRenderer*>(glfwGetWindowUserPointer(win))->getInputPtr());

    InputEvent ev{};
    ev.type   = InputEventType::MouseMoved;
    ev.mouseX = static_cast<int>(xpos);
    ev.mouseY = static_cast<int>(ypos);
    self->m_eventQueue.push(ev);
}

void GLFWInput::windowSizeCallback(GLFWwindow* win, int width, int height)
{
    GLRenderer* renderer = static_cast<GLRenderer*>(glfwGetWindowUserPointer(win));
    GLFWInput* self = static_cast<GLFWInput*>(renderer->getInputPtr());

    // Update GLRenderer's internal window size and viewport
    renderer->handleWindowResize(width, height);

    InputEvent ev{};
    ev.type   = InputEventType::WindowResized;
    ev.width  = static_cast<unsigned int>(width);
    ev.height = static_cast<unsigned int>(height);
    self->m_eventQueue.push(ev);
}

void GLFWInput::windowCloseCallback(GLFWwindow* win)
{
    GLRenderer* renderer = static_cast<GLRenderer*>(glfwGetWindowUserPointer(win));
    GLFWInput* self = static_cast<GLFWInput*>(renderer->getInputPtr());

    renderer->handleWindowClose();

    InputEvent ev{};
    ev.type = InputEventType::WindowClosed;
    self->m_eventQueue.push(ev);
}

void GLFWInput::joystickCallback(int jid, int event)
{
    if (!s_joystickInstance)
        return;

    // GLFW_CONNECTED / GLFW_DISCONNECTED — we can push a gamepad event
    // but our InputEventType doesn't have a dedicated connected/disconnected
    // type. The game polls isGamepadConnected() each frame, so this is fine.
    (void)jid;
    (void)event;
}
