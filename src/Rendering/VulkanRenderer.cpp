#include "VulkanRenderer.h"
#include "../Input/InputSystem.h"

#include <iostream>
#include <stdexcept>

// ── Stub InputSystem for the Vulkan backend ──────────────────────────────────
// Minimal implementation that processes GLFW window events so the window
// remains responsive (can be closed, resized, etc.). Key/gamepad queries
// return no-op values. Will be replaced with the full GLFW input system
// once the input layer is decoupled from GLRenderer.

class VulkanInput : public InputSystem
{
public:
    explicit VulkanInput(GLFWwindow* window)
        : m_window(window)
    {
        s_instance = this;
    }

    ~VulkanInput() override
    {
        if (s_instance == this)
            s_instance = nullptr;
    }

    bool pollEvent(InputEvent& /*event*/) override
    {
        glfwPollEvents();
        return false;
    }

    bool  isKeyPressed(KeyCode /*key*/) const override          { return false; }
    bool  isGamepadConnected(int /*id*/) const override         { return false; }
    float getGamepadAxis(int /*id*/, GamepadAxis /*a*/) const override { return 0.f; }
    bool  isGamepadButtonPressed(int /*id*/, GamepadButton /*b*/) const override { return false; }
    void  setMouseCursorVisible(bool visible) override
    {
        glfwSetInputMode(m_window, GLFW_CURSOR,
                         visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
    }

private:
    GLFWwindow* m_window;
};

// ── Constructor / Destructor ─────────────────────────────────────────────────

VulkanRenderer::VulkanRenderer(const std::string& title, unsigned int width,
                               unsigned int height, unsigned int /*fpsCap*/)
    : m_windowW(static_cast<float>(width))
    , m_windowH(static_cast<float>(height))
{
    if (!glfwInit())
        throw std::runtime_error("VulkanRenderer: failed to initialize GLFW");

    // No OpenGL context — this window will be used with a Vulkan surface
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_window = glfwCreateWindow(static_cast<int>(width),
                                static_cast<int>(height),
                                title.c_str(), nullptr, nullptr);
    if (!m_window)
    {
        glfwTerminate();
        throw std::runtime_error("VulkanRenderer: failed to create GLFW window");
    }

    glfwSetWindowUserPointer(m_window, this);

    m_input = std::make_unique<VulkanInput>(m_window);
}

VulkanRenderer::~VulkanRenderer()
{
    m_input.reset();

    if (m_window)
        glfwDestroyWindow(m_window);
    glfwTerminate();
}

// ── Input ────────────────────────────────────────────────────────────────────

InputSystem& VulkanRenderer::getInput() { return *m_input; }

// ── Window operations ────────────────────────────────────────────────────────

bool VulkanRenderer::isOpen() const
{
    return m_open && !glfwWindowShouldClose(m_window);
}

void VulkanRenderer::close()
{
    m_open = false;
    glfwSetWindowShouldClose(m_window, GLFW_TRUE);
}

void VulkanRenderer::setMouseCursorVisible(bool visible)
{
    glfwSetInputMode(m_window, GLFW_CURSOR,
                     visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
}

void VulkanRenderer::setWindowSize(unsigned int w, unsigned int h)
{
    glfwSetWindowSize(m_window, static_cast<int>(w), static_cast<int>(h));
    m_windowW = static_cast<float>(w);
    m_windowH = static_cast<float>(h);
}

void VulkanRenderer::setWindowPosition(int x, int y)
{
    glfwSetWindowPos(m_window, x, y);
}

void VulkanRenderer::getDesktopSize(unsigned int& w, unsigned int& h) const
{
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    w = mode ? static_cast<unsigned int>(mode->width)  : 1920;
    h = mode ? static_cast<unsigned int>(mode->height) : 1080;
}

// ── Lifecycle ────────────────────────────────────────────────────────────────

void VulkanRenderer::clear(float /*r*/, float /*g*/, float /*b*/, float /*a*/) {}
void VulkanRenderer::display() {}

// ── Frame / lighting pass ────────────────────────────────────────────────────

void VulkanRenderer::beginFrame() {}
void VulkanRenderer::endFrame() {}
void VulkanRenderer::addLight(const Light& /*light*/) {}
void VulkanRenderer::clearLights() {}
void VulkanRenderer::setAmbientColor(float /*r*/, float /*g*/, float /*b*/) {}

// ── View / camera ────────────────────────────────────────────────────────────

void VulkanRenderer::setView(float /*centerX*/, float /*centerY*/,
                             float /*width*/, float /*height*/) {}
void VulkanRenderer::resetView() {}

void VulkanRenderer::getWindowSize(float& w, float& h) const
{
    w = m_windowW;
    h = m_windowH;
}

// ── Primitives ───────────────────────────────────────────────────────────────

void VulkanRenderer::drawRect(float /*x*/, float /*y*/, float /*w*/, float /*h*/,
                              float /*r*/, float /*g*/, float /*b*/, float /*a*/) {}

void VulkanRenderer::drawRectOutlined(float /*x*/, float /*y*/, float /*w*/, float /*h*/,
                                      float /*fillR*/, float /*fillG*/, float /*fillB*/, float /*fillA*/,
                                      float /*outR*/, float /*outG*/, float /*outB*/, float /*outA*/,
                                      float /*outlineThickness*/) {}

void VulkanRenderer::drawCircle(float /*cx*/, float /*cy*/, float /*radius*/,
                                float /*r*/, float /*g*/, float /*b*/, float /*a*/,
                                float /*outR*/, float /*outG*/, float /*outB*/, float /*outA*/,
                                float /*outlineThickness*/) {}

void VulkanRenderer::drawRoundedRect(float /*x*/, float /*y*/, float /*w*/, float /*h*/,
                                     float /*cornerRadius*/,
                                     float /*r*/, float /*g*/, float /*b*/, float /*a*/,
                                     float /*outR*/, float /*outG*/, float /*outB*/, float /*outA*/,
                                     float /*outlineThickness*/) {}

// ── Textured sprites ─────────────────────────────────────────────────────────

Renderer::TextureHandle VulkanRenderer::loadTexture(const std::string& /*path*/) { return 0; }

void VulkanRenderer::drawSprite(TextureHandle /*tex*/, float /*x*/, float /*y*/,
                                int /*frameX*/, int /*frameY*/, int /*frameW*/, int /*frameH*/,
                                float /*originX*/, float /*originY*/) {}

// ── Text ─────────────────────────────────────────────────────────────────────

Renderer::FontHandle VulkanRenderer::loadFont(const std::string& /*path*/) { return 0; }

void VulkanRenderer::drawText(FontHandle /*font*/, const std::string& /*str*/,
                              float /*x*/, float /*y*/, unsigned int /*size*/,
                              float /*r*/, float /*g*/, float /*b*/, float /*a*/) {}

void VulkanRenderer::measureText(FontHandle /*font*/, const std::string& /*str*/,
                                 unsigned int /*size*/,
                                 float& outWidth, float& outHeight)
{
    outWidth  = 0.f;
    outHeight = 0.f;
}

// ── Vertex-colored geometry ──────────────────────────────────────────────────

void VulkanRenderer::drawTriangleStrip(const std::vector<Vertex>& /*verts*/) {}
void VulkanRenderer::drawLines(const std::vector<Vertex>& /*verts*/) {}
