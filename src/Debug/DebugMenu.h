#pragma once

#include <string>

#include "../Input/InputTypes.h"
#include "../Rendering/Renderer.h"

class DebugMenu
{
public:
    DebugMenu() = default;

    // Feed every window event through here while the game loop runs.
    void handleEvent(const InputEvent& event);

    // Returns the path the user selected via the file dialog, then clears it.
    // Returns an empty string when nothing new was chosen.
    std::string pollSelectedMap();

    void render(Renderer& renderer);

    bool isOpen() const { return m_open; }

private:
    void openFileDialog();

    bool        m_open       = false;
    std::string m_pendingMap;

    Renderer::FontHandle m_font = 0;

    // Layout cache for hit-testing
    struct ItemLayout { float x, y, w, h; };
    ItemLayout m_buttonLayout = {};
};
