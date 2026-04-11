#pragma once

#include <string>

#include "../Input/InputTypes.h"
#include "../Rendering/Renderer.h"

// Full-screen controls rebinding menu accessible from the main menu.
class ControlsMenu
{
public:
    ControlsMenu();

    void open();
    void close();
    bool isOpen() const { return m_open; }

    // Process one event. Call each frame while open.
    void handleEvent(const InputEvent& event);

    void render(Renderer& renderer);

private:
    void layout(Renderer& renderer);
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

    Renderer::FontHandle m_font = 0;

    // Row labels (action name on the left, binding text on the right)
    std::string m_bindingLabels[ROW_COUNT];

    // Instruction text (changes when rebinding)
    std::string m_instructionStr;

    // Layout cache for hit-testing
    struct ItemLayout { float x, y, w, h; };
    ItemLayout m_rowLayouts[ROW_COUNT] = {};
};
