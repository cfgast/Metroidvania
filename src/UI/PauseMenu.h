#pragma once

#include <functional>
#include <string>

#include "../Input/InputTypes.h"
#include "../Rendering/Renderer.h"

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
    Action handleEvent(const InputEvent& event);

    void render(Renderer& renderer);

private:
    void layout(Renderer& renderer);

    bool m_open = false;
    int  m_selectedIndex = 0;

    // Joystick axis edge-detection to prevent repeated triggers
    bool m_joyUpHeld   = false;
    bool m_joyDownHeld = false;

    static constexpr int ITEM_COUNT = 3;
    static constexpr const char* LABELS[ITEM_COUNT] = { "Resume", "Save Game", "Quit" };
    static constexpr Action ACTIONS[ITEM_COUNT]      = { Resume, Save, Quit };

    Renderer::FontHandle m_font = 0;

    // Layout cache (set by layout(), used for hit-testing in handleEvent)
    struct ItemLayout { float x, y, w, h; };
    ItemLayout m_itemLayouts[ITEM_COUNT] = {};
};
