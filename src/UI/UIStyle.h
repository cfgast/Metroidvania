#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include "RoundedRectangleShape.h"

// Centralized color palette and drawing helpers for a modern dark-themed UI.
namespace UIStyle
{
    // --- Color palette ---

    // Backgrounds
    inline sf::Color overlayColor()      { return {0, 0, 0, 150}; }
    inline sf::Color panelBg()           { return {16, 18, 30, 245}; }
    inline sf::Color panelBorder()       { return {55, 65, 100}; }

    // Menu items – normal
    inline sf::Color itemBg()            { return {26, 30, 48}; }
    inline sf::Color itemBorder()        { return {45, 50, 72}; }
    inline sf::Color itemTextColor()     { return {170, 175, 200}; }

    // Menu items – selected / hovered
    inline sf::Color itemBgSelected()    { return {35, 55, 105}; }
    inline sf::Color itemBorderSel()     { return {90, 150, 255}; }
    inline sf::Color itemTextSelected()  { return sf::Color::White; }

    // Accent bar on selected items (left edge highlight)
    inline sf::Color accentColor()       { return {90, 150, 255}; }

    // Title text
    inline sf::Color titleColor()        { return {210, 220, 255}; }

    // Subtitle / section header
    inline sf::Color sectionColor()      { return {120, 145, 210}; }

    // Instruction / hint text
    inline sf::Color hintColor()         { return {120, 125, 155}; }

    // Rebinding highlight
    inline sf::Color rebindBg()          { return {75, 40, 45}; }
    inline sf::Color rebindBorder()      { return {255, 160, 70}; }
    inline sf::Color rebindText()        { return {255, 195, 80}; }

    // Glow behind selected items
    inline sf::Color glowColor()         { return {70, 130, 255, 35}; }

    // --- Rounded-rect dimensions ---
    constexpr float CORNER_RADIUS       = 8.f;
    constexpr float PANEL_CORNER_RADIUS = 12.f;

    // --- Drawing helpers ---

    // Draw a soft glow rectangle behind a selected item for depth.
    inline void drawGlow(sf::RenderWindow& window, float x, float y, float w, float h)
    {
        RoundedRectangleShape glow({w + 8.f, h + 8.f}, CORNER_RADIUS + 2.f);
        glow.setPosition(x - 4.f, y - 4.f);
        glow.setFillColor(glowColor());
        glow.setOutlineThickness(0.f);
        window.draw(glow);
    }

    // Draw a thin left-side accent bar on a selected item.
    inline void drawAccentBar(sf::RenderWindow& window, float x, float y, float h)
    {
        constexpr float BAR_W = 4.f;
        RoundedRectangleShape bar({BAR_W, h - 8.f}, 2.f, 4);
        bar.setPosition(x + 4.f, y + 4.f);
        bar.setFillColor(accentColor());
        bar.setOutlineThickness(0.f);
        window.draw(bar);
    }

    // Draw a standard menu item (rounded rect + centered text).
    // Returns the global bounds of the background box.
    inline sf::FloatRect drawMenuItem(sf::RenderWindow& window,
                                       RoundedRectangleShape& box,
                                       sf::Text& label,
                                       float x, float y, float w, float h,
                                       bool selected)
    {
        box.setParameters({w, h}, CORNER_RADIUS);
        box.setPosition(x, y);
        box.setFillColor(selected ? itemBgSelected() : itemBg());
        box.setOutlineColor(selected ? itemBorderSel() : itemBorder());
        box.setOutlineThickness(1.f);

        if (selected)
        {
            drawGlow(window, x, y, w, h);
        }

        window.draw(box);

        if (selected)
        {
            drawAccentBar(window, x, y, h);
        }

        label.setFillColor(selected ? itemTextSelected() : itemTextColor());
        const sf::FloatRect lb = label.getLocalBounds();
        label.setPosition(x + (w - lb.width) * 0.5f,
                          y + (h - lb.height) * 0.5f - 3.f);
        window.draw(label);

        return box.getGlobalBounds();
    }

    // Draw a menu item with left-aligned text (for key-value rows).
    inline sf::FloatRect drawMenuRow(sf::RenderWindow& window,
                                      RoundedRectangleShape& box,
                                      sf::Text& leftLabel,
                                      sf::Text* rightLabel,
                                      float x, float y, float w, float h,
                                      bool selected, bool rebinding = false)
    {
        box.setParameters({w, h}, CORNER_RADIUS);
        box.setPosition(x, y);

        sf::Color bg, border;
        if (rebinding)
        {
            bg = rebindBg();
            border = rebindBorder();
        }
        else if (selected)
        {
            bg = itemBgSelected();
            border = itemBorderSel();
        }
        else
        {
            bg = itemBg();
            border = itemBorder();
        }

        box.setFillColor(bg);
        box.setOutlineColor(border);
        box.setOutlineThickness(1.f);

        if (selected && !rebinding)
            drawGlow(window, x, y, w, h);

        window.draw(box);

        if (selected && !rebinding)
            drawAccentBar(window, x, y, h);

        sf::Color textCol = rebinding ? rebindText()
                          : selected  ? itemTextSelected()
                          : itemTextColor();

        leftLabel.setFillColor(textCol);
        const sf::FloatRect ll = leftLabel.getLocalBounds();
        leftLabel.setPosition(x + 20.f, y + (h - ll.height) * 0.5f - 3.f);
        window.draw(leftLabel);

        if (rightLabel)
        {
            rightLabel->setFillColor(rebinding ? rebindText() : textCol);
            const sf::FloatRect rl = rightLabel->getLocalBounds();
            rightLabel->setPosition(x + w - rl.width - 20.f,
                                    y + (h - rl.height) * 0.5f - 3.f);
            window.draw(*rightLabel);
        }

        return box.getGlobalBounds();
    }
}
