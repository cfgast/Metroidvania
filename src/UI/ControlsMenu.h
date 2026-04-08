#pragma once

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Event.hpp>

#include "RoundedRectangleShape.h"

namespace sf { class RenderWindow; }

// Full-screen controls rebinding menu accessible from the main menu.
class ControlsMenu
{
public:
    ControlsMenu();

    void open();
    void close();
    bool isOpen() const { return m_open; }

    // Process one event. Call each frame while open.
    void handleEvent(const sf::Event& event);

    void render(sf::RenderWindow& window);

private:
    void layout(const sf::RenderWindow& window);
    void refreshLabels();

    bool m_open     = false;
    int  m_selected = 0;

    // When true, the selected row is waiting for a key/button press.
    bool m_rebinding = false;

    // Joystick edge-detection
    bool m_joyUpHeld   = false;
    bool m_joyDownHeld = false;

    // Row indices
    enum Row
    {
        MoveLeftKey,
        MoveLeftAltKey,
        MoveRightKey,
        MoveRightAltKey,
        JumpKey,
        DashKey,
        AttackKey,
        ControllerJump,
        ControllerDash,
        ControllerAttack,
        ResetDefaults,
        Back,
        ROW_COUNT
    };

    // True if the row is a controller binding (listens for button, not key).
    bool isControllerRow(int row) const { return row == ControllerJump || row == ControllerDash || row == ControllerAttack; }
    // True if the row is a rebindable binding (keyboard or controller).
    bool isBindingRow(int row) const;

    sf::Font           m_font;
    bool               m_fontLoaded = false;
    sf::Text           m_titleText;
    sf::Text           m_sectionKb;
    sf::Text           m_sectionCtrl;
    sf::Text           m_instructionText;
    sf::RectangleShape m_background;

    struct RowWidget
    {
        RoundedRectangleShape box;
        sf::Text              actionLabel;
        sf::Text              bindingLabel;
    };
    RowWidget m_rows[ROW_COUNT];
};
