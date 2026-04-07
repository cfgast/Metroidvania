#include "PauseMenu.h"

#include <SFML/Graphics/RenderWindow.hpp>

PauseMenu::PauseMenu()
{
    m_fontLoaded = m_font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf");

    m_overlay.setFillColor(sf::Color(0, 0, 0, 120));

    m_panel.setFillColor(sf::Color(25, 25, 40, 230));
    m_panel.setOutlineColor(sf::Color(100, 160, 255));
    m_panel.setOutlineThickness(2.f);

    if (m_fontLoaded)
    {
        m_titleText.setFont(m_font);
        m_titleText.setString("Paused");
        m_titleText.setCharacterSize(26);
        m_titleText.setFillColor(sf::Color(220, 220, 255));

        for (int i = 0; i < ITEM_COUNT; ++i)
        {
            m_items[i].box.setSize({ 220.f, 44.f });
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
}

void PauseMenu::layout(const sf::RenderWindow& window)
{
    const float winW = static_cast<float>(window.getSize().x);
    const float winH = static_cast<float>(window.getSize().y);

    m_overlay.setSize({ winW, winH });
    m_overlay.setPosition(0.f, 0.f);

    const float panelW = 280.f;
    const float itemH  = 44.f;
    const float gap    = 12.f;
    const float panelH = 70.f + ITEM_COUNT * (itemH + gap);
    m_panel.setSize({ panelW, panelH });
    float px = (winW - panelW) * 0.5f;
    float py = (winH - panelH) * 0.5f;
    m_panel.setPosition(px, py);

    if (m_fontLoaded)
    {
        const sf::FloatRect tb = m_titleText.getLocalBounds();
        m_titleText.setPosition(px + (panelW - tb.width) * 0.5f, py + 14.f);
    }

    float startY = py + 60.f;
    for (int i = 0; i < ITEM_COUNT; ++i)
    {
        float ix = px + (panelW - 220.f) * 0.5f;
        float iy = startY + static_cast<float>(i) * (itemH + gap);
        m_items[i].box.setPosition(ix, iy);

        bool sel = (i == m_selectedIndex);
        m_items[i].box.setFillColor(sel ? sf::Color(50, 70, 120) : sf::Color(40, 40, 60));
        m_items[i].box.setOutlineColor(sel ? sf::Color(120, 180, 255) : sf::Color(70, 70, 90));

        if (m_fontLoaded)
        {
            m_items[i].label.setFillColor(sel ? sf::Color::White : sf::Color(180, 180, 200));
            const sf::FloatRect lb = m_items[i].label.getLocalBounds();
            m_items[i].label.setPosition(
                ix + (220.f - lb.width) * 0.5f,
                iy + (itemH - lb.height) * 0.5f - 4.f);
        }
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
        for (int i = 0; i < ITEM_COUNT; ++i)
        {
            window.draw(m_items[i].box);
            window.draw(m_items[i].label);
        }
    }

    window.setView(prev);
}
