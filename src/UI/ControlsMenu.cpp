#include "ControlsMenu.h"
#include "UIStyle.h"

#include "../Core/InputBindings.h"

// Row action labels (left column)
static const char* ROW_ACTIONS[] = {
    "Move Left",
    "Move Left (Alt)",
    "Move Right",
    "Move Right (Alt)",
    "Jump",
    "Dash",
    "Attack",
    "Jump",
    "Dash",
    "Attack",
    "Reset Defaults",
    "Back"
};

ControlsMenu::ControlsMenu()
{
    refreshLabels();
}

bool ControlsMenu::isBindingRow(int row) const
{
    return row >= MoveLeftKey && row <= ControllerAttack;
}

void ControlsMenu::open()
{
    m_open     = true;
    m_selected = 0;
    m_rebinding = false;
    m_joyUpHeld   = false;
    m_joyDownHeld = false;
    refreshLabels();
}

void ControlsMenu::close()
{
    m_open      = false;
    m_rebinding = false;
}

void ControlsMenu::refreshLabels()
{
    auto& b = InputBindings::instance();

    m_bindingLabels[MoveLeftKey]      = InputBindings::keyToString(b.moveLeftKey);
    m_bindingLabels[MoveLeftAltKey]   = InputBindings::keyToString(b.moveLeftAlt);
    m_bindingLabels[MoveRightKey]     = InputBindings::keyToString(b.moveRightKey);
    m_bindingLabels[MoveRightAltKey]  = InputBindings::keyToString(b.moveRightAlt);
    m_bindingLabels[JumpKey]          = InputBindings::keyToString(b.jumpKey);
    m_bindingLabels[DashKey]          = InputBindings::keyToString(b.dashKey);
    m_bindingLabels[AttackKey]        = InputBindings::keyToString(b.attackKey);
    m_bindingLabels[ControllerJump]   = InputBindings::buttonToString(b.controllerJumpButton);
    m_bindingLabels[ControllerDash]   = InputBindings::buttonToString(b.controllerDashButton);
    m_bindingLabels[ControllerAttack] = InputBindings::buttonToString(b.controllerAttackButton);
    m_bindingLabels[ResetDefaults]    = "";
    m_bindingLabels[Back]             = "";

    if (m_rebinding)
    {
        if (isControllerRow(m_selected))
            m_instructionStr = "Press a controller button to bind...";
        else
            m_instructionStr = "Press a key to bind...";
    }
    else
    {
        m_instructionStr = "Up/Down: navigate  |  Enter/A: rebind  |  Esc/B: back";
    }
}

