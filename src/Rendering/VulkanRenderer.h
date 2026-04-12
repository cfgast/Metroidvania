#pragma once

#include "Renderer.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <array>
#include <memory>
#include <string>
#include <unordered_map>
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
    void createTexturedPipeline();

    // ── Texture helpers ──────────────────────────────────────────────
    void createTextureDescriptorResources();
    void cleanupTextureResources();
    VkCommandBuffer beginOneTimeCommands();
    void endOneTimeCommands(VkCommandBuffer cmd);

    // ── Sprite/lighting resource helpers ──────────────────────────────
    void createLightingResources();
    void createFlatNormalTexture();
    void createTextureFlagsUBO();
    VkDescriptorSet getOrCreateTextureDescriptorSet(TextureHandle diffuse,
                                                     TextureHandle normal);

    // ── Frame recording helpers ──────────────────────────────────────
    void ensureFrameStarted();

    // ── VMA / buffer helpers ─────────────────────────────────────────
    void initAllocator();
    void createQuadBuffer();
    void createDynamicBuffers();

    // Sub-allocate from the current frame's dynamic buffer.
    // Returns buffer handle, byte offset, and a host-visible pointer to
    // write vertex data into. Returns data==nullptr on overflow.
    struct DynamicAllocation {
        VkBuffer     buffer = VK_NULL_HANDLE;
        VkDeviceSize offset = 0;
        void*        data   = nullptr;
    };
    DynamicAllocation dynamicAllocate(VkDeviceSize size,
                                     VkDeviceSize alignment = 16);

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

    // ── Textured sprite pipeline ──────────────────────────────────────
    VkPipelineLayout m_texturedPipelineLayout = VK_NULL_HANDLE;
    VkPipeline       m_texturedPipeline       = VK_NULL_HANDLE;

    // ── VMA allocator ─────────────────────────────────────────────────
    VmaAllocator m_allocator = VK_NULL_HANDLE;

    // ── Unit quad vertex buffer (GPU-local, static) ───────────────────
    VkBuffer      m_quadBuffer     = VK_NULL_HANDLE;
    VmaAllocation m_quadAllocation = VK_NULL_HANDLE;
    static constexpr uint32_t QUAD_VERTEX_COUNT = 6;

    // ── Per-frame dynamic vertex buffers (host-visible, reset each frame)
    static constexpr VkDeviceSize DYNAMIC_BUFFER_SIZE = 4 * 1024 * 1024; // 4 MB
    struct FrameDynamicBuffer {
        VkBuffer      buffer     = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        void*         mapped     = nullptr;
        VkDeviceSize  offset     = 0;
    };
    std::array<FrameDynamicBuffer, MAX_FRAMES_IN_FLIGHT> m_dynamicBuffers{};

    // ── Frame recording state ────────────────────────────────────────
    bool     m_frameStarted      = false;
    uint32_t m_currentImageIndex = 0;

    // ── Projection matrix ────────────────────────────────────────────
    glm::mat4 m_projection{1.f};

    // ── Texture system ───────────────────────────────────────────────
    struct VulkanTextureInfo {
        VkImage       image      = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkImageView   view       = VK_NULL_HANDLE;
        VkSampler     sampler    = VK_NULL_HANDLE;
        int           width      = 0;
        int           height     = 0;
    };

    std::unordered_map<TextureHandle, VulkanTextureInfo> m_textures;
    std::unordered_map<std::string, TextureHandle>       m_texturePaths;
    TextureHandle m_nextTexHandle = 1;

    // Descriptor set layout for texture binding (set 1):
    //   binding 0: combined image sampler (diffuse)
    //   binding 1: combined image sampler (normal map)
    VkDescriptorSetLayout m_textureDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool      m_textureDescriptorPool      = VK_NULL_HANDLE;

    // Descriptor set cache keyed by (diffuseHandle, normalHandle)
    struct DescriptorSetKey {
        TextureHandle diffuse = 0;
        TextureHandle normal  = 0;
        bool operator==(const DescriptorSetKey& o) const {
            return diffuse == o.diffuse && normal == o.normal;
        }
    };
    struct DescriptorSetKeyHash {
        size_t operator()(const DescriptorSetKey& k) const {
            return std::hash<uint64_t>()(k.diffuse) ^
                   (std::hash<uint64_t>()(k.normal) << 1);
        }
    };
    std::unordered_map<DescriptorSetKey, VkDescriptorSet,
                       DescriptorSetKeyHash> m_textureDescriptorCache;

    static constexpr uint32_t MAX_TEXTURE_DESCRIPTOR_SETS = 256;

    // ── Dummy lighting UBO (placeholder until lighting is ported) ─────
    VkBuffer         m_lightingUBO              = VK_NULL_HANDLE;
    VmaAllocation    m_lightingUBOAllocation    = VK_NULL_HANDLE;
    VkDescriptorPool m_lightingDescriptorPool   = VK_NULL_HANDLE;
    VkDescriptorSet  m_lightingDescriptorSet    = VK_NULL_HANDLE;

    // ── Texture flags UBO (uHasNormalMap) ─────────────────────────────
    VkBuffer      m_textureFlagsUBO           = VK_NULL_HANDLE;
    VmaAllocation m_textureFlagsUBOAllocation = VK_NULL_HANDLE;

    // ── Default flat normal texture ──────────────────────────────────
    TextureHandle m_flatNormalTexture = 0;
};
