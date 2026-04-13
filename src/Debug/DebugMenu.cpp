#include "DebugMenu.h"
#include "../UI/UIStyle.h"

#include <cstdio>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

static constexpr float k_panelW  = 340.f;
static constexpr float k_panelH  = 260.f;
static constexpr float k_btnW    = 220.f;
static constexpr float k_btnH    =  44.f;

void DebugMenu::handleEvent(const InputEvent& event)
{
    if (event.type == InputEventType::KeyPressed &&
        event.key == KeyCode::F1)
    {
        m_open = !m_open;
        return;
    }

    if (!m_open)
        return;

    if (event.type == InputEventType::MouseButtonPressed &&
        event.mouseButton == MouseButton::Left)
    {
        float mx = static_cast<float>(event.mouseX);
        float my = static_cast<float>(event.mouseY);
        if (mx >= m_buttonLayout.x && mx <= m_buttonLayout.x + m_buttonLayout.w &&
            my >= m_buttonLayout.y && my <= m_buttonLayout.y + m_buttonLayout.h)
            openFileDialog();
    }
}

std::string DebugMenu::pollSelectedMap()
{
    std::string result;
    std::swap(result, m_pendingMap);
    return result;
}

void DebugMenu::render(Renderer& renderer)
{
    if (!m_open)
        return;

    // Lazy-load font on first render
    if (m_font == 0)
        m_font = renderer.loadFont("C:\\Windows\\Fonts\\arial.ttf");

    // Refresh diagnostics from renderer
    m_gpuFrameTimeMs     = renderer.getGpuFrameTimeMs();
    m_validationErrors   = renderer.getValidationErrorCount();
    m_validationWarnings = renderer.getValidationWarningCount();

    float winW, winH;
    renderer.getWindowSize(winW, winH);

    // Compute layout
    const float px = (winW - k_panelW) * 0.5f;
    const float py = (winH - k_panelH) * 0.5f;
    const float bx = px + (k_panelW - k_btnW) * 0.5f;
    const float by = py + k_panelH - k_btnH - 24.f;
    m_buttonLayout = { bx, by, k_btnW, k_btnH };

    // Panel
    {
        float r, g, b, a, br, bg2, bb, ba;
        UIStyle::panelBg(r, g, b, a);
        UIStyle::panelBorder(br, bg2, bb, ba);
        renderer.drawRoundedRect(px, py, k_panelW, k_panelH,
                                 UIStyle::PANEL_CORNER_RADIUS,
                                 r, g, b, a, br, bg2, bb, ba, 1.5f);
    }

    // Button
    {
        float r, g, b, a, br, bg2, bb, ba;
        UIStyle::itemBgSelected(r, g, b, a);
        UIStyle::itemBorderSel(br, bg2, bb, ba);
        renderer.drawRoundedRect(bx, by, k_btnW, k_btnH,
                                 UIStyle::CORNER_RADIUS,
                                 r, g, b, a, br, bg2, bb, ba, 1.f);
    }

    if (m_font)
    {
        // Title
        {
            float tR, tG, tB, tA;
            UIStyle::titleColor(tR, tG, tB, tA);
            float tw, th;
            renderer.measureText(m_font, "Debug Menu", 20, tw, th);
            renderer.drawText(m_font, "Debug Menu",
                              px + (k_panelW - tw) * 0.5f, py + 16.f,
                              20, tR, tG, tB, tA);
        }

        // GPU device name
        {
            std::string gpuName = renderer.getGpuDeviceName();
            if (!gpuName.empty())
            {
                float tw, th;
                renderer.measureText(m_font, gpuName, 13, tw, th);
                renderer.drawText(m_font, gpuName,
                                  px + (k_panelW - tw) * 0.5f, py + 44.f,
                                  13, 0.7f, 0.7f, 0.7f, 1.f);
            }
        }

        // GPU frame time
        {
            char buf[64];
            snprintf(buf, sizeof(buf), "GPU: %.2f ms", static_cast<double>(m_gpuFrameTimeMs));
            std::string line(buf);
            renderer.drawText(m_font, line,
                              px + 20.f, py + 66.f,
                              14, 0.8f, 1.f, 0.8f, 1.f);
        }

        // Validation errors
        {
            char buf[64];
            snprintf(buf, sizeof(buf), "Errors: %u", m_validationErrors);
            std::string line(buf);
            float eR = (m_validationErrors > 0) ? 1.f : 0.5f;
            float eG = (m_validationErrors > 0) ? 0.3f : 1.f;
            float eB = (m_validationErrors > 0) ? 0.3f : 0.5f;
            renderer.drawText(m_font, line,
                              px + 20.f, py + 86.f,
                              14, eR, eG, eB, 1.f);
        }

        // Validation warnings
        {
            char buf[64];
            snprintf(buf, sizeof(buf), "Warnings: %u", m_validationWarnings);
            std::string line(buf);
            float wR = (m_validationWarnings > 0) ? 1.f : 0.5f;
            float wG = (m_validationWarnings > 0) ? 0.85f : 1.f;
            float wB = (m_validationWarnings > 0) ? 0.3f : 0.5f;
            renderer.drawText(m_font, line,
                              px + 20.f, py + 106.f,
                              14, wR, wG, wB, 1.f);
        }

        // Button text
        {
            float bw, bh;
            renderer.measureText(m_font, "Open Map...", 16, bw, bh);
            renderer.drawText(m_font, "Open Map...",
                              bx + (k_btnW - bw) * 0.5f,
                              by + (k_btnH - bh) * 0.5f - 2.f,
                              16, 1.f, 1.f, 1.f, 1.f);
        }
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
