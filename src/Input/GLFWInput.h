#pragma once

#include "InputSystem.h"

#include <GLFW/glfw3.h>
#include <functional>
#include <queue>

// GLFW-backed InputSystem. Uses GLFW callbacks to queue InputEvents and
// GLFW polling functions for instantaneous key/gamepad state queries.
// Owned by the active Renderer, which passes its GLFWwindow* at construction.
// Optional callbacks decouple this class from any specific renderer backend.
class GLFWInput : public InputSystem
{
public:
    using ResizeCallback = std::function<void(int width, int height)>;
    using CloseCallback  = std::function<void()>;

    explicit GLFWInput(GLFWwindow* window,
                       ResizeCallback onResize = {},
                       CloseCallback  onClose  = {});
    ~GLFWInput() override;

    GLFWInput(const GLFWInput&) = delete;
    GLFWInput& operator=(const GLFWInput&) = delete;

    bool  pollEvent(InputEvent& event) override;
    bool  isKeyPressed(KeyCode key) const override;
    bool  isGamepadConnected(int id = 0) const override;
    float getGamepadAxis(int id, GamepadAxis axis) const override;
    bool  isGamepadButtonPressed(int id, GamepadButton btn) const override;
    void  setMouseCursorVisible(bool visible) override;

private:
    // GLFW callback trampolines (static → instance via s_callbackInstance)
    static void keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* win, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* win, double xpos, double ypos);
    static void windowSizeCallback(GLFWwindow* win, int width, int height);
    static void windowCloseCallback(GLFWwindow* win);
    static void joystickCallback(int jid, int event);

    // Gamepad polling — synthesizes button/axis events by diffing against
    // previous state (GLFW has no gamepad callbacks).
    void pollGamepadEvents();

    // Mapping helpers
    static KeyCode      fromGlfwKey(int glfwKey);
    static int          toGlfwKey(KeyCode code);
    static MouseButton  fromGlfwMouseButton(int glfwBtn);
    static GamepadAxis  toGlfwGamepadAxis(GamepadAxis axis);

    GLFWwindow*            m_window;
    ResizeCallback         m_onResize;
    CloseCallback          m_onClose;
    std::queue<InputEvent> m_eventQueue;

    // Previous-frame gamepad state for edge detection
    bool  m_prevButtons[static_cast<int>(GamepadButton::ButtonCount)] = {};
    float m_prevAxes[6] = {}; // LeftX, LeftY, RightX, RightY, DPadX, DPadY
    bool  m_gamepadWasConnected = false;

    // Static pointer so all GLFW callbacks can reach the instance.
    static GLFWInput* s_callbackInstance;
};