void ControlsMenu::handleEvent(const sf::Event& event)
{
    if (!m_open)
        return;

    auto& bindings = InputBindings::instance();

    // --- Rebinding mode: capture next key or button press ---
    if (m_rebinding)
    {
        if (isControllerRow(m_selected))
        {
            if (event.type == sf::Event::JoystickButtonPressed)
            {
                unsigned int btn = event.joystickButton.button;
                switch (m_selected)
                {
                    case ControllerJump: bindings.controllerJumpButton = btn; break;
                    case ControllerDash: bindings.controllerDashButton = btn; break;
                    case ControllerAttack: bindings.controllerAttackButton = btn; break;
                    default: break;
                }
                bindings.save();
                m_rebinding = false;
                refreshLabels();
            }
            if (event.type == sf::Event::KeyPressed
                && event.key.code == sf::Keyboard::Escape)
            {
                m_rebinding = false;
                refreshLabels();
            }
        }
        else
        {
            if (event.type == sf::Event::KeyPressed)
            {
                sf::Keyboard::Key newKey = event.key.code;

                if (newKey == sf::Keyboard::Escape)
                {
                    m_rebinding = false;
                    refreshLabels();
                    return;
                }

                switch (m_selected)
                {
                    case MoveLeftKey:     bindings.moveLeftKey    = newKey; break;
                    case MoveLeftAltKey:  bindings.moveLeftAlt    = newKey; break;
                    case MoveRightKey:    bindings.moveRightKey   = newKey; break;
                    case MoveRightAltKey: bindings.moveRightAlt   = newKey; break;
                    case JumpKey:         bindings.jumpKey        = newKey; break;
                    case DashKey:         bindings.dashKey        = newKey; break;
                    case AttackKey:       bindings.attackKey      = newKey; break;
                    default: break;
                }
                bindings.save();
                m_rebinding = false;
                refreshLabels();
            }
        }
        return;
    }

    // --- Normal navigation ---

    // Mouse hover – highlight row under cursor
    if (event.type == sf::Event::MouseMoved)
    {
        float mx = static_cast<float>(event.mouseMove.x);
        float my = static_cast<float>(event.mouseMove.y);
        for (int i = 0; i < ROW_COUNT; ++i)
        {
            auto& il = m_rowLayouts[i];
            if (mx >= il.x && mx <= il.x + il.w &&
                my >= il.y && my <= il.y + il.h)
            {
                m_selected = i;
                break;
            }
        }
    }

    // Mouse click – activate row
    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left)
    {
        float mx = static_cast<float>(event.mouseButton.x);
        float my = static_cast<float>(event.mouseButton.y);
        for (int i = 0; i < ROW_COUNT; ++i)
        {
            auto& il = m_rowLayouts[i];
            if (mx >= il.x && mx <= il.x + il.w &&
                my >= il.y && my <= il.y + il.h)
            {
                m_selected = i;
                if (i == Back)
                {
                    close();
                    return;
                }
                if (i == ResetDefaults)
                {
                    bindings.resetDefaults();
                    bindings.save();
                    refreshLabels();
                    return;
                }
                if (isBindingRow(i))
                {
                    m_rebinding = true;
                    refreshLabels();
                }
                return;
            }
        }
    }

    if (event.type == sf::Event::KeyPressed)
    {
        if (event.key.code == sf::Keyboard::Escape)
        {
            close();
            return;
        }
        if (event.key.code == sf::Keyboard::Up)
            m_selected = (m_selected - 1 + ROW_COUNT) % ROW_COUNT;
        else if (event.key.code == sf::Keyboard::Down)
            m_selected = (m_selected + 1) % ROW_COUNT;
        else if (event.key.code == sf::Keyboard::Return)
        {
            if (m_selected == Back)
            {
                close();
                return;
            }
            if (m_selected == ResetDefaults)
            {
                bindings.resetDefaults();
                bindings.save();
                refreshLabels();
                return;
            }
            if (isBindingRow(m_selected))
            {
                m_rebinding = true;
                refreshLabels();
            }
        }
    }

    // Controller: A = confirm, B = back
    if (event.type == sf::Event::JoystickButtonPressed)
    {
        unsigned int btn = event.joystickButton.button;
        if (btn == 0)
        {
            if (m_selected == Back)
            {
                close();
                return;
            }
            if (m_selected == ResetDefaults)
            {
                bindings.resetDefaults();
                bindings.save();
                refreshLabels();
                return;
            }
            if (isBindingRow(m_selected))
            {
                m_rebinding = true;
                refreshLabels();
            }
        }
        else if (btn == 1)
        {
            close();
            return;
        }
    }

    // Controller: D-pad / left stick vertical navigation
    if (event.type == sf::Event::JoystickMoved)
    {
        constexpr float threshold = 50.f;
        float pos = event.joystickMove.position;

        bool isStickY = (event.joystickMove.axis == sf::Joystick::Y);
        bool isPovY   = (event.joystickMove.axis == sf::Joystick::PovY);

        if (isStickY || isPovY)
        {
            bool up   = isStickY ? (pos < -threshold) : (pos > threshold);
            bool down = isStickY ? (pos > threshold)  : (pos < -threshold);

            if (up && !m_joyUpHeld)
            {
                m_selected = (m_selected - 1 + ROW_COUNT) % ROW_COUNT;
                m_joyUpHeld = true;
            }
            else if (!up)
            {
                m_joyUpHeld = false;
            }

            if (down && !m_joyDownHeld)
            {
                m_selected = (m_selected + 1) % ROW_COUNT;
                m_joyDownHeld = true;
            }
            else if (!down)
            {
                m_joyDownHeld = false;
            }
        }
    }
}

void ControlsMenu::layout(Renderer& renderer)
{
    float winW, winH;
    renderer.getWindowSize(winW, winH);

    const float rowW   = 500.f;
    const float rowH   = 48.f;
    const float gap    = 10.f;
    const float sectionGap = 28.f;

    float totalH = 60.f
               + 24.f
               + 7 * (rowH + gap)
               + sectionGap
               + 24.f
               + 3 * (rowH + gap)
               + sectionGap
               + 2 * (rowH + gap);
    float startY = (winH - totalH) * 0.5f;
    float cx     = (winW - rowW) * 0.5f;

    float y = startY + 60.f;

    // Keyboard section label position
    y += 28.f;

    // Keyboard rows
    for (int i = MoveLeftKey; i <= AttackKey; ++i)
    {
        m_rowLayouts[i] = { cx, y, rowW, rowH };
        y += rowH + gap;
    }

    // Controller section
    y += sectionGap - gap;
    y += 28.f;

    for (int i = ControllerJump; i <= ControllerAttack; ++i)
    {
        m_rowLayouts[i] = { cx, y, rowW, rowH };
        y += rowH + gap;
    }

    // Gap before Reset/Back
    y += sectionGap - gap;

    for (int i = ResetDefaults; i <= Back; ++i)
    {
        m_rowLayouts[i] = { cx, y, rowW, rowH };
        y += rowH + gap;
    }
}

