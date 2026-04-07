#include "SaveSlotScreen.h"
#include "UIStyle.h"

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
        m_titleText.setCharacterSize(30);
        m_titleText.setFillColor(UIStyle::titleColor());

        m_instructionText.setFont(m_font);
        m_instructionText.setString("Up/Down select  |  Enter load/start  |  Del erase  |  Left/Right change resolution  |  Controls: rebind keys");
        m_instructionText.setCharacterSize(14);
        m_instructionText.setFillColor(UIStyle::hintColor());

        m_resWidget.box.setParameters({ 440.f, 52.f }, UIStyle::CORNER_RADIUS);
        m_resWidget.box.setOutlineThickness(1.f);
        m_resWidget.label.setFont(m_font);
        m_resWidget.label.setCharacterSize(18);

        m_controlsWidget.box.setParameters({ 440.f, 52.f }, UIStyle::CORNER_RADIUS);
        m_controlsWidget.box.setOutlineThickness(1.f);
        m_controlsWidget.label.setFont(m_font);
        m_controlsWidget.label.setString("Controls");
        m_controlsWidget.label.setCharacterSize(18);
    }

    m_background.setFillColor(UIStyle::panelBg());

    populateResolutions();
    updateResolutionLabel();
}

void SaveSlotScreen::populateResolutions()
{
    m_resolutions.clear();

    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();

    static const Resolution COMMON[] = {
        {800, 600}, {1024, 768}, {1280, 720}, {1280, 800},
        {1366, 768}, {1440, 900}, {1600, 900}, {1680, 1050},
        {1920, 1080}, {2560, 1440}, {3840, 2160}
    };

    for (const auto& r : COMMON)
    {
        if (r.width <= desktop.width && r.height <= desktop.height)
            m_resolutions.push_back(r);
    }

    if (m_resolutions.empty())
        m_resolutions.push_back({800, 600});

    // Default to 800x600 (matching initial window size)
    m_resolutionIndex = 0;
    for (int i = 0; i < static_cast<int>(m_resolutions.size()); ++i)
    {
        if (m_resolutions[i].width == 800 && m_resolutions[i].height == 600)
        {
            m_resolutionIndex = i;
            break;
        }
    }
}

void SaveSlotScreen::updateResolutionLabel()
{
    if (!m_fontLoaded || m_resolutions.empty())
        return;

    const auto& res = m_resolutions[m_resolutionIndex];
    std::string text = "Resolution:   <  "
                     + std::to_string(res.width) + " x " + std::to_string(res.height)
                     + "  >";
    m_resWidget.label.setString(text);
}

void SaveSlotScreen::applyResolution(sf::RenderWindow& window)
{
    const auto& res = m_resolutions[m_resolutionIndex];
    window.setSize(sf::Vector2u(res.width, res.height));

    // Center the window on screen
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    int x = static_cast<int>((desktop.width  - res.width)  / 2);
    int y = static_cast<int>((desktop.height - res.height) / 2);
    window.setPosition(sf::Vector2i(x, y));

    updateResolutionLabel();
}

int SaveSlotScreen::totalItemCount() const
{
    return static_cast<int>(m_slots.size()) + 2; // +1 resolution, +1 controls
}

bool SaveSlotScreen::isOnResolutionRow() const
{
    return m_selectedIndex == static_cast<int>(m_slots.size());
}

bool SaveSlotScreen::isOnControlsRow() const
{
    return m_selectedIndex == static_cast<int>(m_slots.size()) + 1;
}

void SaveSlotScreen::open()
{
    m_open = true;
    m_selectedIndex = 0;
    m_joyUpHeld    = false;
    m_joyDownHeld  = false;
    m_joyLeftHeld  = false;
    m_joyRightHeld = false;
    refreshSlots();
}

