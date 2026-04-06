#include "SaveSlotScreen.h"

#include <sstream>
#include <iomanip>

#include <SFML/Graphics/RenderWindow.hpp>

SaveSlotScreen::SaveSlotScreen()
{
    m_fontLoaded = m_font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf");

    if (m_fontLoaded)
    {
        m_titleText.setFont(m_font);
        m_titleText.setString("Select Save Slot");
        m_titleText.setCharacterSize(28);
        m_titleText.setFillColor(sf::Color(220, 220, 255));

        m_instructionText.setFont(m_font);
        m_instructionText.setString("Up/Down to select  |  Enter to load/start  |  Delete to erase");
        m_instructionText.setCharacterSize(14);
        m_instructionText.setFillColor(sf::Color(160, 160, 180));
    }

    m_background.setFillColor(sf::Color(20, 20, 35, 240));
}

void SaveSlotScreen::open()
{
    m_open = true;
    m_selectedIndex = 0;
    refreshSlots();
}

void SaveSlotScreen::refreshSlots()
{
    m_slots = SaveSystem::querySlotsInfo();
    m_widgets.resize(m_slots.size());

    for (size_t i = 0; i < m_slots.size(); ++i)
    {
        auto& w = m_widgets[i];
        w.box.setSize({ 400.f, 60.f });
        w.box.setOutlineThickness(2.f);

        if (m_fontLoaded)
        {
            w.label.setFont(m_font);
            w.label.setCharacterSize(18);

            if (m_slots[i].exists)
            {
                std::ostringstream oss;
                oss << "Slot " << m_slots[i].slot << "  |  "
                    << m_slots[i].mapFile << "  |  HP: "
                    << std::fixed << std::setprecision(0) << m_slots[i].hp;
                w.label.setString(oss.str());
            }
            else
            {
                w.label.setString("Slot " + std::to_string(m_slots[i].slot) + "  -  Empty (New Game)");
            }
        }
    }
}

void SaveSlotScreen::layout(const sf::RenderWindow& window)
{
    const float winW = static_cast<float>(window.getSize().x);
    const float winH = static_cast<float>(window.getSize().y);

    m_background.setSize({ winW, winH });
    m_background.setPosition(0.f, 0.f);

    if (m_fontLoaded)
    {
        const sf::FloatRect tb = m_titleText.getLocalBounds();
        m_titleText.setPosition((winW - tb.width) * 0.5f, 80.f);
    }

    const float slotW   = 400.f;
    const float slotH   = 60.f;
    const float gap     = 16.f;
    const float totalH  = static_cast<float>(m_widgets.size()) * (slotH + gap) - gap;
    float startY        = (winH - totalH) * 0.5f + 20.f;

    for (size_t i = 0; i < m_widgets.size(); ++i)
    {
        auto& w = m_widgets[i];
        float x = (winW - slotW) * 0.5f;
        float y = startY + static_cast<float>(i) * (slotH + gap);
        w.box.setPosition(x, y);

        bool selected = (static_cast<int>(i) == m_selectedIndex);
        w.box.setFillColor(selected ? sf::Color(50, 70, 120) : sf::Color(35, 35, 55));
        w.box.setOutlineColor(selected ? sf::Color(120, 180, 255) : sf::Color(80, 80, 100));

        if (m_fontLoaded)
        {
            w.label.setFillColor(selected ? sf::Color::White : sf::Color(180, 180, 200));
            const sf::FloatRect lb = w.label.getLocalBounds();
            w.label.setPosition(x + 20.f, y + (slotH - lb.height) * 0.5f - 4.f);
        }
    }

    if (m_fontLoaded)
    {
        const sf::FloatRect ib = m_instructionText.getLocalBounds();
        m_instructionText.setPosition((winW - ib.width) * 0.5f, winH - 60.f);
    }
}

SaveSlotResult SaveSlotScreen::handleEvent(const sf::Event& event,
                                           const sf::RenderWindow& window)
{
    SaveSlotResult result;
    if (!m_open)
        return result;

    if (event.type == sf::Event::KeyPressed)
    {
        if (event.key.code == sf::Keyboard::Up)
        {
            m_selectedIndex = (m_selectedIndex - 1 + static_cast<int>(m_slots.size()))
                              % static_cast<int>(m_slots.size());
        }
        else if (event.key.code == sf::Keyboard::Down)
        {
            m_selectedIndex = (m_selectedIndex + 1) % static_cast<int>(m_slots.size());
        }
        else if (event.key.code == sf::Keyboard::Return)
        {
            int slot = m_slots[m_selectedIndex].slot;
            if (m_slots[m_selectedIndex].exists)
            {
                result.action = SaveSlotResult::LoadSlot;
                result.slot   = slot;
            }
            else
            {
                result.action = SaveSlotResult::NewGame;
                result.slot   = slot;
            }
        }
        else if (event.key.code == sf::Keyboard::Delete)
        {
            int slot = m_slots[m_selectedIndex].slot;
            SaveSystem::deleteSlot(slot);
            refreshSlots();
        }
    }

    return result;
}

void SaveSlotScreen::render(sf::RenderWindow& window)
{
    if (!m_open)
        return;

    layout(window);

    sf::View prev = window.getView();
    window.setView(window.getDefaultView());

    window.draw(m_background);

    if (m_fontLoaded)
        window.draw(m_titleText);

    for (auto& w : m_widgets)
    {
        window.draw(w.box);
        if (m_fontLoaded)
            window.draw(w.label);
    }

    if (m_fontLoaded)
        window.draw(m_instructionText);

    window.setView(prev);
}
