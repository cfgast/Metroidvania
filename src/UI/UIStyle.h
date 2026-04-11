#pragma once

#include "../Rendering/Renderer.h"

// Simple bounds struct returned by menu drawing helpers.
struct UIBounds { float x, y, w, h; };

// Centralized color palette and drawing helpers for a modern dark-themed UI.
namespace UIStyle
{
    // --- Color palette (float RGBA, 0–1 range) ---

    // Backgrounds
    inline void overlayColor(float& r, float& g, float& b, float& a)       { r = 0/255.f; g = 0/255.f; b = 0/255.f; a = 150/255.f; }
    inline void panelBg(float& r, float& g, float& b, float& a)            { r = 16/255.f; g = 18/255.f; b = 30/255.f; a = 245/255.f; }
    inline void panelBorder(float& r, float& g, float& b, float& a)        { r = 55/255.f; g = 65/255.f; b = 100/255.f; a = 1.f; }

    // Menu items – normal
    inline void itemBg(float& r, float& g, float& b, float& a)             { r = 26/255.f; g = 30/255.f; b = 48/255.f; a = 1.f; }
    inline void itemBorder(float& r, float& g, float& b, float& a)         { r = 45/255.f; g = 50/255.f; b = 72/255.f; a = 1.f; }
    inline void itemTextColor(float& r, float& g, float& b, float& a)      { r = 170/255.f; g = 175/255.f; b = 200/255.f; a = 1.f; }

    // Menu items – selected / hovered
    inline void itemBgSelected(float& r, float& g, float& b, float& a)     { r = 35/255.f; g = 55/255.f; b = 105/255.f; a = 1.f; }
    inline void itemBorderSel(float& r, float& g, float& b, float& a)      { r = 90/255.f; g = 150/255.f; b = 255/255.f; a = 1.f; }
    inline void itemTextSelected(float& r, float& g, float& b, float& a)   { r = 1.f; g = 1.f; b = 1.f; a = 1.f; }

    // Accent bar on selected items (left edge highlight)
    inline void accentColor(float& r, float& g, float& b, float& a)        { r = 90/255.f; g = 150/255.f; b = 255/255.f; a = 1.f; }

    // Title text
    inline void titleColor(float& r, float& g, float& b, float& a)         { r = 210/255.f; g = 220/255.f; b = 255/255.f; a = 1.f; }

    // Subtitle / section header
    inline void sectionColor(float& r, float& g, float& b, float& a)       { r = 120/255.f; g = 145/255.f; b = 210/255.f; a = 1.f; }

    // Instruction / hint text
    inline void hintColor(float& r, float& g, float& b, float& a)          { r = 120/255.f; g = 125/255.f; b = 155/255.f; a = 1.f; }

    // Rebinding highlight
    inline void rebindBg(float& r, float& g, float& b, float& a)           { r = 75/255.f; g = 40/255.f; b = 45/255.f; a = 1.f; }
    inline void rebindBorder(float& r, float& g, float& b, float& a)       { r = 255/255.f; g = 160/255.f; b = 70/255.f; a = 1.f; }
    inline void rebindText(float& r, float& g, float& b, float& a)         { r = 255/255.f; g = 195/255.f; b = 80/255.f; a = 1.f; }

    // Glow behind selected items
    inline void glowColor(float& r, float& g, float& b, float& a)          { r = 70/255.f; g = 130/255.f; b = 255/255.f; a = 35/255.f; }

    // --- Rounded-rect dimensions ---
    constexpr float CORNER_RADIUS       = 8.f;
    constexpr float PANEL_CORNER_RADIUS = 12.f;

    // --- Drawing helpers ---

    // Draw a soft glow rectangle behind a selected item for depth.
    inline void drawGlow(Renderer& renderer, float x, float y, float w, float h)
    {
        float r, g, b, a;
        glowColor(r, g, b, a);
        renderer.drawRoundedRect(x - 4.f, y - 4.f, w + 8.f, h + 8.f,
                                 CORNER_RADIUS + 2.f, r, g, b, a);
    }

