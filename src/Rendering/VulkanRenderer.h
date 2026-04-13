#pragma once

#include "Renderer.h"
#include "Light.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <array>
#include <map>
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
                    float originX, float originY,
                    float tintR = 1.f, float tintG = 1.f, float tintB = 1.f) override;

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

    // ── Diagnostics ───────────────────────────────────────────────────
    uint32_t    getValidationErrorCount()   const override;
    uint32_t    getValidationWarningCount() const override;
    float       getGpuFrameTimeMs()         const override;
    std::string getGpuDeviceName()          const override;

    GLFWwindow* getWindow() const { return m_window; }

private:
    friend void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    void initVulkan();
    void createSwapchain();
    void destroySwapchain();
    void createOffscreenImage();
    void destroyOffscreenImage();
    void createCommandPool();
    void createSyncObjects();
    void createTimestampQueries();
    void cleanupVulkan();

    void recreateSwapchain();
    void transitionImageLayout(VkCommandBuffer cmd, VkImage image,
                               VkImageLayout oldLayout, VkImageLayout newLayout);

    // ── Pipeline helpers ──────────────────────────────────────────────
    static std::vector<char> readSPVFile(const std::string& path);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createFlatPipeline();
    void createTexturedPipeline();
    void createTextPipeline();
    void createVertexColorPipelines();
    void createBlitPipeline();
    void createBlitQuadBuffer();
    void updateBlitDescriptorSet();

    // ── Texture helpers ──────────────────────────────────────────────
    void createTextureDescriptorResources();
    void cleanupTextureResources();
    VkCommandBuffer beginOneTimeCommands();
    void endOneTimeCommands(VkCommandBuffer cmd);
    TextureHandle loadNormalMapForDiffuse(const std::string& diffusePath,
                                         TextureHandle diffuseHandle);

    // ── Sprite/lighting resource helpers ──────────────────────────────
    void createLightingResources();
    void uploadLightingData();
    void createFlatNormalTexture();
    VkDescriptorSet getOrCreateTextureDescriptorSet(TextureHandle diffuse,
                                                     TextureHandle normal);

    // ── Font / glyph atlas helpers ───────────────────────────────────
    struct GlyphInfo {
        float u0 = 0, v0 = 0, u1 = 0, v1 = 0;
        int   width = 0, height = 0;
        int   bearingX = 0, bearingY = 0;
        int   advance = 0;
    };

    struct GlyphAtlas {
        VkImage       image      = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkImageView   view       = VK_NULL_HANDLE;
        VkSampler     sampler    = VK_NULL_HANDLE;
        int           atlasWidth  = 0;
        int           atlasHeight = 0;
        int           lineHeight  = 0;
        int           ascender    = 0;
        GlyphInfo        glyphs[128]; // ASCII 0-127; only 32-126 populated
        VkDescriptorSet  descriptorSet = VK_NULL_HANDLE;
    };

    struct FontData {
        FT_Face face = nullptr;
        std::map<unsigned int, GlyphAtlas> atlases;
    };

    void initFreeType();
    void cleanupFontResources();
    GlyphAtlas& getOrBuildAtlas(FontData& font, unsigned int size);

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

    // ── Off-screen render target ──────────────────────────────────────
    VkImage       m_offscreenImage      = VK_NULL_HANDLE;
    VmaAllocation m_offscreenAllocation = VK_NULL_HANDLE;
    VkImageView   m_offscreenView       = VK_NULL_HANDLE;
    VkSampler     m_offscreenSampler    = VK_NULL_HANDLE;

    // ── Flat-color pipeline ───────────────────────────────────────────
    VkDescriptorSetLayout m_flatDescriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout      m_flatPipelineLayout      = VK_NULL_HANDLE;
    VkPipeline            m_flatPipeline            = VK_NULL_HANDLE;

    // ── Textured sprite pipeline ──────────────────────────────────────
    VkPipelineLayout m_texturedPipelineLayout = VK_NULL_HANDLE;
    VkPipeline       m_texturedPipeline       = VK_NULL_HANDLE;

    // ── Text pipeline ─────────────────────────────────────────────────
    VkDescriptorSetLayout m_textDescriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout      m_textPipelineLayout      = VK_NULL_HANDLE;
    VkPipeline            m_textPipeline            = VK_NULL_HANDLE;
    VkDescriptorPool      m_textDescriptorPool      = VK_NULL_HANDLE;

    // ── Vertex-color pipelines ────────────────────────────────────────
    VkPipelineLayout m_vertColorPipelineLayout     = VK_NULL_HANDLE;
    VkPipeline       m_vertColorTriListPipeline    = VK_NULL_HANDLE;
    VkPipeline       m_vertColorTriStripPipeline   = VK_NULL_HANDLE;
    VkPipeline       m_vertColorLineListPipeline   = VK_NULL_HANDLE;

    // ── Blit pipeline ─────────────────────────────────────────────────
    VkDescriptorSetLayout m_blitDescriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout      m_blitPipelineLayout      = VK_NULL_HANDLE;
    VkPipeline            m_blitPipeline             = VK_NULL_HANDLE;
    VkDescriptorPool      m_blitDescriptorPool       = VK_NULL_HANDLE;
    VkDescriptorSet       m_blitDescriptorSet        = VK_NULL_HANDLE;

    // ── Fullscreen blit quad vertex buffer (NDC pos + texcoord) ───────
    VkBuffer      m_blitQuadBuffer     = VK_NULL_HANDLE;
    VmaAllocation m_blitQuadAllocation = VK_NULL_HANDLE;
    static constexpr uint32_t BLIT_QUAD_VERTEX_COUNT = 6;

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
    bool     m_worldPass         = false;
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
    std::unordered_map<TextureHandle, TextureHandle>     m_normalMaps;
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

    // ── Per-frame lighting UBOs ──────────────────────────────────────
    static constexpr uint32_t MAX_LIGHTS = 32;

    struct FrameLightingUBO {
        VkBuffer      buffer     = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        void*         mapped     = nullptr;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    };
    std::array<FrameLightingUBO, MAX_FRAMES_IN_FLIGHT> m_lightingUBOs{};
    VkDescriptorPool m_lightingDescriptorPool = VK_NULL_HANDLE;

    // CPU-side lighting state
    std::vector<Light> m_lights;
    glm::vec3          m_ambientColor{0.f, 0.f, 0.f};

    // ── Default flat normal texture ──────────────────────────────────
    TextureHandle m_flatNormalTexture = 0;

    // ── FreeType / font system ───────────────────────────────────────
    FT_Library m_ftLib = nullptr;
    std::unordered_map<FontHandle, FontData>  m_fonts;
    std::unordered_map<std::string, FontHandle> m_fontPaths;
    FontHandle m_nextFontHandle = 1;

    // ── GPU timestamp queries (frame profiling) ──────────────────────
    VkQueryPool m_timestampQueryPool = VK_NULL_HANDLE;
    float       m_timestampPeriod    = 0.f;   // nanoseconds per tick
    float       m_gpuFrameTimeMs     = 0.f;   // last measured GPU frame time
    bool        m_timestampSupported = false;
    static constexpr uint32_t TIMESTAMPS_PER_FRAME = 2; // begin + end

    // ── GPU device name ──────────────────────────────────────────────
    std::string m_gpuDeviceName;
};
