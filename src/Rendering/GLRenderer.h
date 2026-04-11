#pragma once

#include "Renderer.h"
#include "Shader.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <memory>
#include <unordered_map>
#include <string>

class InputSystem;

// OpenGL 3.3 Core implementation of the Renderer interface.
// Owns the GLFWwindow, shaders, and persistent GPU resources.
class GLRenderer : public Renderer
{
public:
    GLRenderer(const std::string& title, unsigned int width,
               unsigned int height, unsigned int fpsCap = 60);
    ~GLRenderer() override;

    GLRenderer(const GLRenderer&) = delete;
    GLRenderer& operator=(const GLRenderer&) = delete;

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
    void initQuadVAO();
    void drawQuad(float x, float y, float w, float h,
                  float r, float g, float b, float a);

    static void framebufferSizeCallback(GLFWwindow* win, int w, int h);

    GLFWwindow* m_window = nullptr;
    float       m_windowW = 0.f;
    float       m_windowH = 0.f;
    bool        m_open = true;

    // Flat-color shader and persistent quad VAO
    std::unique_ptr<Shader> m_flatShader;
    GLuint m_quadVAO = 0;
    GLuint m_quadVBO = 0;

    // Current projection matrix
    glm::mat4 m_projection{1.f};
};
