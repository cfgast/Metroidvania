#pragma once

#include "InputSystem.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

// SFML-backed InputSystem.  Wraps sf::Event, sf::Keyboard, sf::Joystick and
// sf::Mouse behind the backend-agnostic InputSystem interface.
class SFMLInput : public InputSystem
{
public:
    explicit SFMLInput(sf::RenderWindow& window);

    bool  pollEvent(InputEvent& event) override;
    bool  isKeyPressed(KeyCode key) const override;
    bool  isGamepadConnected(int id = 0) const override;
    float getGamepadAxis(int id, GamepadAxis axis) const override;
    bool  isGamepadButtonPressed(int id, GamepadButton btn) const override;
    void  setMouseCursorVisible(bool visible) override;

private:
    sf::RenderWindow& m_window;

    bool translateEvent(const sf::Event& sfEvent, InputEvent& out) const;
};