void SaveSlotScreen::refreshSlots()
{
    m_slots = SaveSystem::querySlotsInfo();
    m_widgets.resize(m_slots.size());

    for (size_t i = 0; i < m_slots.size(); ++i)
    {
        auto& w = m_widgets[i];
        w.box.setParameters({ 440.f, 62.f }, UIStyle::CORNER_RADIUS);
        w.box.setOutlineThickness(1.f);

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
        m_titleText.setPosition((winW - tb.width) * 0.5f, 70.f);
    }

    const float slotW   = 440.f;
    const float slotH   = 62.f;
    const float resH    = 52.f;
    const float ctrlH   = 52.f;
    const float gap     = 16.f;
    const float totalH  = static_cast<float>(m_widgets.size()) * (slotH + gap) + resH + gap + ctrlH;
    float startY        = (winH - totalH) * 0.5f + 20.f;

    for (size_t i = 0; i < m_widgets.size(); ++i)
    {
        auto& w = m_widgets[i];
        float x = (winW - slotW) * 0.5f;
        float y = startY + static_cast<float>(i) * (slotH + gap);
        w.box.setParameters({ slotW, slotH }, UIStyle::CORNER_RADIUS);
        w.box.setPosition(x, y);
    }

    // Resolution combo box
    {
        float x = (winW - slotW) * 0.5f;
        float y = startY + static_cast<float>(m_widgets.size()) * (slotH + gap);
        m_resWidget.box.setParameters({ slotW, resH }, UIStyle::CORNER_RADIUS);
        m_resWidget.box.setPosition(x, y);
    }

    // Controls button
    {
        float x = (winW - slotW) * 0.5f;
        float y = startY + static_cast<float>(m_widgets.size()) * (slotH + gap) + resH + gap;
        m_controlsWidget.box.setParameters({ slotW, ctrlH }, UIStyle::CORNER_RADIUS);
        m_controlsWidget.box.setPosition(x, y);
    }

    if (m_fontLoaded)
    {
        const sf::FloatRect ib = m_instructionText.getLocalBounds();
        m_instructionText.setPosition((winW - ib.width) * 0.5f, winH - 60.f);
    }
}

