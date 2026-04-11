#include "GLRenderer.h"
#include "../Input/InputSystem.h"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <stdexcept>

// ── Shader sources ────────────────────────────────────────────────────────────

static const char* kFlatVS = R"(
#version 330 core
layout (location = 0) in vec2 aPos;

uniform mat4 uProjection;
uniform mat4 uModel;

void main()
{
    gl_Position = uProjection * uModel * vec4(aPos, 0.0, 1.0);
}
)";

static const char* kFlatFS = R"(
#version 330 core
out vec4 FragColor;

uniform vec4 uColor;

void main()
{
    FragColor = uColor;
}
)";

// ── GLFW callbacks ────────────────────────────────────────────────────────────

void GLRenderer::framebufferSizeCallback(GLFWwindow* win, int w, int h)
{
    glViewport(0, 0, w, h);
    GLRenderer* self = static_cast<GLRenderer*>(glfwGetWindowUserPointer(win));
    if (self)
    {
        self->m_windowW = static_cast<float>(w);
        self->m_windowH = static_cast<float>(h);
    }
}

// ── Constructor / Destructor ──────────────────────────────────────────────────

GLRenderer::GLRenderer(const std::string& title, unsigned int width,
                       unsigned int height, unsigned int fpsCap)
    : m_windowW(static_cast<float>(width))
    , m_windowH(static_cast<float>(height))
{
    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(static_cast<int>(width),
                                static_cast<int>(height),
                                title.c_str(), nullptr, nullptr);
    if (!m_window)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(m_window);

    if (fpsCap > 0)
        glfwSwapInterval(1); // VSync on
    else
        glfwSwapInterval(0);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        glfwDestroyWindow(m_window);
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLAD");
    }

    glViewport(0, 0, static_cast<int>(width), static_cast<int>(height));
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);

    // Build the flat-color shader
    m_flatShader = std::make_unique<Shader>(kFlatVS, kFlatFS);

    // Create the persistent unit-quad VAO
    initQuadVAO();

    // Default projection: top-left origin
    resetView();
}

GLRenderer::~GLRenderer()
{
    if (m_quadVAO) glDeleteVertexArrays(1, &m_quadVAO);
    if (m_quadVBO) glDeleteBuffers(1, &m_quadVBO);
    m_flatShader.reset();

    if (m_window)
        glfwDestroyWindow(m_window);
    glfwTerminate();
}

// ── Quad VAO (unit square 0,0 → 1,1) ─────────────────────────────────────────

