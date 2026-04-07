#pragma once

#include <vector>
#include <string>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/VideoMode.hpp>

#include "../Core/SaveSystem.h"

namespace sf { class RenderWindow; }

// Result produced by the save-slot selection screen each frame.
struct SaveSlotResult
{
    enum Action { None, NewGame, LoadSlot, Controls };
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
    // Window is non-const so the resolution selector can resize it.
    SaveSlotResult handleEvent(const sf::Event& event, sf::RenderWindow& window);

    void render(sf::RenderWindow& window);

private:
    void refreshSlots();
    void populateResolutions();
    void updateResolutionLabel();
    void applyResolution(sf::RenderWindow& window);
    void layout(const sf::RenderWindow& window);

    int  totalItemCount() const;
    bool isOnResolutionRow() const;
    bool isOnControlsRow() const;

    bool m_open = false;
    int  m_selectedIndex = 0;   // 0..slots+resolution

    std::vector<SaveSlotInfo> m_slots;

    // --- Resolution selector ---
    struct Resolution { unsigned width; unsigned height; };
    std::vector<Resolution> m_resolutions;
    int m_resolutionIndex = 0;

    // Joystick axis edge-detection to prevent repeated triggers
    bool m_joyUpHeld    = false;
    bool m_joyDownHeld  = false;
    bool m_joyLeftHeld  = false;
    bool m_joyRightHeld = false;

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

    // Resolution combo box widget
    SlotWidget m_resWidget;

    // Controls button widget
    SlotWidget m_controlsWidget;
};