SaveSlotResult SaveSlotScreen::handleEvent(const sf::Event& event,
                                           sf::RenderWindow& window)
{
    SaveSlotResult result;
    if (!m_open)
        return result;

    if (event.type == sf::Event::KeyPressed)
    {
        const int total = totalItemCount();

        if (event.key.code == sf::Keyboard::Up)
        {
            m_selectedIndex = (m_selectedIndex - 1 + total) % total;
        }
        else if (event.key.code == sf::Keyboard::Down)
        {
            m_selectedIndex = (m_selectedIndex + 1) % total;
        }
        else if (event.key.code == sf::Keyboard::Left && isOnResolutionRow())
        {
            m_resolutionIndex = (m_resolutionIndex - 1 + static_cast<int>(m_resolutions.size()))
                                % static_cast<int>(m_resolutions.size());
            applyResolution(window);
        }
        else if (event.key.code == sf::Keyboard::Right && isOnResolutionRow())
        {
            m_resolutionIndex = (m_resolutionIndex + 1)
                                % static_cast<int>(m_resolutions.size());
            applyResolution(window);
        }
        else if (event.key.code == sf::Keyboard::Return && !isOnResolutionRow())
        {
            if (isOnControlsRow())
            {
                result.action = SaveSlotResult::Controls;
            }
            else
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
        }
        else if (event.key.code == sf::Keyboard::Delete && !isOnResolutionRow() && !isOnControlsRow())
        {
            int slot = m_slots[m_selectedIndex].slot;
            SaveSystem::deleteSlot(slot);
            refreshSlots();
        }
    }

    // Mouse hover – highlight item under cursor
    if (event.type == sf::Event::MouseMoved)
    {
        sf::Vector2f mouse(static_cast<float>(event.mouseMove.x),
                           static_cast<float>(event.mouseMove.y));
        for (int i = 0; i < static_cast<int>(m_widgets.size()); ++i)
        {
            if (m_widgets[i].box.getGlobalBounds().contains(mouse))
            {
                m_selectedIndex = i;
                break;
            }
        }
        if (m_resWidget.box.getGlobalBounds().contains(mouse))
            m_selectedIndex = static_cast<int>(m_slots.size());
        if (m_controlsWidget.box.getGlobalBounds().contains(mouse))
            m_selectedIndex = static_cast<int>(m_slots.size()) + 1;
    }

    // Mouse click – activate item
    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left)
    {
        sf::Vector2f mouse(static_cast<float>(event.mouseButton.x),
                           static_cast<float>(event.mouseButton.y));

        // Check slot widgets
        for (int i = 0; i < static_cast<int>(m_widgets.size()); ++i)
        {
            if (m_widgets[i].box.getGlobalBounds().contains(mouse))
            {
                m_selectedIndex = i;
                int slot = m_slots[i].slot;
                if (m_slots[i].exists)
                {
                    result.action = SaveSlotResult::LoadSlot;
                    result.slot   = slot;
                }
                else
                {
                    result.action = SaveSlotResult::NewGame;
                    result.slot   = slot;
                }
                return result;
            }
        }

        // Check resolution widget – left third decreases, right third increases
        if (m_resWidget.box.getGlobalBounds().contains(mouse))
        {
            m_selectedIndex = static_cast<int>(m_slots.size());
            sf::FloatRect bounds = m_resWidget.box.getGlobalBounds();
            float third = bounds.width / 3.f;
            if (mouse.x < bounds.left + third)
            {
                m_resolutionIndex = (m_resolutionIndex - 1 + static_cast<int>(m_resolutions.size()))
                                    % static_cast<int>(m_resolutions.size());
                applyResolution(window);
            }
            else if (mouse.x > bounds.left + 2.f * third)
            {
                m_resolutionIndex = (m_resolutionIndex + 1)
                                    % static_cast<int>(m_resolutions.size());
                applyResolution(window);
            }
        }

        // Check controls widget
        if (m_controlsWidget.box.getGlobalBounds().contains(mouse))
        {
            m_selectedIndex = static_cast<int>(m_slots.size()) + 1;
            result.action = SaveSlotResult::Controls;
            return result;
        }
    }

    // Controller: A button = confirm, X button = delete slot
    if (event.type == sf::Event::JoystickButtonPressed)
    {
        unsigned int btn = event.joystickButton.button;
        if (btn == 0 && !isOnResolutionRow()) // A button - select/load
        {
            if (isOnControlsRow())
            {
                result.action = SaveSlotResult::Controls;
            }
            else
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
        }
        else if (btn == 2 && !isOnResolutionRow() && !isOnControlsRow()) // X button - delete
        {
            int slot = m_slots[m_selectedIndex].slot;
            SaveSystem::deleteSlot(slot);
            refreshSlots();
        }
    }

    // Controller: D-pad / left stick for navigation
    if (event.type == sf::Event::JoystickMoved)
    {
        constexpr float threshold = 50.f;
        float pos = event.joystickMove.position;
        const int total = totalItemCount();

        bool isStickY = (event.joystickMove.axis == sf::Joystick::Y);
        bool isPovY   = (event.joystickMove.axis == sf::Joystick::PovY);
        bool isStickX = (event.joystickMove.axis == sf::Joystick::X);
        bool isPovX   = (event.joystickMove.axis == sf::Joystick::PovX);

        if (isStickY || isPovY)
        {
            // Stick Y: negative=up, positive=down. PovY: positive=up, negative=down.
            bool up   = isStickY ? (pos < -threshold) : (pos > threshold);
            bool down = isStickY ? (pos > threshold)  : (pos < -threshold);

            if (up && !m_joyUpHeld)
            {
                m_selectedIndex = (m_selectedIndex - 1 + total) % total;
                m_joyUpHeld = true;
            }
            else if (!up)
            {
                m_joyUpHeld = false;
            }

            if (down && !m_joyDownHeld)
            {
                m_selectedIndex = (m_selectedIndex + 1) % total;
                m_joyDownHeld = true;
            }
            else if (!down)
            {
                m_joyDownHeld = false;
            }
        }

        if ((isStickX || isPovX) && isOnResolutionRow())
        {
            bool left  = (pos < -threshold);
            bool right = (pos > threshold);

            if (left && !m_joyLeftHeld)
            {
                m_resolutionIndex = (m_resolutionIndex - 1 + static_cast<int>(m_resolutions.size()))
                                    % static_cast<int>(m_resolutions.size());
                applyResolution(window);
                m_joyLeftHeld = true;
            }
            else if (!left)
            {
                m_joyLeftHeld = false;
            }

            if (right && !m_joyRightHeld)
            {
                m_resolutionIndex = (m_resolutionIndex + 1)
                                    % static_cast<int>(m_resolutions.size());
                applyResolution(window);
                m_joyRightHeld = true;
            }
            else if (!right)
            {
                m_joyRightHeld = false;
            }
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
    sf::View uiView(sf::FloatRect(0.f, 0.f,
                                   static_cast<float>(window.getSize().x),
                                   static_cast<float>(window.getSize().y)));
    window.setView(uiView);

    window.draw(m_background);

    if (m_fontLoaded)
        window.draw(m_titleText);

    const float slotW = 440.f;
    const float slotH = 62.f;
    const float resH  = 52.f;
    const float ctrlH = 52.f;

    for (int i = 0; i < static_cast<int>(m_widgets.size()); ++i)
    {
        auto& w = m_widgets[i];
        bool selected = (i == m_selectedIndex);
        sf::Vector2f pos = w.box.getPosition();
        UIStyle::drawMenuRow(window, w.box, w.label, nullptr,
                             pos.x, pos.y, slotW, slotH, selected);
    }

    // Draw resolution combo box
    {
        bool selected = isOnResolutionRow();
        sf::Vector2f pos = m_resWidget.box.getPosition();
        UIStyle::drawMenuRow(window, m_resWidget.box, m_resWidget.label, nullptr,
                             pos.x, pos.y, slotW, resH, selected);
    }

    // Draw controls button
    {
        bool selected = isOnControlsRow();
        sf::Vector2f pos = m_controlsWidget.box.getPosition();
        UIStyle::drawMenuItem(window, m_controlsWidget.box, m_controlsWidget.label,
                              pos.x, pos.y, slotW, ctrlH, selected);
    }

    if (m_fontLoaded)
        window.draw(m_instructionText);

    window.setView(prev);
}
