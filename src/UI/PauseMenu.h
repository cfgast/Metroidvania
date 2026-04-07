#pragma once

#include <functional>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Event.hpp>

namespace sf { class RenderWindow; }

// Simple pause menu shown when the player presses Escape during gameplay.
// Offers Resume, Save, and Quit options.
class PauseMenu
{
public:
    enum Action { None, Resume, Save, Quit };

    PauseMenu();

    void open();
    void close()           { m_open = false; }
    bool isOpen() const    { return m_open; }

    // Process a single event.  Returns an action when the player picks one.
    Action handleEvent(const sf::Event& event);

    void render(sf::RenderWindow& window);

private:
    void layout(const sf::RenderWindow& window);

    bool m_open = false;
    int  m_selectedIndex = 0;

    // Joystick axis edge-detection to prevent repeated triggers
    bool m_joyUpHeld   = false;
    bool m_joyDownHeld = false;

    static constexpr int ITEM_COUNT = 3;
    static constexpr const char* LABELS[ITEM_COUNT] = { "Resume", "Save Game", "Quit" };
    static constexpr Action ACTIONS[ITEM_COUNT]      = { Resume, Save, Quit };

    sf::Font           m_font;
    bool               m_fontLoaded = false;
    sf::Text           m_titleText;
    sf::RectangleShape m_overlay;
    sf::RectangleShape m_panel;

    struct MenuItem
    {
        sf::RectangleShape box;
        sf::Text           label;
    };
    MenuItem m_items[ITEM_COUNT];
};