void GLRenderer::initQuadVAO()
{
    // Two triangles forming a unit quad
    float verts[] = {
        0.f, 0.f,
        1.f, 0.f,
        1.f, 1.f,

        0.f, 0.f,
        1.f, 1.f,
        0.f, 1.f,
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);

    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

// ── Input (stub — GLFWInput will be added in a later task) ────────────────────

InputSystem& GLRenderer::getInput()
{
    // Will be replaced with a GLFWInput instance in a later task.
    // For now return the global current() so the build succeeds if something
    // has set it; this path is never reached until the switchover task.
    return InputSystem::current();
}

// ── Window operations ─────────────────────────────────────────────────────────

bool GLRenderer::isOpen() const
{
    return m_open && !glfwWindowShouldClose(m_window);
}

void GLRenderer::close()
{
    m_open = false;
    glfwSetWindowShouldClose(m_window, GLFW_TRUE);
}

void GLRenderer::setMouseCursorVisible(bool visible)
{
    glfwSetInputMode(m_window, GLFW_CURSOR,
                     visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
}

void GLRenderer::setWindowSize(unsigned int w, unsigned int h)
{
    glfwSetWindowSize(m_window, static_cast<int>(w), static_cast<int>(h));
    m_windowW = static_cast<float>(w);
    m_windowH = static_cast<float>(h);
}

void GLRenderer::setWindowPosition(int x, int y)
{
    glfwSetWindowPos(m_window, x, y);
}

void GLRenderer::getDesktopSize(unsigned int& w, unsigned int& h) const
{
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    w = static_cast<unsigned int>(mode->width);
    h = static_cast<unsigned int>(mode->height);
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void GLRenderer::clear(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void GLRenderer::display()
{
    glfwSwapBuffers(m_window);
}

// ── View / camera ─────────────────────────────────────────────────────────────

void GLRenderer::setView(float centerX, float centerY,
                         float width, float height)
{
    float halfW = width  * 0.5f;
    float halfH = height * 0.5f;
    // Top-left origin: top = centerY - halfH, bottom = centerY + halfH
    m_projection = glm::ortho(centerX - halfW, centerX + halfW,
                              centerY + halfH, centerY - halfH,
                              -1.f, 1.f);
}

void GLRenderer::resetView()
{
    // Top-left origin to match SFML convention
    m_projection = glm::ortho(0.f, m_windowW, m_windowH, 0.f, -1.f, 1.f);
}

void GLRenderer::getWindowSize(float& w, float& h) const
{
    w = m_windowW;
    h = m_windowH;
}

// ── Primitive helpers ─────────────────────────────────────────────────────────

void GLRenderer::drawQuad(float x, float y, float w, float h,
                          float r, float g, float b, float a)
{
    m_flatShader->use();
    m_flatShader->setMat4("uProjection", m_projection);

    glm::mat4 model(1.f);
    model = glm::translate(model, glm::vec3(x, y, 0.f));
    model = glm::scale(model, glm::vec3(w, h, 1.f));
    m_flatShader->setMat4("uModel", model);

    m_flatShader->setVec4("uColor", glm::vec4(r, g, b, a));

    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// ── Primitives ────────────────────────────────────────────────────────────────

void GLRenderer::drawRect(float x, float y, float w, float h,
                          float r, float g, float b, float a)
{
    drawQuad(x, y, w, h, r, g, b, a);
}

void GLRenderer::drawRectOutlined(float x, float y, float w, float h,
                                  float fillR, float fillG, float fillB, float fillA,
                                  float outR, float outG, float outB, float outA,
                                  float outlineThickness)
{
    // Draw the filled rectangle
    drawQuad(x, y, w, h, fillR, fillG, fillB, fillA);

    if (outlineThickness <= 0.f)
        return;

    float t = outlineThickness;

    // Top edge
    drawQuad(x - t, y - t, w + 2 * t, t,
             outR, outG, outB, outA);
    // Bottom edge
    drawQuad(x - t, y + h, w + 2 * t, t,
             outR, outG, outB, outA);
    // Left edge
    drawQuad(x - t, y, t, h,
             outR, outG, outB, outA);
    // Right edge
    drawQuad(x + w, y, t, h,
             outR, outG, outB, outA);
}

// ── Stubs for methods implemented in later tasks ──────────────────────────────

void GLRenderer::drawCircle(float, float, float,
                            float, float, float, float,
                            float, float, float, float,
                            float)
{
    // Will be implemented in a later task (circle/rounded-rect/tri-strip).
}

void GLRenderer::drawRoundedRect(float, float, float, float,
                                 float,
                                 float, float, float, float,
                                 float, float, float, float,
                                 float)
{
    // Will be implemented in a later task.
}

Renderer::TextureHandle GLRenderer::loadTexture(const std::string&)
{
    // Will be implemented in the texture/sprite task.
    return 0;
}

void GLRenderer::drawSprite(TextureHandle, float, float,
                            int, int, int, int,
                            float, float)
{
    // Will be implemented in the texture/sprite task.
}

Renderer::FontHandle GLRenderer::loadFont(const std::string&)
{
    // Will be implemented in the text rendering task.
    return 0;
}

void GLRenderer::drawText(FontHandle, const std::string&,
                           float, float, unsigned int,
                           float, float, float, float)
{
    // Will be implemented in the text rendering task.
}

void GLRenderer::measureText(FontHandle, const std::string&,
                              unsigned int,
                              float& outWidth, float& outHeight)
{
    // Will be implemented in the text rendering task.
    outWidth = outHeight = 0.f;
}

void GLRenderer::drawTriangleStrip(const std::vector<Vertex>&)
{
    // Will be implemented in the vertex-colored geometry task.
}

void GLRenderer::drawLines(const std::vector<Vertex>&)
{
    // Will be implemented in the vertex-colored geometry task.
}