void ControlsMenu::render(Renderer& renderer)
{
    if (!m_open)
        return;

    // Lazy-load font on first render
    if (m_font == 0)
        m_font = renderer.loadFont("C:\\Windows\\Fonts\\arial.ttf");

    layout(renderer);

    float winW, winH;
    renderer.getWindowSize(winW, winH);

    renderer.resetView();

    // Full-screen background
    {
        float r, g, b, a;
        UIStyle::panelBg(r, g, b, a);
        renderer.drawRect(0.f, 0.f, winW, winH, r, g, b, a);
    }

    if (!m_font)
        return;

    // Recompute positions for section labels and title
    const float rowW   = 500.f;
    const float rowH   = 48.f;
    const float gap    = 10.f;
    const float sectionGap = 28.f;

    float totalH = 60.f + 24.f + 7 * (rowH + gap) + sectionGap
                 + 24.f + 3 * (rowH + gap) + sectionGap + 2 * (rowH + gap);
    float startY = (winH - totalH) * 0.5f;

    // Title
    {
        float tR, tG, tB, tA;
        UIStyle::titleColor(tR, tG, tB, tA);
        float tw, th;
        renderer.measureText(m_font, "Controls", 30, tw, th);
        renderer.drawText(m_font, "Controls",
                          (winW - tw) * 0.5f, startY,
                          30, tR, tG, tB, tA);
    }

    float y = startY + 60.f;

    // Keyboard section label
    {
        float sR, sG, sB, sA;
        UIStyle::sectionColor(sR, sG, sB, sA);
        float sw, sh;
        renderer.measureText(m_font, "Keyboard", 16, sw, sh);
        renderer.drawText(m_font, "Keyboard",
                          (winW - sw) * 0.5f, y,
                          16, sR, sG, sB, sA);
    }
    y += 28.f;

    // Keyboard binding rows
    for (int i = MoveLeftKey; i <= AttackKey; ++i)
    {
        bool sel = (i == m_selected);
        bool rebindThis = (m_rebinding && i == m_selected);
        UIStyle::drawMenuRow(renderer, m_font,
                             ROW_ACTIONS[i],
                             &m_bindingLabels[i],
                             m_rowLayouts[i].x, m_rowLayouts[i].y, rowW, rowH,
                             17, sel, rebindThis);
        y += rowH + gap;
    }

    // Controller section
    y += sectionGap - gap;
    {
        float sR, sG, sB, sA;
        UIStyle::sectionColor(sR, sG, sB, sA);
        float sw, sh;
        renderer.measureText(m_font, "Controller", 16, sw, sh);
        renderer.drawText(m_font, "Controller",
                          (winW - sw) * 0.5f, y,
                          16, sR, sG, sB, sA);
    }
    y += 28.f;

    // Controller binding rows
    for (int i = ControllerJump; i <= ControllerAttack; ++i)
    {
        bool sel = (i == m_selected);
        bool rebindThis = (m_rebinding && i == m_selected);
        UIStyle::drawMenuRow(renderer, m_font,
                             ROW_ACTIONS[i],
                             &m_bindingLabels[i],
                             m_rowLayouts[i].x, m_rowLayouts[i].y, rowW, rowH,
                             17, sel, rebindThis);
    }

    // Reset Defaults and Back (centered text, no right label)
    for (int i = ResetDefaults; i <= Back; ++i)
    {
        bool sel = (i == m_selected);
        std::string action = ROW_ACTIONS[i];
        UIStyle::drawMenuItem(renderer, m_font, action,
                              m_rowLayouts[i].x, m_rowLayouts[i].y, rowW, rowH,
                              17, sel);
    }

    // Instruction text at bottom
    {
        float hR, hG, hB, hA;
        UIStyle::hintColor(hR, hG, hB, hA);
        float iw, ih;
        renderer.measureText(m_font, m_instructionStr, 14, iw, ih);
        renderer.drawText(m_font, m_instructionStr,
                          (winW - iw) * 0.5f, winH - 50.f,
                          14, hR, hG, hB, hA);
    }
}
