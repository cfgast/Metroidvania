#pragma once

#include "InputSystem.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <queue>

class GLRenderer;

// GLFW-backed InputSystem. Uses GLFW callbacks to queue InputEvents and
// GLFW polling functions for instantaneous key/gamepad state queries.
// Owned by GLRenderer, which passes its GLFWwindow* at construction.
class GLFWInput : public InputSystem
{
public:
    explicit GLFWInput(GLFWwindow* window, GLRenderer& renderer);
    ~GLFWInput() override = default;

    GLFWInput(const GLFWInput&) = delete;
    GLFWInput& operator=(const GLFWInput&) = delete;

    bool  pollEvent(InputEvent& event) override;
    bool  isKeyPressed(KeyCode key) const override;
    bool  isGamepadConnected(int id = 0) const override;
    float getGamepadAxis(int id, GamepadAxis axis) const override;
    bool  isGamepadButtonPressed(int id, GamepadButton btn) const override;
    void  setMouseCursorVisible(bool visible) override;

private:
    // GLFW callback trampolines (static → instance via glfwGetWindowUserPointer)
    static void keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* win, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* win, double xpos, double ypos);
    static void windowSizeCallback(GLFWwindow* win, int width, int height);
    static void windowCloseCallback(GLFWwindow* win);
    static void joystickCallback(int jid, int event);

    // Mapping helpers
    static KeyCode      fromGlfwKey(int glfwKey);
    static int          toGlfwKey(KeyCode code);
    static MouseButton  fromGlfwMouseButton(int glfwBtn);
    static GamepadAxis  toGlfwGamepadAxis(GamepadAxis axis);

    GLFWwindow*            m_window;
    GLRenderer&            m_renderer;
    std::queue<InputEvent> m_eventQueue;

    // Static pointer so the joystick callback (which has no window param) can
    // reach the instance.
    static GLFWInput* s_joystickInstance;
};
