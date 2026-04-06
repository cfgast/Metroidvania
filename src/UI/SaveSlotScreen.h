#pragma once

#include <vector>
#include <string>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Event.hpp>

#include "../Core/SaveSystem.h"

namespace sf { class RenderWindow; }

// Result produced by the save-slot selection screen each frame.
struct SaveSlotResult
{
    enum Action { None, NewGame, LoadSlot };
    Action action  = None;
    int    slot    = 0;      // 1-based slot chosen
};

// Full-screen save-slot selection menu shown before gameplay begins.
class SaveSlotScreen
{
public:
    SaveSlotScreen();

    void open();
    void close()               { m_open = false; }
    bool isOpen() const        { return m_open; }

    // Handle input.  Returns an action when the player makes a choice.
    SaveSlotResult handleEvent(const sf::Event& event, const sf::RenderWindow& window);

    void render(sf::RenderWindow& window);

private:
    void refreshSlots();
    void layout(const sf::RenderWindow& window);

    bool m_open = false;
    int  m_selectedIndex = 0;   // 0..MAX_SLOTS-1

    std::vector<SaveSlotInfo> m_slots;

    sf::Font           m_font;
    bool               m_fontLoaded = false;
    sf::Text           m_titleText;
    sf::Text           m_instructionText;
    sf::RectangleShape m_background;

    struct SlotWidget
    {
        sf::RectangleShape box;
        sf::Text           label;
    };
    std::vector<SlotWidget> m_widgets;
};
