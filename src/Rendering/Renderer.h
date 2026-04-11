#pragma once

#include <string>
#include <vector>
#include <cstdint>

// Pure-virtual rendering interface.
// All game code draws through this API so the back-end (SFML, OpenGL, …)
// can be swapped without touching callers.
class Renderer
{
public:
    virtual ~Renderer() = default;

    // ── Lifecycle ──────────────────────────────────────────────────────
    virtual void clear(float r, float g, float b, float a = 1.f) = 0;
    virtual void display() = 0;

    // ── View / camera ─────────────────────────────────────────────────
    virtual void setView(float centerX, float centerY,
                         float width, float height) = 0;
    virtual void resetView() = 0;
    virtual void getWindowSize(float& w, float& h) const = 0;

    // ── Primitives ────────────────────────────────────────────────────
    virtual void drawRect(float x, float y, float w, float h,
                          float r, float g, float b, float a = 1.f) = 0;

    virtual void drawRectOutlined(float x, float y, float w, float h,
                                  float fillR, float fillG, float fillB, float fillA,
                                  float outR, float outG, float outB, float outA,
                                  float outlineThickness) = 0;

    virtual void drawCircle(float cx, float cy, float radius,
                            float r, float g, float b, float a = 1.f,
                            float outR = 0, float outG = 0, float outB = 0, float outA = 0,
                            float outlineThickness = 0.f) = 0;

    virtual void drawRoundedRect(float x, float y, float w, float h,
                                 float radius,
                                 float r, float g, float b, float a = 1.f,
                                 float outR = 0, float outG = 0, float outB = 0, float outA = 0,
                                 float outlineThickness = 0.f) = 0;

    // ── Textured sprites ──────────────────────────────────────────────
    using TextureHandle = uint64_t;
    virtual TextureHandle loadTexture(const std::string& path) = 0;
    virtual void drawSprite(TextureHandle tex, float x, float y,
                            int frameX, int frameY, int frameW, int frameH,
                            float originX, float originY) = 0;

    // ── Text ──────────────────────────────────────────────────────────
    using FontHandle = uint64_t;
    virtual FontHandle loadFont(const std::string& path) = 0;
    virtual void drawText(FontHandle font, const std::string& str,
                          float x, float y, unsigned int size,
                          float r, float g, float b, float a = 1.f) = 0;
    virtual void measureText(FontHandle font, const std::string& str,
                             unsigned int size,
                             float& outWidth, float& outHeight) = 0;

    // ── Vertex-colored geometry ───────────────────────────────────────
    struct Vertex { float x, y, r, g, b, a; };
    virtual void drawTriangleStrip(const std::vector<Vertex>& verts) = 0;
    virtual void drawLines(const std::vector<Vertex>& verts) = 0;
};
