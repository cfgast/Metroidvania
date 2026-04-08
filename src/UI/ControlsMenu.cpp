#include "ControlsMenu.h"
#include "UIStyle.h"

#include <SFML/Graphics/RenderWindow.hpp>

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
    m_fontLoaded = m_font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf");

    m_background.setFillColor(UIStyle::panelBg());

    if (m_fontLoaded)
    {
        m_titleText.setFont(m_font);
        m_titleText.setString("Controls");
        m_titleText.setCharacterSize(30);
        m_titleText.setFillColor(UIStyle::titleColor());

        m_sectionKb.setFont(m_font);
        m_sectionKb.setString("Keyboard");
        m_sectionKb.setCharacterSize(16);
        m_sectionKb.setFillColor(UIStyle::sectionColor());

        m_sectionCtrl.setFont(m_font);
        m_sectionCtrl.setString("Controller");
        m_sectionCtrl.setCharacterSize(16);
        m_sectionCtrl.setFillColor(UIStyle::sectionColor());

        m_instructionText.setFont(m_font);
        m_instructionText.setCharacterSize(14);
        m_instructionText.setFillColor(UIStyle::hintColor());

        for (int i = 0; i < ROW_COUNT; ++i)
        {
            m_rows[i].box.setParameters({ 500.f, 48.f }, UIStyle::CORNER_RADIUS);
            m_rows[i].box.setOutlineThickness(1.f);

            m_rows[i].actionLabel.setFont(m_font);
            m_rows[i].actionLabel.setCharacterSize(17);
            m_rows[i].actionLabel.setString(ROW_ACTIONS[i]);

            m_rows[i].bindingLabel.setFont(m_font);
            m_rows[i].bindingLabel.setCharacterSize(17);
        }
    }

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
    if (!m_fontLoaded)
        return;

    auto& b = InputBindings::instance();

    auto setBinding = [&](int row, const std::string& text) {
        m_rows[row].bindingLabel.setString(text);
    };

    setBinding(MoveLeftKey,     InputBindings::keyToString(b.moveLeftKey));
    setBinding(MoveLeftAltKey,  InputBindings::keyToString(b.moveLeftAlt));
    setBinding(MoveRightKey,    InputBindings::keyToString(b.moveRightKey));
    setBinding(MoveRightAltKey, InputBindings::keyToString(b.moveRightAlt));
    setBinding(JumpKey,         InputBindings::keyToString(b.jumpKey));
    setBinding(DashKey,         InputBindings::keyToString(b.dashKey));
    setBinding(AttackKey,       InputBindings::keyToString(b.attackKey));
    setBinding(ControllerJump,  InputBindings::buttonToString(b.controllerJumpButton));
    setBinding(ControllerDash,  InputBindings::buttonToString(b.controllerDashButton));
    setBinding(ControllerAttack, InputBindings::buttonToString(b.controllerAttackButton));

    // Non-binding rows have no right-side text
    setBinding(ResetDefaults, "");
    setBinding(Back, "");

    // Update instruction text based on state
    if (m_rebinding)
    {
        if (isControllerRow(m_selected))
            m_instructionText.setString("Press a controller button to bind...");
        else
            m_instructionText.setString("Press a key to bind...");
    }
    else
    {
        m_instructionText.setString(
            "Up/Down: navigate  |  Enter/A: rebind  |  Esc/B: back");
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
            // Waiting for a controller button press
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
            // Allow Escape to cancel
            if (event.type == sf::Event::KeyPressed
                && event.key.code == sf::Keyboard::Escape)
            {
                m_rebinding = false;
                refreshLabels();
            }
        }
        else
        {
            // Waiting for a keyboard key press
            if (event.type == sf::Event::KeyPressed)
            {
                sf::Keyboard::Key newKey = event.key.code;

                // Escape cancels the rebind
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
        return; // Don't process navigation while rebinding
    }

    // --- Normal navigation ---

    // Mouse hover – highlight row under cursor
    if (event.type == sf::Event::MouseMoved)
    {
        sf::Vector2f mouse(static_cast<float>(event.mouseMove.x),
                           static_cast<float>(event.mouseMove.y));
        for (int i = 0; i < ROW_COUNT; ++i)
        {
            if (m_rows[i].box.getGlobalBounds().contains(mouse))
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
        sf::Vector2f mouse(static_cast<float>(event.mouseButton.x),
                           static_cast<float>(event.mouseButton.y));
        for (int i = 0; i < ROW_COUNT; ++i)
        {
            if (m_rows[i].box.getGlobalBounds().contains(mouse))
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
        if (btn == 0) // A button
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
        else if (btn == 1) // B button
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

void ControlsMenu::layout(const sf::RenderWindow& window)
{
    const float winW = static_cast<float>(window.getSize().x);
    const float winH = static_cast<float>(window.getSize().y);

    m_background.setSize({ winW, winH });
    m_background.setPosition(0.f, 0.f);

    const float rowW   = 500.f;
    const float rowH   = 48.f;
    const float gap    = 10.f;
    const float sectionGap = 28.f;

    // Vertical layout: title, section header, keyboard rows, section header,
    // controller rows, gap, reset, back
    float totalH = 60.f              // title area
               + 24.f                // keyboard section label
               + 7 * (rowH + gap)    // 7 keyboard rows (through attack)
               + sectionGap
               + 24.f                // controller section label
               + 3 * (rowH + gap)    // 3 controller rows (jump + dash + attack)
               + sectionGap
               + 2 * (rowH + gap);   // reset + back
    float startY = (winH - totalH) * 0.5f;
    float cx     = (winW - rowW) * 0.5f;

    // Title
    if (m_fontLoaded)
    {
        const sf::FloatRect tb = m_titleText.getLocalBounds();
        m_titleText.setPosition((winW - tb.width) * 0.5f, startY);
    }
    float y = startY + 60.f;

    // Keyboard section label
    if (m_fontLoaded)
    {
        const sf::FloatRect sb = m_sectionKb.getLocalBounds();
        m_sectionKb.setPosition((winW - sb.width) * 0.5f, y);
    }
    y += 28.f;

    // Keyboard rows: MoveLeftKey .. JumpKey (indices 0-4)
    for (int i = MoveLeftKey; i <= AttackKey; ++i)
    {
        auto& rw = m_rows[i];
        rw.box.setParameters({ rowW, rowH }, UIStyle::CORNER_RADIUS);
        rw.box.setPosition(cx, y);
        y += rowH + gap;
    }

    // Controller section
    y += sectionGap - gap;
    if (m_fontLoaded)
    {
        const sf::FloatRect sb = m_sectionCtrl.getLocalBounds();
        m_sectionCtrl.setPosition((winW - sb.width) * 0.5f, y);
    }
    y += 28.f;

    // Controller jump and dash rows
    for (int i = ControllerJump; i <= ControllerAttack; ++i)
    {
        auto& rw = m_rows[i];
        rw.box.setParameters({ rowW, rowH }, UIStyle::CORNER_RADIUS);
        rw.box.setPosition(cx, y);
        y += rowH + gap;
    }

    // Gap before Reset/Back
    y += sectionGap - gap;

    // Reset Defaults and Back
    for (int i = ResetDefaults; i <= Back; ++i)
    {
        auto& rw = m_rows[i];
        rw.box.setParameters({ rowW, rowH }, UIStyle::CORNER_RADIUS);
        rw.box.setPosition(cx, y);
        y += rowH + gap;
    }

    // Instruction text at bottom
    if (m_fontLoaded)
    {
        const sf::FloatRect ib = m_instructionText.getLocalBounds();
        m_instructionText.setPosition((winW - ib.width) * 0.5f, winH - 50.f);
    }
}

void ControlsMenu::render(sf::RenderWindow& window)
{
    if (!m_open)
        return;

    layout(window);

    sf::View prev = window.getView();
    sf::View uiView(sf::FloatRect(0.f, 0.f,
                                   static_cast<float>(window.getSize().x),
                                   static_cast<float>(window.getSize().y)));
    window.setView(uiView);

    window.draw(m_background);

    if (m_fontLoaded)
    {
        window.draw(m_titleText);
        window.draw(m_sectionKb);
        window.draw(m_sectionCtrl);

        const float rowW = 500.f;
        const float rowH = 48.f;

        // Binding rows (keyboard + controller)
        for (int i = MoveLeftKey; i <= ControllerAttack; ++i)
        {
            bool sel = (i == m_selected);
            bool rebindThis = (m_rebinding && i == m_selected);
            sf::Vector2f pos = m_rows[i].box.getPosition();
            UIStyle::drawMenuRow(window, m_rows[i].box,
                                 m_rows[i].actionLabel,
                                 &m_rows[i].bindingLabel,
                                 pos.x, pos.y, rowW, rowH,
                                 sel, rebindThis);
        }

        // Reset Defaults and Back (centered text, no right label)
        for (int i = ResetDefaults; i <= Back; ++i)
        {
            bool sel = (i == m_selected);
            sf::Vector2f pos = m_rows[i].box.getPosition();
            UIStyle::drawMenuItem(window, m_rows[i].box,
                                  m_rows[i].actionLabel,
                                  pos.x, pos.y, rowW, rowH, sel);
        }

        window.draw(m_instructionText);
    }

    window.setView(prev);
}
