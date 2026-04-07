#include "DebugMenu.h"
#include "../UI/UIStyle.h"

#include <SFML/Graphics/RenderWindow.hpp>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

static constexpr float k_panelW  = 340.f;
static constexpr float k_panelH  = 150.f;
static constexpr float k_btnW    = 220.f;
static constexpr float k_btnH    =  44.f;

DebugMenu::DebugMenu()
{
    m_fontLoaded = m_font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf");

    m_panel.setParameters({ k_panelW, k_panelH }, UIStyle::PANEL_CORNER_RADIUS);
    m_panel.setFillColor(UIStyle::panelBg());
    m_panel.setOutlineColor(UIStyle::panelBorder());
    m_panel.setOutlineThickness(1.5f);

    m_button.setParameters({ k_btnW, k_btnH }, UIStyle::CORNER_RADIUS);
    m_button.setFillColor(UIStyle::itemBgSelected());
    m_button.setOutlineColor(UIStyle::itemBorderSel());
    m_button.setOutlineThickness(1.f);

    if (m_fontLoaded)
    {
        m_titleText.setFont(m_font);
        m_titleText.setString("Debug Menu");
        m_titleText.setCharacterSize(20);
        m_titleText.setFillColor(UIStyle::titleColor());

        m_buttonText.setFont(m_font);
        m_buttonText.setString("Open Map...");
        m_buttonText.setCharacterSize(16);
        m_buttonText.setFillColor(sf::Color::White);
    }
}

// Position panel and children relative to current window size.
void DebugMenu::layout(const sf::RenderWindow& window)
{
    const float winW = static_cast<float>(window.getSize().x);
    const float winH = static_cast<float>(window.getSize().y);

    const float px = (winW - k_panelW) * 0.5f;
    const float py = (winH - k_panelH) * 0.5f;
    m_panel.setPosition(px, py);

    const float bx = px + (k_panelW - k_btnW) * 0.5f;
    const float by = py + k_panelH - k_btnH - 24.f;
    m_button.setPosition(bx, by);

    if (m_fontLoaded)
    {
        const sf::FloatRect tb = m_titleText.getLocalBounds();
        m_titleText.setPosition(px + (k_panelW - tb.width) * 0.5f, py + 16.f);

        // Centre button label inside button
        const sf::FloatRect bb = m_buttonText.getLocalBounds();
        m_buttonText.setOrigin(bb.left + bb.width  * 0.5f,
                               bb.top  + bb.height * 0.5f);
        m_buttonText.setPosition(bx + k_btnW * 0.5f, by + k_btnH * 0.5f);
    }
}

void DebugMenu::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    if (event.type == sf::Event::KeyPressed &&
        event.key.code == sf::Keyboard::F1)
    {
        m_open = !m_open;
        if (m_open)
            layout(window);
        return;
    }

    if (!m_open)
        return;

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left)
    {
        const sf::Vector2f mouse(static_cast<float>(event.mouseButton.x),
                                 static_cast<float>(event.mouseButton.y));
        if (m_button.getGlobalBounds().contains(mouse))
            openFileDialog();
    }
}

std::string DebugMenu::pollSelectedMap()
{
    std::string result;
    std::swap(result, m_pendingMap);
    return result;
}

void DebugMenu::render(sf::RenderWindow& window)
{
    if (!m_open)
        return;

    layout(window);

    window.draw(m_panel);
    window.draw(m_button);

    if (m_fontLoaded)
    {
        window.draw(m_titleText);
        window.draw(m_buttonText);
    }
}

void DebugMenu::openFileDialog()
{
    char filename[MAX_PATH] = {};

    OPENFILENAMEA ofn   = {};
    ofn.lStructSize     = sizeof(ofn);
    ofn.lpstrFilter     = "Map Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile       = filename;
    ofn.nMaxFile        = MAX_PATH;
    ofn.lpstrTitle      = "Select Map File";
    ofn.Flags           = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn))
    {
        m_pendingMap = filename;
        m_open = false;
    }
}
