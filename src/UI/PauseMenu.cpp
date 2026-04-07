#include "PauseMenu.h"
#include "UIStyle.h"

#include <SFML/Graphics/RenderWindow.hpp>

PauseMenu::PauseMenu()
{
    m_fontLoaded = m_font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf");

    m_overlay.setFillColor(UIStyle::overlayColor());

    m_panel.setParameters({ 320.f, 260.f }, UIStyle::PANEL_CORNER_RADIUS);
    m_panel.setFillColor(UIStyle::panelBg());
    m_panel.setOutlineColor(UIStyle::panelBorder());
    m_panel.setOutlineThickness(1.5f);

    if (m_fontLoaded)
    {
        m_titleText.setFont(m_font);
        m_titleText.setString("Paused");
        m_titleText.setCharacterSize(28);
        m_titleText.setFillColor(UIStyle::titleColor());

        for (int i = 0; i < ITEM_COUNT; ++i)
        {
            m_items[i].box.setParameters({ 240.f, 48.f }, UIStyle::CORNER_RADIUS);
            m_items[i].box.setOutlineThickness(1.f);

            m_items[i].label.setFont(m_font);
            m_items[i].label.setString(LABELS[i]);
            m_items[i].label.setCharacterSize(18);
        }
    }
}

void PauseMenu::open()
{
    m_open = true;
    m_selectedIndex = 0;
    m_joyUpHeld   = false;
    m_joyDownHeld = false;
}

void PauseMenu::layout(const sf::RenderWindow& window)
{
    const float winW = static_cast<float>(window.getSize().x);
    const float winH = static_cast<float>(window.getSize().y);

    m_overlay.setSize({ winW, winH });
    m_overlay.setPosition(0.f, 0.f);

    const float panelW = 320.f;
    const float itemW  = 240.f;
    const float itemH  = 48.f;
    const float gap    = 14.f;
    const float panelH = 80.f + ITEM_COUNT * (itemH + gap);
    m_panel.setParameters({ panelW, panelH }, UIStyle::PANEL_CORNER_RADIUS);
    float px = (winW - panelW) * 0.5f;
    float py = (winH - panelH) * 0.5f;
    m_panel.setPosition(px, py);

    if (m_fontLoaded)
    {
        const sf::FloatRect tb = m_titleText.getLocalBounds();
        m_titleText.setPosition(px + (panelW - tb.width) * 0.5f, py + 18.f);
    }

    float startY = py + 70.f;
    for (int i = 0; i < ITEM_COUNT; ++i)
    {
        float ix = px + (panelW - itemW) * 0.5f;
        float iy = startY + static_cast<float>(i) * (itemH + gap);
        m_items[i].box.setParameters({ itemW, itemH }, UIStyle::CORNER_RADIUS);
        m_items[i].box.setPosition(ix, iy);
    }
}

PauseMenu::Action PauseMenu::handleEvent(const sf::Event& event)
{
    if (!m_open)
        return None;

    if (event.type == sf::Event::KeyPressed)
    {
        if (event.key.code == sf::Keyboard::Escape)
        {
            m_open = false;
            return Resume;
        }
        if (event.key.code == sf::Keyboard::Up)
            m_selectedIndex = (m_selectedIndex - 1 + ITEM_COUNT) % ITEM_COUNT;
        else if (event.key.code == sf::Keyboard::Down)
            m_selectedIndex = (m_selectedIndex + 1) % ITEM_COUNT;
        else if (event.key.code == sf::Keyboard::Return)
        {
            Action a = ACTIONS[m_selectedIndex];
            if (a == Resume)
                m_open = false;
            return a;
        }
    }

    // Controller: A button = confirm, B/Start = resume
    if (event.type == sf::Event::JoystickButtonPressed)
    {
        unsigned int btn = event.joystickButton.button;
        if (btn == 0) // A button - confirm
        {
            Action a = ACTIONS[m_selectedIndex];
            if (a == Resume)
                m_open = false;
            return a;
        }
        if (btn == 1 || btn == 7) // B button or Start - resume
        {
            m_open = false;
            return Resume;
        }
    }

    // Mouse hover – update highlighted item
    if (event.type == sf::Event::MouseMoved)
    {
        sf::Vector2f mouse(static_cast<float>(event.mouseMove.x),
                           static_cast<float>(event.mouseMove.y));
        for (int i = 0; i < ITEM_COUNT; ++i)
        {
            if (m_items[i].box.getGlobalBounds().contains(mouse))
            {
                m_selectedIndex = i;
                break;
            }
        }
    }

    // Mouse click – activate item
    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left)
    {
        sf::Vector2f mouse(static_cast<float>(event.mouseButton.x),
                           static_cast<float>(event.mouseButton.y));
        for (int i = 0; i < ITEM_COUNT; ++i)
        {
            if (m_items[i].box.getGlobalBounds().contains(mouse))
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
    if (event.type == sf::Event::JoystickMoved)
    {
        constexpr float threshold = 50.f;
        float pos = event.joystickMove.position;

        bool isStickY = (event.joystickMove.axis == sf::Joystick::Y);
        bool isPovY   = (event.joystickMove.axis == sf::Joystick::PovY);

        if (isStickY || isPovY)
        {
            // Stick Y: negative=up, positive=down. PovY: positive=up, negative=down.
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

void PauseMenu::render(sf::RenderWindow& window)
{
    if (!m_open)
        return;

    layout(window);

    sf::View prev = window.getView();
    sf::View uiView(sf::FloatRect(0.f, 0.f,
                                   static_cast<float>(window.getSize().x),
                                   static_cast<float>(window.getSize().y)));
    window.setView(uiView);

    window.draw(m_overlay);
    window.draw(m_panel);

    if (m_fontLoaded)
    {
        window.draw(m_titleText);

        const float itemW = 240.f;
        const float itemH = 48.f;

        for (int i = 0; i < ITEM_COUNT; ++i)
        {
            bool sel = (i == m_selectedIndex);
            sf::Vector2f pos = m_items[i].box.getPosition();
            UIStyle::drawMenuItem(window, m_items[i].box, m_items[i].label,
                                  pos.x, pos.y, itemW, itemH, sel);
        }
    }

    window.setView(prev);
}
