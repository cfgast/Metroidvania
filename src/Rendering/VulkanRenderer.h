#pragma once

#include "Renderer.h"

#include <GLFW/glfw3.h>
#include <memory>
#include <string>

class InputSystem;

// Vulkan 1.3 implementation of the Renderer interface.
// Currently a stub — all draw calls are no-ops. Creates a GLFW window
// without an OpenGL context (GLFW_NO_API) ready for Vulkan surface creation.
class VulkanRenderer : public Renderer
{
public:
    VulkanRenderer(const std::string& title, unsigned int width,
                   unsigned int height, unsigned int fpsCap = 60);
    ~VulkanRenderer() override;

    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;

    // ── Input ─────────────────────────────────────────────────────────
    InputSystem& getInput() override;

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

    // ── Frame / lighting pass ─────────────────────────────────────────
    void beginFrame() override;
    void endFrame() override;
    void addLight(const Light& light) override;
    void clearLights() override;
    void setAmbientColor(float r, float g, float b) override;

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

    GLFWwindow* getWindow() const { return m_window; }

private:
    GLFWwindow* m_window = nullptr;
    float       m_windowW = 0.f;
    float       m_windowH = 0.f;
    bool        m_open = true;

    std::unique_ptr<InputSystem> m_input;
};
