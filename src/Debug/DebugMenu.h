#pragma once

#include <string>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Event.hpp>

namespace sf { class RenderWindow; }

class DebugMenu
{
public:
    DebugMenu();

    // Feed every window event through here while the game loop runs.
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);

    // Returns the path the user selected via the file dialog, then clears it.
    // Returns an empty string when nothing new was chosen.
    std::string pollSelectedMap();

    void render(sf::RenderWindow& window);

    bool isOpen() const { return m_open; }

private:
    void openFileDialog();
    void layout(const sf::RenderWindow& window);

    bool        m_open       = false;
    std::string m_pendingMap;

    sf::Font            m_font;
    bool                m_fontLoaded = false;

    sf::RectangleShape  m_panel;
    sf::RectangleShape  m_button;
    sf::Text            m_titleText;
    sf::Text            m_buttonText;
};
