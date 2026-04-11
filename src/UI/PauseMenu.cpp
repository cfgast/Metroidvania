#include "PauseMenu.h"
#include "UIStyle.h"

PauseMenu::PauseMenu()
{
}

void PauseMenu::open()
{
    m_open = true;
    m_selectedIndex = 0;
    m_joyUpHeld   = false;
    m_joyDownHeld = false;
}

void PauseMenu::layout(Renderer& renderer)
{
    float winW, winH;
    renderer.getWindowSize(winW, winH);

    const float panelW = 320.f;
    const float itemW  = 240.f;
    const float itemH  = 48.f;
    const float gap    = 14.f;
    const float panelH = 80.f + ITEM_COUNT * (itemH + gap);
    float px = (winW - panelW) * 0.5f;
    float py = (winH - panelH) * 0.5f;

    float startY = py + 70.f;
    for (int i = 0; i < ITEM_COUNT; ++i)
    {
        float ix = px + (panelW - itemW) * 0.5f;
        float iy = startY + static_cast<float>(i) * (itemH + gap);
        m_itemLayouts[i] = { ix, iy, itemW, itemH };
    }
}

PauseMenu::Action PauseMenu::handleEvent(const InputEvent& event)
{
    if (!m_open)
        return None;

    if (event.type == InputEventType::KeyPressed)
    {
        if (event.key == KeyCode::Escape)
        {
            m_open = false;
            return Resume;
        }
        if (event.key == KeyCode::Up)
            m_selectedIndex = (m_selectedIndex - 1 + ITEM_COUNT) % ITEM_COUNT;
        else if (event.key == KeyCode::Down)
            m_selectedIndex = (m_selectedIndex + 1) % ITEM_COUNT;
        else if (event.key == KeyCode::Enter)
        {
            Action a = ACTIONS[m_selectedIndex];
            if (a == Resume)
                m_open = false;
            return a;
        }
    }

    // Controller: A button = confirm, B/Start = resume
    if (event.type == InputEventType::GamepadButtonPressed)
    {
        GamepadButton btn = event.gamepadButton;
        if (btn == GamepadButton::A)
        {
            Action a = ACTIONS[m_selectedIndex];
            if (a == Resume)
                m_open = false;
            return a;
        }
        if (btn == GamepadButton::B || btn == GamepadButton::Start)
        {
            m_open = false;
            return Resume;
        }
    }

    // Mouse hover – update highlighted item
    if (event.type == InputEventType::MouseMoved)
    {
        float mx = static_cast<float>(event.mouseX);
        float my = static_cast<float>(event.mouseY);
        for (int i = 0; i < ITEM_COUNT; ++i)
        {
            auto& il = m_itemLayouts[i];
            if (mx >= il.x && mx <= il.x + il.w &&
                my >= il.y && my <= il.y + il.h)
            {
                m_selectedIndex = i;
                break;
            }
        }
    }

    // Mouse click – activate item
    if (event.type == InputEventType::MouseButtonPressed &&
        event.mouseButton == MouseButton::Left)
    {
        float mx = static_cast<float>(event.mouseX);
        float my = static_cast<float>(event.mouseY);
        for (int i = 0; i < ITEM_COUNT; ++i)
        {
            auto& il = m_itemLayouts[i];
            if (mx >= il.x && mx <= il.x + il.w &&
                my >= il.y && my <= il.y + il.h)
            {
                m_selectedIndex = i;
                Action a = ACTIONS[m_selectedIndex];
                if (a == Resume)
                    m_open = false;
                return a;
            }
        }
    }

    // Controller: D-pad / left stick vertical for menu navigation
    if (event.type == InputEventType::GamepadAxisMoved)
    {
        constexpr float threshold = 50.f;
        float pos = event.axisPosition;

        bool isStickY = (event.gamepadAxis == GamepadAxis::LeftY);
        bool isPovY   = (event.gamepadAxis == GamepadAxis::DPadY);

        if (isStickY || isPovY)
        {
            bool up   = isStickY ? (pos < -threshold) : (pos > threshold);
            bool down = isStickY ? (pos > threshold)  : (pos < -threshold);

            if (up && !m_joyUpHeld)
            {
                m_selectedIndex = (m_selectedIndex - 1 + ITEM_COUNT) % ITEM_COUNT;
                m_joyUpHeld = true;
            }
            else if (!up)
            {
                m_joyUpHeld = false;
            }

            if (down && !m_joyDownHeld)
            {
                m_selectedIndex = (m_selectedIndex + 1) % ITEM_COUNT;
                m_joyDownHeld = true;
            }
            else if (!down)
            {
                m_joyDownHeld = false;
            }
        }
    }

    return None;
}

void PauseMenu::render(Renderer& renderer)
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

    // Semi-transparent overlay
    {
        float r, g, b, a;
        UIStyle::overlayColor(r, g, b, a);
        renderer.drawRect(0.f, 0.f, winW, winH, r, g, b, a);
    }

    // Panel background
    const float panelW = 320.f;
    const float itemH  = 48.f;
    const float gap    = 14.f;
    const float panelH = 80.f + ITEM_COUNT * (itemH + gap);
    float px = (winW - panelW) * 0.5f;
    float py = (winH - panelH) * 0.5f;

    {
        float r, g, b, a, br, bg2, bb, ba;
        UIStyle::panelBg(r, g, b, a);
        UIStyle::panelBorder(br, bg2, bb, ba);
        renderer.drawRoundedRect(px, py, panelW, panelH,
                                 UIStyle::PANEL_CORNER_RADIUS,
                                 r, g, b, a, br, bg2, bb, ba, 1.5f);
    }

    // Title
    if (m_font)
    {
        float tR, tG, tB, tA;
        UIStyle::titleColor(tR, tG, tB, tA);
        float tw, th;
        renderer.measureText(m_font, "Paused", 28, tw, th);
        renderer.drawText(m_font, "Paused",
                          px + (panelW - tw) * 0.5f, py + 18.f,
                          28, tR, tG, tB, tA);

        // Menu items
        const float itemW = 240.f;
        for (int i = 0; i < ITEM_COUNT; ++i)
        {
            bool sel = (i == m_selectedIndex);
            auto& il = m_itemLayouts[i];
            UIStyle::drawMenuItem(renderer, m_font, LABELS[i],
                                  il.x, il.y, itemW, itemH,
                                  18, sel);
        }
    }
}
