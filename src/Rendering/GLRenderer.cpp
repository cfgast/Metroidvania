#include "GLRenderer.h"
#include "../Input/InputSystem.h"

#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
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

static const char* kTexVS = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 uProjection;

out vec2 vTexCoord;

void main()
{
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
)";

static const char* kTexFS = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;

void main()
{
    FragColor = texture(uTexture, vTexCoord);
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

    // Build shaders
    m_flatShader = std::make_unique<Shader>(kFlatVS, kFlatFS);
    m_texShader  = std::make_unique<Shader>(kTexVS, kTexFS);

    // Create persistent GPU buffers
    initQuadVAO();
    initSpriteVAO();

    // Default projection: top-left origin
    resetView();
}

GLRenderer::~GLRenderer()
{
    if (m_quadVAO) glDeleteVertexArrays(1, &m_quadVAO);
    if (m_quadVBO) glDeleteBuffers(1, &m_quadVBO);
    if (m_spriteVAO) glDeleteVertexArrays(1, &m_spriteVAO);
    if (m_spriteVBO) glDeleteBuffers(1, &m_spriteVBO);

    for (auto& [handle, info] : m_textures)
    {
        if (info.glId) glDeleteTextures(1, &info.glId);
    }

    m_flatShader.reset();
    m_texShader.reset();

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

// ── Sprite VAO (dynamic, position + texcoord) ────────────────────────────────

void GLRenderer::initSpriteVAO()
{
    glGenVertexArrays(1, &m_spriteVAO);
    glGenBuffers(1, &m_spriteVBO);

    glBindVertexArray(m_spriteVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_spriteVBO);
    // 6 vertices × 4 floats (x, y, u, v) — re-uploaded each draw call
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    // position (location 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    // texcoord (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// ── Magenta fallback texture ──────────────────────────────────────────────────

GLuint GLRenderer::createMagentaTexture()
{
    unsigned char magenta[2 * 2 * 4] = {
        255, 0, 255, 255,   255, 0, 255, 255,
        255, 0, 255, 255,   255, 0, 255, 255,
    };

    GLuint texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, magenta);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texId;
}

// ── Textured sprites ──────────────────────────────────────────────────────────

Renderer::TextureHandle GLRenderer::loadTexture(const std::string& path)
{
    // Return cached handle if already loaded
    auto it = m_texturePaths.find(path);
    if (it != m_texturePaths.end())
        return it->second;

    TextureHandle handle = m_nextTexHandle++;
    TextureInfo info;

    stbi_set_flip_vertically_on_load(0); // top-left origin, no flip
    int channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &info.width, &info.height,
                                    &channels, 4); // force RGBA

    if (!data)
    {
        std::cerr << "GLRenderer: failed to load texture: " << path << "\n";
        info.glId   = createMagentaTexture();
        info.width  = 2;
        info.height = 2;
    }
    else
    {
        glGenTextures(1, &info.glId);
        glBindTexture(GL_TEXTURE_2D, info.glId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, info.width, info.height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(data);
    }

    m_textures[handle] = info;
    m_texturePaths[path] = handle;
    return handle;
}

void GLRenderer::drawSprite(TextureHandle tex, float x, float y,
                            int frameX, int frameY, int frameW, int frameH,
                            float originX, float originY)
{
    auto it = m_textures.find(tex);
    if (it == m_textures.end())
        return;

    const TextureInfo& info = it->second;
    float texW = static_cast<float>(info.width);
    float texH = static_cast<float>(info.height);

    // Compute UV coordinates from the frame rect
    float u0 = static_cast<float>(frameX) / texW;
    float v0 = static_cast<float>(frameY) / texH;
    float u1 = static_cast<float>(frameX + frameW) / texW;
    float v1 = static_cast<float>(frameY + frameH) / texH;

    // Position quad with origin offset (top-left origin)
    float px = x - originX;
    float py = y - originY;
    float pw = static_cast<float>(frameW);
    float ph = static_cast<float>(frameH);

    // 6 vertices: 2 triangles, each with (x, y, u, v)
    float verts[] = {
        px,      py,      u0, v0,
        px + pw, py,      u1, v0,
        px + pw, py + ph, u1, v1,

        px,      py,      u0, v0,
        px + pw, py + ph, u1, v1,
        px,      py + ph, u0, v1,
    };

    m_texShader->use();
    m_texShader->setMat4("uProjection", m_projection);
    m_texShader->setInt("uTexture", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, info.glId);

    glBindVertexArray(m_spriteVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_spriteVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
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
