#include "SaveSlotScreen.h"
#include "UIStyle.h"

#include <sstream>
#include <iomanip>

#include <SFML/Graphics/RenderWindow.hpp>

SaveSlotScreen::SaveSlotScreen()
{
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
    if (m_resolutions.empty())
        return;

    const auto& res = m_resolutions[m_resolutionIndex];
    m_resLabel = "Resolution:   <  "
               + std::to_string(res.width) + " x " + std::to_string(res.height)
               + "  >";
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
    m_slotLabels.resize(m_slots.size());

    for (size_t i = 0; i < m_slots.size(); ++i)
    {
        if (m_slots[i].exists)
        {
            std::ostringstream oss;
            oss << "Slot " << m_slots[i].slot << "  |  "
                << m_slots[i].mapFile << "  |  HP: "
                << std::fixed << std::setprecision(0) << m_slots[i].hp;
            m_slotLabels[i] = oss.str();
        }
        else
        {
            m_slotLabels[i] = "Slot " + std::to_string(m_slots[i].slot) + "  -  Empty (New Game)";
        }
    }
}

void SaveSlotScreen::layout(Renderer& renderer)
{
    float winW, winH;
    renderer.getWindowSize(winW, winH);

    const float slotW   = 440.f;
    const float slotH   = 62.f;
    const float resH    = 52.f;
    const float ctrlH   = 52.f;
    const float gap     = 16.f;
    const float totalH  = static_cast<float>(m_slots.size()) * (slotH + gap) + resH + gap + ctrlH;
    float startY        = (winH - totalH) * 0.5f + 20.f;

    m_slotLayouts.resize(m_slots.size());
    for (size_t i = 0; i < m_slots.size(); ++i)
    {
        float x = (winW - slotW) * 0.5f;
        float y = startY + static_cast<float>(i) * (slotH + gap);
        m_slotLayouts[i] = { x, y, slotW, slotH };
    }

    // Resolution combo box
    {
        float x = (winW - slotW) * 0.5f;
        float y = startY + static_cast<float>(m_slots.size()) * (slotH + gap);
        m_resLayout = { x, y, slotW, resH };
    }

    // Controls button
    {
        float x = (winW - slotW) * 0.5f;
        float y = startY + static_cast<float>(m_slots.size()) * (slotH + gap) + resH + gap;
        m_controlsLayout = { x, y, slotW, ctrlH };
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
        float mx = static_cast<float>(event.mouseMove.x);
        float my = static_cast<float>(event.mouseMove.y);
        for (int i = 0; i < static_cast<int>(m_slotLayouts.size()); ++i)
        {
            auto& il = m_slotLayouts[i];
            if (mx >= il.x && mx <= il.x + il.w &&
                my >= il.y && my <= il.y + il.h)
            {
                m_selectedIndex = i;
                break;
            }
        }
        if (mx >= m_resLayout.x && mx <= m_resLayout.x + m_resLayout.w &&
            my >= m_resLayout.y && my <= m_resLayout.y + m_resLayout.h)
            m_selectedIndex = static_cast<int>(m_slots.size());
        if (mx >= m_controlsLayout.x && mx <= m_controlsLayout.x + m_controlsLayout.w &&
            my >= m_controlsLayout.y && my <= m_controlsLayout.y + m_controlsLayout.h)
            m_selectedIndex = static_cast<int>(m_slots.size()) + 1;
    }

    // Mouse click – activate item
    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left)
    {
        float mx = static_cast<float>(event.mouseButton.x);
        float my = static_cast<float>(event.mouseButton.y);

        // Check slot widgets
        for (int i = 0; i < static_cast<int>(m_slotLayouts.size()); ++i)
        {
            auto& il = m_slotLayouts[i];
            if (mx >= il.x && mx <= il.x + il.w &&
                my >= il.y && my <= il.y + il.h)
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
        if (mx >= m_resLayout.x && mx <= m_resLayout.x + m_resLayout.w &&
            my >= m_resLayout.y && my <= m_resLayout.y + m_resLayout.h)
        {
            m_selectedIndex = static_cast<int>(m_slots.size());
            float third = m_resLayout.w / 3.f;
            if (mx < m_resLayout.x + third)
            {
                m_resolutionIndex = (m_resolutionIndex - 1 + static_cast<int>(m_resolutions.size()))
                                    % static_cast<int>(m_resolutions.size());
                applyResolution(window);
            }
            else if (mx > m_resLayout.x + 2.f * third)
            {
                m_resolutionIndex = (m_resolutionIndex + 1)
                                    % static_cast<int>(m_resolutions.size());
                applyResolution(window);
            }
        }

        // Check controls widget
        if (mx >= m_controlsLayout.x && mx <= m_controlsLayout.x + m_controlsLayout.w &&
            my >= m_controlsLayout.y && my <= m_controlsLayout.y + m_controlsLayout.h)
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

void SaveSlotScreen::render(Renderer& renderer)
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

    // Title
    if (m_font)
    {
        float tR, tG, tB, tA;
        UIStyle::titleColor(tR, tG, tB, tA);
        float tw, th;
        renderer.measureText(m_font, "Select Save Slot", 30, tw, th);
        renderer.drawText(m_font, "Select Save Slot",
                          (winW - tw) * 0.5f, 70.f,
                          30, tR, tG, tB, tA);
    }

    const float slotW = 440.f;
    const float slotH = 62.f;
    const float resH  = 52.f;
    const float ctrlH = 52.f;

    for (int i = 0; i < static_cast<int>(m_slotLayouts.size()); ++i)
    {
        bool selected = (i == m_selectedIndex);
        auto& il = m_slotLayouts[i];
        UIStyle::drawMenuRow(renderer, m_font, m_slotLabels[i], nullptr,
                             il.x, il.y, slotW, slotH, 18, selected);
    }

    // Draw resolution combo box
    {
        bool selected = isOnResolutionRow();
        UIStyle::drawMenuRow(renderer, m_font, m_resLabel, nullptr,
                             m_resLayout.x, m_resLayout.y, slotW, resH, 18, selected);
    }

    // Draw controls button
    {
        bool selected = isOnControlsRow();
        std::string controlsStr = "Controls";
        UIStyle::drawMenuItem(renderer, m_font, controlsStr,
                              m_controlsLayout.x, m_controlsLayout.y, slotW, ctrlH,
                              18, selected);
    }

    // Instruction text
    if (m_font)
    {
        float hR, hG, hB, hA;
        UIStyle::hintColor(hR, hG, hB, hA);
        std::string hint = "Up/Down select  |  Enter load/start  |  Del erase  |  Left/Right change resolution  |  Controls: rebind keys";
        float hw, hh;
        renderer.measureText(m_font, hint, 14, hw, hh);
        renderer.drawText(m_font, hint,
                          (winW - hw) * 0.5f, winH - 60.f,
                          14, hR, hG, hB, hA);
    }
}
