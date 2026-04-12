#pragma once

#include "Renderer.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <array>
#include <memory>
#include <string>
#include <vector>

class InputSystem;

static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

// Vulkan 1.3 implementation of the Renderer interface.
// Initializes Vulkan instance, device, swap chain, command buffers, and
// per-frame synchronization primitives via vk-bootstrap.
// The frame loop clears to a solid color each frame using dynamic rendering.
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
    friend void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    void initVulkan();
    void createSwapchain();
    void destroySwapchain();
    void createCommandPool();
    void createSyncObjects();
    void cleanupVulkan();

    void recreateSwapchain();
    void transitionImageLayout(VkCommandBuffer cmd, VkImage image,
                               VkImageLayout oldLayout, VkImageLayout newLayout);

    // ── Pipeline helpers ──────────────────────────────────────────────
    static std::vector<char> readSPVFile(const std::string& path);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createFlatPipeline();

    // ── GLFW ──────────────────────────────────────────────────────────
    GLFWwindow* m_window = nullptr;
    float       m_windowW = 0.f;
    float       m_windowH = 0.f;
    bool        m_open = true;
    bool        m_framebufferResized = false;

    std::unique_ptr<InputSystem> m_input;

    // ── Vulkan core ───────────────────────────────────────────────────
    VkInstance               m_instance       = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR             m_surface        = VK_NULL_HANDLE;
    VkPhysicalDevice         m_physicalDevice = VK_NULL_HANDLE;
    VkDevice                 m_device         = VK_NULL_HANDLE;
    VkQueue                  m_graphicsQueue  = VK_NULL_HANDLE;
    VkQueue                  m_presentQueue   = VK_NULL_HANDLE;
    uint32_t                 m_graphicsQueueFamily = 0;
    uint32_t                 m_presentQueueFamily  = 0;

    // ── Swap chain ────────────────────────────────────────────────────
    VkSwapchainKHR           m_swapchain       = VK_NULL_HANDLE;
    VkFormat                 m_swapchainFormat  = VK_FORMAT_UNDEFINED;
    VkExtent2D               m_swapchainExtent = {0, 0};
    std::vector<VkImage>     m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;

    // ── Command buffers ───────────────────────────────────────────────
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> m_commandBuffers{};

    // ── Per-frame synchronization ─────────────────────────────────────
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_imageAvailable{};
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_renderFinished{};
    std::array<VkFence,     MAX_FRAMES_IN_FLIGHT> m_inFlight{};
    uint32_t m_currentFrame = 0;

    // ── Clear color ───────────────────────────────────────────────────
    VkClearColorValue m_clearColor = {{0.0f, 0.0f, 0.0f, 1.0f}};

    // ── Flat-color pipeline ───────────────────────────────────────────
    VkDescriptorSetLayout m_flatDescriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout      m_flatPipelineLayout      = VK_NULL_HANDLE;
    VkPipeline            m_flatPipeline            = VK_NULL_HANDLE;
};
