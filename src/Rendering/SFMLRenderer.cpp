#include "SFMLRenderer.h"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Window/VideoMode.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>

// ── Helpers ───────────────────────────────────────────────────────────────────

sf::Color SFMLRenderer::toSfColor(float r, float g, float b, float a)
{
    auto clamp = [](float v) -> sf::Uint8 {
        return static_cast<sf::Uint8>(std::max(0.f, std::min(255.f, v * 255.f)));
    };
    return { clamp(r), clamp(g), clamp(b), clamp(a) };
}

// ── Constructor ───────────────────────────────────────────────────────────────

SFMLRenderer::SFMLRenderer(const std::string& title, unsigned int width,
                           unsigned int height, unsigned int fpsCap)
    : m_window(sf::VideoMode(width, height), title)
{
    m_window.setFramerateLimit(fpsCap);
}

// ── Window operations ─────────────────────────────────────────────────────────

bool SFMLRenderer::isOpen() const { return m_window.isOpen(); }
void SFMLRenderer::close()       { m_window.close(); }

void SFMLRenderer::setMouseCursorVisible(bool visible)
{
    m_window.setMouseCursorVisible(visible);
}

bool SFMLRenderer::pollEvent(sf::Event& event)
{
    return m_window.pollEvent(event);
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void SFMLRenderer::clear(float r, float g, float b, float a)
{
    m_window.clear(toSfColor(r, g, b, a));
}

void SFMLRenderer::display()
{
    m_window.display();
}

// ── View / camera ─────────────────────────────────────────────────────────────

void SFMLRenderer::setView(float centerX, float centerY,
                           float width, float height)
{
    sf::View view;
    view.setCenter(centerX, centerY);
    view.setSize(width, height);
    m_window.setView(view);
}

void SFMLRenderer::resetView()
{
    sf::Vector2u sz = m_window.getSize();
    sf::View view(sf::FloatRect(0.f, 0.f,
                                static_cast<float>(sz.x),
                                static_cast<float>(sz.y)));
    m_window.setView(view);
}

void SFMLRenderer::getWindowSize(float& w, float& h) const
{
    sf::Vector2u sz = m_window.getSize();
    w = static_cast<float>(sz.x);
    h = static_cast<float>(sz.y);
}

// ── Primitives ────────────────────────────────────────────────────────────────

void SFMLRenderer::drawRect(float x, float y, float w, float h,
                            float r, float g, float b, float a)
{
    sf::RectangleShape rect({w, h});
    rect.setPosition(x, y);
    rect.setFillColor(toSfColor(r, g, b, a));
    m_window.draw(rect);
}

void SFMLRenderer::drawRectOutlined(float x, float y, float w, float h,
                                    float fillR, float fillG, float fillB, float fillA,
                                    float outR, float outG, float outB, float outA,
                                    float outlineThickness)
{
    sf::RectangleShape rect({w, h});
    rect.setPosition(x, y);
    rect.setFillColor(toSfColor(fillR, fillG, fillB, fillA));
    rect.setOutlineColor(toSfColor(outR, outG, outB, outA));
    rect.setOutlineThickness(outlineThickness);
    m_window.draw(rect);
}

void SFMLRenderer::drawCircle(float cx, float cy, float radius,
                              float r, float g, float b, float a,
                              float outR, float outG, float outB, float outA,
                              float outlineThickness)
{
    sf::CircleShape circle(radius);
    circle.setOrigin(radius, radius);
    circle.setPosition(cx, cy);
    circle.setFillColor(toSfColor(r, g, b, a));
    if (outlineThickness > 0.f)
    {
        circle.setOutlineColor(toSfColor(outR, outG, outB, outA));
        circle.setOutlineThickness(outlineThickness);
    }
    m_window.draw(circle);
}

void SFMLRenderer::drawRoundedRect(float x, float y, float w, float h,
                                   float cornerRadius,
                                   float r, float g, float b, float a,
                                   float outR, float outG, float outB, float outA,
                                   float outlineThickness)
{
    // Build a convex shape with rounded corners (6 points per corner).
    constexpr unsigned int PTS_PER_CORNER = 6;
    constexpr unsigned int TOTAL_POINTS   = PTS_PER_CORNER * 4;
    constexpr float PI = 3.14159265f;

    float cr = std::min(cornerRadius, std::min(w * 0.5f, h * 0.5f));

    sf::ConvexShape shape;
    shape.setPointCount(TOTAL_POINTS);

    auto setCorner = [&](unsigned int base, float cxOff, float cyOff,
                         float startDeg) {
        for (unsigned int i = 0; i < PTS_PER_CORNER; ++i)
        {
            float angle = startDeg +
                90.f * static_cast<float>(i) /
                    static_cast<float>(PTS_PER_CORNER - 1);
            float rad = angle * PI / 180.f;
            shape.setPoint(base + i,
                           {cxOff + cr * std::cos(rad),
                            cyOff - cr * std::sin(rad)});
        }
    };

    // Top-right, bottom-right, bottom-left, top-left
    setCorner(0 * PTS_PER_CORNER, w - cr, cr,      0.f);
    setCorner(1 * PTS_PER_CORNER, w - cr, h - cr, -90.f);
    setCorner(2 * PTS_PER_CORNER, cr,     h - cr, -180.f);
    setCorner(3 * PTS_PER_CORNER, cr,     cr,     -270.f);

    shape.setPosition(x, y);
    shape.setFillColor(toSfColor(r, g, b, a));
    if (outlineThickness > 0.f)
    {
        shape.setOutlineColor(toSfColor(outR, outG, outB, outA));
        shape.setOutlineThickness(outlineThickness);
    }
    m_window.draw(shape);
}

// ── Textured sprites ──────────────────────────────────────────────────────────

Renderer::TextureHandle SFMLRenderer::loadTexture(const std::string& path)
{
    auto it = m_texturePaths.find(path);
    if (it != m_texturePaths.end())
        return it->second;

    TextureHandle handle = m_nextTexHandle++;
    sf::Texture& tex = m_textures[handle];

    if (!tex.loadFromFile(path))
    {
        // Fallback: solid magenta so missing art is obvious.
        sf::Image img;
        img.create(256, 256, sf::Color::Magenta);
        tex.loadFromImage(img);
        std::cerr << "SFMLRenderer: failed to load texture: " << path << "\n";
    }

    m_texturePaths[path] = handle;
    return handle;
}

void SFMLRenderer::drawSprite(TextureHandle tex, float x, float y,
                              int frameX, int frameY, int frameW, int frameH,
                              float originX, float originY)
{
    auto it = m_textures.find(tex);
    if (it == m_textures.end())
        return;

    sf::Sprite sprite(it->second);
    sprite.setTextureRect({frameX, frameY, frameW, frameH});
    sprite.setOrigin(originX, originY);
    sprite.setPosition(x, y);
    m_window.draw(sprite);
}

// ── Text ──────────────────────────────────────────────────────────────────────

Renderer::FontHandle SFMLRenderer::loadFont(const std::string& path)
{
    auto it = m_fontPaths.find(path);
    if (it != m_fontPaths.end())
        return it->second;

    FontHandle handle = m_nextFontHandle++;
    sf::Font& font = m_fonts[handle];

    if (!font.loadFromFile(path))
        std::cerr << "SFMLRenderer: failed to load font: " << path << "\n";

    m_fontPaths[path] = handle;
    return handle;
}

void SFMLRenderer::drawText(FontHandle font, const std::string& str,
                            float x, float y, unsigned int size,
                            float r, float g, float b, float a)
{
    auto it = m_fonts.find(font);
    if (it == m_fonts.end())
        return;

    sf::Text text(str, it->second, size);
    text.setPosition(x, y);
    text.setFillColor(toSfColor(r, g, b, a));
    m_window.draw(text);
}

void SFMLRenderer::measureText(FontHandle font, const std::string& str,
                               unsigned int size,
                               float& outWidth, float& outHeight)
{
    auto it = m_fonts.find(font);
    if (it == m_fonts.end())
    {
        outWidth = outHeight = 0.f;
        return;
    }

    sf::Text text(str, it->second, size);
    sf::FloatRect bounds = text.getLocalBounds();
    outWidth  = bounds.width;
    outHeight = bounds.height;
}

// ── Vertex-colored geometry ───────────────────────────────────────────────────

void SFMLRenderer::drawTriangleStrip(const std::vector<Vertex>& verts)
{
    sf::VertexArray va(sf::TrianglesStrip, verts.size());
    for (std::size_t i = 0; i < verts.size(); ++i)
    {
        va[i].position = {verts[i].x, verts[i].y};
        va[i].color    = toSfColor(verts[i].r, verts[i].g, verts[i].b, verts[i].a);
    }
    m_window.draw(va);
}

void SFMLRenderer::drawLines(const std::vector<Vertex>& verts)
{
    sf::VertexArray va(sf::Lines, verts.size());
    for (std::size_t i = 0; i < verts.size(); ++i)
    {
        va[i].position = {verts[i].x, verts[i].y};
        va[i].color    = toSfColor(verts[i].r, verts[i].g, verts[i].b, verts[i].a);
    }
    m_window.draw(va);
}
