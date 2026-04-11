#pragma once

#include "Renderer.h"
#include "../Input/SFMLInput.h"

#include <unordered_map>
#include <memory>
#include <string>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Texture.hpp>

// SFML-backed implementation of the Renderer interface.
// Owns the sf::RenderWindow, the SFMLInput, and all loaded GPU resources.
class SFMLRenderer : public Renderer
{
public:
    SFMLRenderer(const std::string& title, unsigned int width,
                 unsigned int height, unsigned int fpsCap = 60);

    // ── Input ─────────────────────────────────────────────────────────
    InputSystem& getInput() override { return m_input; }

    // ── Window operations ─────────────────────────────────────────────
    bool isOpen() const override;
    void close() override;
    void setMouseCursorVisible(bool visible) override;
    void setWindowSize(unsigned int w, unsigned int h) override;
    void setWindowPosition(int x, int y) override;
    void getDesktopSize(unsigned int& w, unsigned int& h) const override;

    // ── Lifecycle ─────────────────────────────────────────────────────
    void clear(float r, float g, float b, float a = 1.f) override;
    void display() override;

    // ── View / camera ─────────────────────────────────────────────────
    void setView(float centerX, float centerY,
                 float width, float height) override;
    void resetView() override;
    void getWindowSize(float& w, float& h) const override;

    // ── Primitives ────────────────────────────────────────────────────
    void drawRect(float x, float y, float w, float h,
                  float r, float g, float b, float a = 1.f) override;

    void drawRectOutlined(float x, float y, float w, float h,
                          float fillR, float fillG, float fillB, float fillA,
                          float outR, float outG, float outB, float outA,
                          float outlineThickness) override;

    void drawCircle(float cx, float cy, float radius,
                    float r, float g, float b, float a = 1.f,
                    float outR = 0, float outG = 0, float outB = 0, float outA = 0,
                    float outlineThickness = 0.f) override;

    void drawRoundedRect(float x, float y, float w, float h,
                         float cornerRadius,
                         float r, float g, float b, float a = 1.f,
                         float outR = 0, float outG = 0, float outB = 0, float outA = 0,
                         float outlineThickness = 0.f) override;

    // ── Textured sprites ──────────────────────────────────────────────
    TextureHandle loadTexture(const std::string& path) override;
    void drawSprite(TextureHandle tex, float x, float y,
                    int frameX, int frameY, int frameW, int frameH,
                    float originX, float originY) override;

    // ── Text ──────────────────────────────────────────────────────────
    FontHandle loadFont(const std::string& path) override;
    void drawText(FontHandle font, const std::string& str,
                  float x, float y, unsigned int size,
                  float r, float g, float b, float a = 1.f) override;
    void measureText(FontHandle font, const std::string& str,
                     unsigned int size,
                     float& outWidth, float& outHeight) override;

    // ── Vertex-colored geometry ───────────────────────────────────────
    void drawTriangleStrip(const std::vector<Vertex>& verts) override;
    void drawLines(const std::vector<Vertex>& verts) override;

private:
    static sf::Color toSfColor(float r, float g, float b, float a);

    sf::RenderWindow m_window;
    SFMLInput        m_input;

    // Handle → resource maps
    std::unordered_map<TextureHandle, sf::Texture> m_textures;
    std::unordered_map<std::string, TextureHandle> m_texturePaths;
    TextureHandle m_nextTexHandle = 1;

    std::unordered_map<FontHandle, sf::Font> m_fonts;
    std::unordered_map<std::string, FontHandle> m_fontPaths;
    FontHandle m_nextFontHandle = 1;
};
