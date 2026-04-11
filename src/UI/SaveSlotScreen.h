#pragma once

#include <vector>
#include <string>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/VideoMode.hpp>

#include "../Core/SaveSystem.h"
#include "../Rendering/Renderer.h"

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

    void render(Renderer& renderer);

private:
    void refreshSlots();
    void populateResolutions();
    void updateResolutionLabel();
    void applyResolution(sf::RenderWindow& window);
    void layout(Renderer& renderer);

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

    Renderer::FontHandle m_font = 0;

    // Slot labels (rebuilt on refreshSlots)
    std::vector<std::string> m_slotLabels;

    // Resolution combo label
    std::string m_resLabel;

    // Layout cache for hit-testing
    struct ItemLayout { float x, y, w, h; };
    std::vector<ItemLayout> m_slotLayouts;
    ItemLayout m_resLayout = {};
    ItemLayout m_controlsLayout = {};
};