    // Draw a thin left-side accent bar on a selected item.
    inline void drawAccentBar(Renderer& renderer, float x, float y, float h)
    {
        constexpr float BAR_W = 4.f;
        float r, g, b, a;
        accentColor(r, g, b, a);
        renderer.drawRoundedRect(x + 4.f, y + 4.f, BAR_W, h - 8.f,
                                 2.f, r, g, b, a);
    }

    // Draw a standard menu item (rounded rect + centered text).
    // Returns the bounds of the background box.
    inline UIBounds drawMenuItem(Renderer& renderer,
                                 Renderer::FontHandle font,
                                 const std::string& labelStr,
                                 float x, float y, float w, float h,
                                 unsigned int fontSize,
                                 bool selected)
    {
        float bgR, bgG, bgB, bgA, brR, brG, brB, brA;
        if (selected)
        {
            itemBgSelected(bgR, bgG, bgB, bgA);
            itemBorderSel(brR, brG, brB, brA);
        }
        else
        {
            itemBg(bgR, bgG, bgB, bgA);
            itemBorder(brR, brG, brB, brA);
        }

        if (selected)
            drawGlow(renderer, x, y, w, h);

        renderer.drawRoundedRect(x, y, w, h, CORNER_RADIUS,
                                 bgR, bgG, bgB, bgA,
                                 brR, brG, brB, brA, 1.f);

        if (selected)
            drawAccentBar(renderer, x, y, h);

        float tR, tG, tB, tA;
        if (selected) itemTextSelected(tR, tG, tB, tA);
        else          itemTextColor(tR, tG, tB, tA);

        float tw, th;
        renderer.measureText(font, labelStr, fontSize, tw, th);
        renderer.drawText(font, labelStr,
                          x + (w - tw) * 0.5f,
                          y + (h - th) * 0.5f - 3.f,
                          fontSize, tR, tG, tB, tA);

        return { x, y, w, h };
    }

    // Draw a menu item with left-aligned text (for key-value rows).
    inline UIBounds drawMenuRow(Renderer& renderer,
                                Renderer::FontHandle font,
                                const std::string& leftStr,
                                const std::string* rightStr,
                                float x, float y, float w, float h,
                                unsigned int fontSize,
                                bool selected, bool rebinding = false)
    {
        float bgR, bgG, bgB, bgA, brR, brG, brB, brA;
        if (rebinding)
        {
            rebindBg(bgR, bgG, bgB, bgA);
            rebindBorder(brR, brG, brB, brA);
        }
        else if (selected)
        {
            itemBgSelected(bgR, bgG, bgB, bgA);
            itemBorderSel(brR, brG, brB, brA);
        }
        else
        {
            itemBg(bgR, bgG, bgB, bgA);
            itemBorder(brR, brG, brB, brA);
        }

        if (selected && !rebinding)
            drawGlow(renderer, x, y, w, h);

        renderer.drawRoundedRect(x, y, w, h, CORNER_RADIUS,
                                 bgR, bgG, bgB, bgA,
                                 brR, brG, brB, brA, 1.f);

        if (selected && !rebinding)
            drawAccentBar(renderer, x, y, h);

        float tR, tG, tB, tA;
        if (rebinding)    rebindText(tR, tG, tB, tA);
        else if (selected) itemTextSelected(tR, tG, tB, tA);
        else               itemTextColor(tR, tG, tB, tA);

        float lw, lh;
        renderer.measureText(font, leftStr, fontSize, lw, lh);
        renderer.drawText(font, leftStr,
                          x + 20.f,
                          y + (h - lh) * 0.5f - 3.f,
                          fontSize, tR, tG, tB, tA);

        if (rightStr && !rightStr->empty())
        {
            float rTR, rTG, rTB, rTA;
            if (rebinding) rebindText(rTR, rTG, rTB, rTA);
            else           { rTR = tR; rTG = tG; rTB = tB; rTA = tA; }

            float rw, rh;
            renderer.measureText(font, *rightStr, fontSize, rw, rh);
            renderer.drawText(font, *rightStr,
                              x + w - rw - 20.f,
                              y + (h - rh) * 0.5f - 3.f,
                              fontSize, rTR, rTG, rTB, rTA);
        }

        return { x, y, w, h };
    }
}
