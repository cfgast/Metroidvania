#include "VulkanRenderer.h"
#include "../Input/InputSystem.h"

#include <VkBootstrap.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

// ── Stub InputSystem for the Vulkan backend ──────────────────────────────────

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

// ── std140-aligned GPU lighting structs ──────────────────────────────────────

struct GpuLight {
    glm::vec2 position;     // offset  0   (8 bytes)
    float     _pad0[2];     // offset  8   (8 bytes padding → align vec3 to 16)
    glm::vec3 color;        // offset 16   (12 bytes)
    float     intensity;    // offset 28   (4 bytes)
    float     radius;       // offset 32   (4 bytes)
    float     z;            // offset 36   (4 bytes)
    glm::vec2 direction;    // offset 40   (8 bytes, aligned to 8)
    float     innerCone;    // offset 48   (4 bytes)
    float     outerCone;    // offset 52   (4 bytes)
    int32_t   type;         // offset 56   (4 bytes)
    float     _pad1;        // offset 60   (4 bytes → round to 64)
};
static_assert(sizeof(GpuLight) == 64, "GpuLight must be 64 bytes (std140)");

struct GpuLightingUBO {
    GpuLight  lights[32];       // offset    0  (2048 bytes)
    int32_t   numLights;        // offset 2048  (4 bytes)
    float     _pad2[3];         // offset 2052  (12 bytes → align vec3 to 16)
    glm::vec3 ambientColor;     // offset 2064  (12 bytes)
    float     _pad3;            // offset 2076  (4 bytes → round to 2080)
};
static_assert(sizeof(GpuLightingUBO) == 2080, "GpuLightingUBO must be 2080 bytes (std140)");

// ── GLFW resize callback ─────────────────────────────────────────────────────

static void framebufferResizeCallback(GLFWwindow* window, int /*width*/, int /*height*/)
{
    auto* renderer = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
    if (renderer)
        renderer->m_framebufferResized = true;
}

// ── Vulkan initialization ────────────────────────────────────────────────────

void VulkanRenderer::initVulkan()
{
    // Instance (validation layers enabled in debug builds)
    vkb::InstanceBuilder instanceBuilder;
    instanceBuilder.set_app_name("Metroidvania")
                   .require_api_version(1, 3, 0)
                   .set_engine_name("MetroidvaniaEngine");

#ifndef NDEBUG
    instanceBuilder.request_validation_layers()
                   .use_default_debug_messenger();
#endif

    auto instResult = instanceBuilder.build();
    if (!instResult)
        throw std::runtime_error("VulkanRenderer: failed to create VkInstance: "
                                 + instResult.error().message());

    vkb::Instance vkbInst = instResult.value();
    m_instance       = vkbInst.instance;
    m_debugMessenger = vkbInst.debug_messenger;

    // Surface via GLFW
    VkResult surfResult = glfwCreateWindowSurface(m_instance, m_window,
                                                  nullptr, &m_surface);
    if (surfResult != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to create window surface");

    // Physical device — prefer discrete GPU, require Vulkan 1.3
    vkb::PhysicalDeviceSelector physSelector(vkbInst);
    physSelector.set_minimum_version(1, 3)
                .set_surface(m_surface)
                .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete);

    // Enable dynamic rendering (core in Vulkan 1.3)
    VkPhysicalDeviceDynamicRenderingFeatures dynRenderingFeature{};
    dynRenderingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynRenderingFeature.dynamicRendering = VK_TRUE;

    // Enable synchronization2 (core in Vulkan 1.3)
    VkPhysicalDeviceSynchronization2Features sync2Feature{};
    sync2Feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    sync2Feature.synchronization2 = VK_TRUE;

    physSelector.add_required_extension_features(dynRenderingFeature);
    physSelector.add_required_extension_features(sync2Feature);

    auto physResult = physSelector.select();
    if (!physResult)
        throw std::runtime_error("VulkanRenderer: failed to select physical device: "
                                 + physResult.error().message());

    vkb::PhysicalDevice vkbPhysDev = physResult.value();
    m_physicalDevice = vkbPhysDev.physical_device;

    std::cout << "[Vulkan] Selected GPU: "
              << vkbPhysDev.properties.deviceName << "\n";

    // Logical device with graphics + present queues
    vkb::DeviceBuilder devBuilder(vkbPhysDev);
    auto devResult = devBuilder.build();
    if (!devResult)
        throw std::runtime_error("VulkanRenderer: failed to create logical device: "
                                 + devResult.error().message());

    vkb::Device vkbDevice = devResult.value();
    m_device = vkbDevice.device;

    auto gfxQueue = vkbDevice.get_queue(vkb::QueueType::graphics);
    if (!gfxQueue)
        throw std::runtime_error("VulkanRenderer: failed to get graphics queue");
    m_graphicsQueue       = gfxQueue.value();
    m_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    auto presQueue = vkbDevice.get_queue(vkb::QueueType::present);
    if (!presQueue)
        throw std::runtime_error("VulkanRenderer: failed to get present queue");
    m_presentQueue       = presQueue.value();
    m_presentQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::present).value();
}

void VulkanRenderer::createSwapchain()
{
    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);

    vkb::SwapchainBuilder swapBuilder(m_physicalDevice, m_device, m_surface);
    swapBuilder.set_desired_format({VK_FORMAT_B8G8R8A8_SRGB,
                                    VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
               .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
               .set_desired_extent(static_cast<uint32_t>(fbWidth),
                                   static_cast<uint32_t>(fbHeight))
               .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT);

    if (m_swapchain != VK_NULL_HANDLE)
        swapBuilder.set_old_swapchain(m_swapchain);

    auto swapResult = swapBuilder.build();
    if (!swapResult)
        throw std::runtime_error("VulkanRenderer: failed to create swap chain: "
                                 + swapResult.error().message());

    // Destroy old swap chain resources if recreating
    if (m_swapchain != VK_NULL_HANDLE)
    {
        for (auto view : m_swapchainImageViews)
            vkDestroyImageView(m_device, view, nullptr);
        m_swapchainImageViews.clear();
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    }

    vkb::Swapchain vkbSwap = swapResult.value();
    m_swapchain       = vkbSwap.swapchain;
    m_swapchainFormat = vkbSwap.image_format;
    m_swapchainExtent = vkbSwap.extent;
    m_swapchainImages = vkbSwap.get_images().value();
    m_swapchainImageViews = vkbSwap.get_image_views().value();

    std::cout << "[Vulkan] Swap chain created: "
              << m_swapchainExtent.width << "x" << m_swapchainExtent.height
              << " (" << m_swapchainImages.size() << " images)\n";
}

void VulkanRenderer::destroySwapchain()
{
    for (auto view : m_swapchainImageViews)
        vkDestroyImageView(m_device, view, nullptr);
    m_swapchainImageViews.clear();
    m_swapchainImages.clear();

    if (m_swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }
}

// ── Off-screen render target ─────────────────────────────────────────────────

void VulkanRenderer::createOffscreenImage()
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.format        = m_swapchainFormat;
    imageInfo.extent        = {m_swapchainExtent.width, m_swapchainExtent.height, 1};
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                              VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocCI{};
    allocCI.usage = VMA_MEMORY_USAGE_AUTO;
    allocCI.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    if (vmaCreateImage(m_allocator, &imageInfo, &allocCI,
                       &m_offscreenImage, &m_offscreenAllocation, nullptr) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to create off-screen image");

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = m_offscreenImage;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = m_swapchainFormat;
    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_offscreenView) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to create off-screen image view");

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter    = VK_FILTER_LINEAR;
    samplerInfo.minFilter    = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_offscreenSampler) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to create off-screen sampler");

    std::cout << "[Vulkan] Off-screen render target created: "
              << m_swapchainExtent.width << "x" << m_swapchainExtent.height << "\n";
}

void VulkanRenderer::destroyOffscreenImage()
{
    if (m_offscreenSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(m_device, m_offscreenSampler, nullptr);
        m_offscreenSampler = VK_NULL_HANDLE;
    }
    if (m_offscreenView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(m_device, m_offscreenView, nullptr);
        m_offscreenView = VK_NULL_HANDLE;
    }
    if (m_offscreenImage != VK_NULL_HANDLE)
    {
        vmaDestroyImage(m_allocator, m_offscreenImage, m_offscreenAllocation);
        m_offscreenImage      = VK_NULL_HANDLE;
        m_offscreenAllocation = VK_NULL_HANDLE;
    }
}

// ── Command pool ─────────────────────────────────────────────────────────────

void VulkanRenderer::createCommandPool()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_graphicsQueueFamily;

    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to create command pool");

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = m_commandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    if (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to allocate command buffers");
}

void VulkanRenderer::createSyncObjects()
{
    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // start signaled so first frame doesn't block

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        if (vkCreateSemaphore(m_device, &semInfo, nullptr, &m_imageAvailable[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device, &semInfo, nullptr, &m_renderFinished[i]) != VK_SUCCESS ||
            vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlight[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("VulkanRenderer: failed to create synchronization objects");
        }
    }
}

void VulkanRenderer::cleanupVulkan()
{
    if (m_device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(m_device);

        // ── Destroy texture resources ───────────────────────────────
        cleanupTextureResources();

        // ── Destroy off-screen render target ────────────────────────
        destroyOffscreenImage();

        // ── Destroy dynamic vertex buffers ───────────────────────────
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            if (m_dynamicBuffers[i].buffer != VK_NULL_HANDLE)
                vmaDestroyBuffer(m_allocator, m_dynamicBuffers[i].buffer,
                                 m_dynamicBuffers[i].allocation);
        }

        // ── Destroy quad vertex buffer ───────────────────────────────
        if (m_quadBuffer != VK_NULL_HANDLE)
            vmaDestroyBuffer(m_allocator, m_quadBuffer, m_quadAllocation);

        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            if (m_imageAvailable[i] != VK_NULL_HANDLE)
                vkDestroySemaphore(m_device, m_imageAvailable[i], nullptr);
            if (m_renderFinished[i] != VK_NULL_HANDLE)
                vkDestroySemaphore(m_device, m_renderFinished[i], nullptr);
            if (m_inFlight[i] != VK_NULL_HANDLE)
                vkDestroyFence(m_device, m_inFlight[i], nullptr);
        }

        if (m_commandPool != VK_NULL_HANDLE)
            vkDestroyCommandPool(m_device, m_commandPool, nullptr);

        if (m_flatPipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(m_device, m_flatPipeline, nullptr);
        if (m_flatPipelineLayout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(m_device, m_flatPipelineLayout, nullptr);
        if (m_flatDescriptorSetLayout != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(m_device, m_flatDescriptorSetLayout, nullptr);

        if (m_texturedPipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(m_device, m_texturedPipeline, nullptr);
        if (m_texturedPipelineLayout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(m_device, m_texturedPipelineLayout, nullptr);

        if (m_textPipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(m_device, m_textPipeline, nullptr);
        if (m_textPipelineLayout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(m_device, m_textPipelineLayout, nullptr);
        if (m_textDescriptorPool != VK_NULL_HANDLE)
            vkDestroyDescriptorPool(m_device, m_textDescriptorPool, nullptr);
        if (m_textDescriptorSetLayout != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(m_device, m_textDescriptorSetLayout, nullptr);

        if (m_vertColorTriListPipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(m_device, m_vertColorTriListPipeline, nullptr);
        if (m_vertColorTriStripPipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(m_device, m_vertColorTriStripPipeline, nullptr);
        if (m_vertColorLineListPipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(m_device, m_vertColorLineListPipeline, nullptr);
        if (m_vertColorPipelineLayout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(m_device, m_vertColorPipelineLayout, nullptr);

        if (m_lightingDescriptorPool != VK_NULL_HANDLE)
            vkDestroyDescriptorPool(m_device, m_lightingDescriptorPool, nullptr);
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            if (m_lightingUBOs[i].buffer != VK_NULL_HANDLE)
                vmaDestroyBuffer(m_allocator, m_lightingUBOs[i].buffer,
                                 m_lightingUBOs[i].allocation);
        }

        // ── Destroy VMA allocator ────────────────────────────────────
        if (m_allocator != VK_NULL_HANDLE)
        {
            vmaDestroyAllocator(m_allocator);
            m_allocator = VK_NULL_HANDLE;
        }

        destroySwapchain();
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }

    if (m_surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }

    if (m_instance != VK_NULL_HANDLE)
    {
#ifndef NDEBUG
        if (m_debugMessenger != VK_NULL_HANDLE)
        {
            vkb::destroy_debug_utils_messenger(m_instance, m_debugMessenger);
            m_debugMessenger = VK_NULL_HANDLE;
        }
#endif
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::recreateSwapchain()
{
    // Handle minimization — wait until window has a non-zero size
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(m_window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(m_device);
    destroyOffscreenImage();
    createSwapchain();
    createOffscreenImage();

    m_windowW = static_cast<float>(m_swapchainExtent.width);
    m_windowH = static_cast<float>(m_swapchainExtent.height);
}

void VulkanRenderer::transitionImageLayout(VkCommandBuffer cmd, VkImage image,
                                           VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier2 barrier{};
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.oldLayout           = oldLayout;
    barrier.newLayout           = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image               = image;
    barrier.subresourceRange    = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        barrier.srcStageMask  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        barrier.srcAccessMask = 0;
        barrier.dstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
    {
        barrier.srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstStageMask  = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
        barrier.dstAccessMask = 0;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    }

    VkDependencyInfo depInfo{};
    depInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers    = &barrier;

    vkCmdPipelineBarrier2(cmd, &depInfo);
}

// ── Pipeline helpers ─────────────────────────────────────────────────────────

std::vector<char> VulkanRenderer::readSPVFile(const std::string& path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("VulkanRenderer: failed to open SPIR-V file: " + path);

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
    return buffer;
}

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to create shader module");

    return shaderModule;
}

void VulkanRenderer::createFlatPipeline()
{
    // ── Descriptor set layout for lighting UBO (set 0, binding 0)
    VkDescriptorSetLayoutBinding lightingBinding{};
    lightingBinding.binding         = 0;
    lightingBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightingBinding.descriptorCount = 1;
    lightingBinding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
    setLayoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.bindingCount = 1;
    setLayoutInfo.pBindings    = &lightingBinding;

    if (vkCreateDescriptorSetLayout(m_device, &setLayoutInfo, nullptr,
                                    &m_flatDescriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to create flat descriptor set layout");

    // ── Push constant range: projection(mat4) + model(mat4) + color(vec4) = 144 bytes
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushRange.offset     = 0;
    pushRange.size       = 144; // 64 + 64 + 16

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount         = 1;
    layoutInfo.pSetLayouts            = &m_flatDescriptorSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges    = &pushRange;

    if (vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_flatPipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to create flat pipeline layout");

    // ── Load shader modules
    auto vertCode = readSPVFile("assets/shaders/flat.vert.spv");
    auto fragCode = readSPVFile("assets/shaders/flat.frag.spv");
    VkShaderModule vertModule = createShaderModule(vertCode);
    VkShaderModule fragModule = createShaderModule(fragCode);

    VkPipelineShaderStageCreateInfo shaderStages[2]{};
    shaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertModule;
    shaderStages[0].pName  = "main";

    shaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragModule;
    shaderStages[1].pName  = "main";

    // ── Vertex input: single vec2 position at location 0
    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding   = 0;
    bindingDesc.stride    = sizeof(float) * 2;
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrDesc{};
    attrDesc.binding  = 0;
    attrDesc.location = 0;
    attrDesc.format   = VK_FORMAT_R32G32_SFLOAT;
    attrDesc.offset   = 0;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount    = 1;
    vertexInputInfo.pVertexBindingDescriptions       = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount  = 1;
    vertexInputInfo.pVertexAttributeDescriptions     = &attrDesc;

    // ── Input assembly: triangle list
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // ── Dynamic viewport and scissor
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates    = dynamicStates;

    // ── Rasterizer: polygon fill, no culling (2D sprites face camera)
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = VK_CULL_MODE_NONE;
    rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;

    // ── Multisampling: disabled
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // ── Color blending: src-alpha / one-minus-src-alpha
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable         = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &colorBlendAttachment;

    // ── Dynamic rendering: specify color attachment format matching swap chain
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount    = 1;
    renderingInfo.pColorAttachmentFormats = &m_swapchainFormat;

    // ── Create the graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext               = &renderingInfo;
    pipelineInfo.stageCount          = 2;
    pipelineInfo.pStages             = shaderStages;
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = nullptr;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicState;
    pipelineInfo.layout              = m_flatPipelineLayout;
    pipelineInfo.renderPass          = VK_NULL_HANDLE; // dynamic rendering
    pipelineInfo.subpass             = 0;

    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &m_flatPipeline) != VK_SUCCESS)
    {
        vkDestroyShaderModule(m_device, vertModule, nullptr);
        vkDestroyShaderModule(m_device, fragModule, nullptr);
        throw std::runtime_error("VulkanRenderer: failed to create flat-color graphics pipeline");
    }

    // Shader modules can be destroyed after pipeline creation
    vkDestroyShaderModule(m_device, vertModule, nullptr);
    vkDestroyShaderModule(m_device, fragModule, nullptr);

    std::cout << "[Vulkan] Flat-color pipeline created\n";
}

void VulkanRenderer::createTexturedPipeline()
{
    // Pipeline layout: set 0 = lighting UBO, set 1 = texture samplers + flags
    VkDescriptorSetLayout setLayouts[] = {
        m_flatDescriptorSetLayout, m_textureDescriptorSetLayout
    };

    // Push constants: mat4 projection (64 bytes) + int hasNormalMap (4 bytes) = 68
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushRange.offset     = 0;
    pushRange.size       = 68;

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount         = 2;
    layoutInfo.pSetLayouts            = setLayouts;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges    = &pushRange;

    if (vkCreatePipelineLayout(m_device, &layoutInfo, nullptr,
                               &m_texturedPipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to create textured pipeline layout");

    // Load shader modules
    auto vertCode = readSPVFile("assets/shaders/textured.vert.spv");
    auto fragCode = readSPVFile("assets/shaders/textured.frag.spv");
    VkShaderModule vertModule = createShaderModule(vertCode);
    VkShaderModule fragModule = createShaderModule(fragCode);

    VkPipelineShaderStageCreateInfo shaderStages[2]{};
    shaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertModule;
    shaderStages[0].pName  = "main";

    shaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragModule;
    shaderStages[1].pName  = "main";

    // Vertex input: vec2 position (location 0) + vec2 texcoord (location 1)
    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding   = 0;
    bindingDesc.stride    = sizeof(float) * 4; // x, y, u, v
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrDescs[2]{};
    attrDescs[0].binding  = 0;
    attrDescs[0].location = 0;
    attrDescs[0].format   = VK_FORMAT_R32G32_SFLOAT;
    attrDescs[0].offset   = 0;

    attrDescs[1].binding  = 0;
    attrDescs[1].location = 1;
    attrDescs[1].format   = VK_FORMAT_R32G32_SFLOAT;
    attrDescs[1].offset   = sizeof(float) * 2;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount    = 1;
    vertexInputInfo.pVertexBindingDescriptions       = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount  = 2;
    vertexInputInfo.pVertexAttributeDescriptions     = attrDescs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates    = dynamicStates;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = VK_CULL_MODE_NONE;
    rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable         = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &colorBlendAttachment;

    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount    = 1;
    renderingInfo.pColorAttachmentFormats = &m_swapchainFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext               = &renderingInfo;
    pipelineInfo.stageCount          = 2;
    pipelineInfo.pStages             = shaderStages;
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = nullptr;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicState;
    pipelineInfo.layout              = m_texturedPipelineLayout;
    pipelineInfo.renderPass          = VK_NULL_HANDLE;
    pipelineInfo.subpass             = 0;

    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &m_texturedPipeline) != VK_SUCCESS)
    {
        vkDestroyShaderModule(m_device, vertModule, nullptr);
        vkDestroyShaderModule(m_device, fragModule, nullptr);
        throw std::runtime_error("VulkanRenderer: failed to create textured sprite graphics pipeline");
    }

    vkDestroyShaderModule(m_device, vertModule, nullptr);
    vkDestroyShaderModule(m_device, fragModule, nullptr);

    std::cout << "[Vulkan] Textured sprite pipeline created\n";
}

void VulkanRenderer::createTextPipeline()
{
    // ── Descriptor set layout for glyph atlas (set 1, binding 0) ─────
    VkDescriptorSetLayoutBinding atlasBinding{};
    atlasBinding.binding         = 0;
    atlasBinding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    atlasBinding.descriptorCount = 1;
    atlasBinding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
    setLayoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.bindingCount = 1;
    setLayoutInfo.pBindings    = &atlasBinding;

    if (vkCreateDescriptorSetLayout(m_device, &setLayoutInfo, nullptr,
                                    &m_textDescriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to create text descriptor set layout");

    // ── Descriptor pool for glyph atlas sets ─────────────────────────
    VkDescriptorPoolSize poolSize{};
    poolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 64;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets       = 64;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes    = &poolSize;

    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr,
                               &m_textDescriptorPool) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to create text descriptor pool");

    // ── Pipeline layout: set 0 = lighting UBO, set 1 = glyph atlas ──
    VkDescriptorSetLayout setLayouts[] = {
        m_flatDescriptorSetLayout, m_textDescriptorSetLayout
    };

    // Push constants: mat4 projection (64) + vec4 textColor (16) = 80 bytes
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushRange.offset     = 0;
    pushRange.size       = 80;

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount         = 2;
    layoutInfo.pSetLayouts            = setLayouts;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges    = &pushRange;

    if (vkCreatePipelineLayout(m_device, &layoutInfo, nullptr,
                               &m_textPipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to create text pipeline layout");

    // ── Load shader modules ──────────────────────────────────────────
    auto vertCode = readSPVFile("assets/shaders/text.vert.spv");
    auto fragCode = readSPVFile("assets/shaders/text.frag.spv");
    VkShaderModule vertModule = createShaderModule(vertCode);
    VkShaderModule fragModule = createShaderModule(fragCode);

    VkPipelineShaderStageCreateInfo shaderStages[2]{};
    shaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertModule;
    shaderStages[0].pName  = "main";

    shaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragModule;
    shaderStages[1].pName  = "main";

    // ── Vertex input: vec2 position (loc 0) + vec2 texcoord (loc 1) ─
    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding   = 0;
    bindingDesc.stride    = sizeof(float) * 4; // x, y, u, v
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrDescs[2]{};
    attrDescs[0].binding  = 0;
    attrDescs[0].location = 0;
    attrDescs[0].format   = VK_FORMAT_R32G32_SFLOAT;
    attrDescs[0].offset   = 0;

    attrDescs[1].binding  = 0;
    attrDescs[1].location = 1;
    attrDescs[1].format   = VK_FORMAT_R32G32_SFLOAT;
    attrDescs[1].offset   = sizeof(float) * 2;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount    = 1;
    vertexInputInfo.pVertexBindingDescriptions       = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount  = 2;
    vertexInputInfo.pVertexAttributeDescriptions     = attrDescs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates    = dynamicStates;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = VK_CULL_MODE_NONE;
    rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable         = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &colorBlendAttachment;

    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount    = 1;
    renderingInfo.pColorAttachmentFormats = &m_swapchainFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext               = &renderingInfo;
    pipelineInfo.stageCount          = 2;
    pipelineInfo.pStages             = shaderStages;
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = nullptr;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicState;
    pipelineInfo.layout              = m_textPipelineLayout;
    pipelineInfo.renderPass          = VK_NULL_HANDLE;
    pipelineInfo.subpass             = 0;

    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &m_textPipeline) != VK_SUCCESS)
    {
        vkDestroyShaderModule(m_device, vertModule, nullptr);
        vkDestroyShaderModule(m_device, fragModule, nullptr);
        throw std::runtime_error("VulkanRenderer: failed to create text graphics pipeline");
    }

    vkDestroyShaderModule(m_device, vertModule, nullptr);
    vkDestroyShaderModule(m_device, fragModule, nullptr);

    std::cout << "[Vulkan] Text pipeline created\n";
}

void VulkanRenderer::createVertexColorPipelines()
{
    // ── Pipeline layout: push constants only (mat4 projection = 64 bytes)
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushRange.offset     = 0;
    pushRange.size       = 64;

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount         = 0;
    layoutInfo.pSetLayouts            = nullptr;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges    = &pushRange;

    if (vkCreatePipelineLayout(m_device, &layoutInfo, nullptr,
                               &m_vertColorPipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to create vertex-color pipeline layout");

    // ── Load shader modules
    auto vertCode = readSPVFile("assets/shaders/vertcolor.vert.spv");
    auto fragCode = readSPVFile("assets/shaders/vertcolor.frag.spv");
    VkShaderModule vertModule = createShaderModule(vertCode);
    VkShaderModule fragModule = createShaderModule(fragCode);

    VkPipelineShaderStageCreateInfo shaderStages[2]{};
    shaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertModule;
    shaderStages[0].pName  = "main";

    shaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragModule;
    shaderStages[1].pName  = "main";

    // ── Vertex input: vec2 position (location 0) + vec4 color (location 1)
    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding   = 0;
    bindingDesc.stride    = sizeof(float) * 6; // x, y, r, g, b, a
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrDescs[2]{};
    attrDescs[0].binding  = 0;
    attrDescs[0].location = 0;
    attrDescs[0].format   = VK_FORMAT_R32G32_SFLOAT;
    attrDescs[0].offset   = 0;

    attrDescs[1].binding  = 0;
    attrDescs[1].location = 1;
    attrDescs[1].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrDescs[1].offset   = sizeof(float) * 2;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount    = 1;
    vertexInputInfo.pVertexBindingDescriptions       = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount  = 2;
    vertexInputInfo.pVertexAttributeDescriptions     = attrDescs;

    // ── Dynamic viewport and scissor
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates    = dynamicStates;

    // ── Rasterizer: polygon fill, no culling
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = VK_CULL_MODE_NONE;
    rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;

    // ── Multisampling: disabled
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // ── Color blending: src-alpha / one-minus-src-alpha
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable         = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &colorBlendAttachment;

    // ── Dynamic rendering: color attachment format matching swap chain
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount    = 1;
    renderingInfo.pColorAttachmentFormats = &m_swapchainFormat;

    // ── Base pipeline create info (shared across topologies)
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext               = &renderingInfo;
    pipelineInfo.stageCount          = 2;
    pipelineInfo.pStages             = shaderStages;
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = nullptr;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicState;
    pipelineInfo.layout              = m_vertColorPipelineLayout;
    pipelineInfo.renderPass          = VK_NULL_HANDLE;
    pipelineInfo.subpass             = 0;

    // ── Create triangle-list pipeline
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &m_vertColorTriListPipeline) != VK_SUCCESS)
    {
        vkDestroyShaderModule(m_device, vertModule, nullptr);
        vkDestroyShaderModule(m_device, fragModule, nullptr);
        throw std::runtime_error("VulkanRenderer: failed to create vertex-color tri-list pipeline");
    }

    // ── Create triangle-strip pipeline
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &m_vertColorTriStripPipeline) != VK_SUCCESS)
    {
        vkDestroyShaderModule(m_device, vertModule, nullptr);
        vkDestroyShaderModule(m_device, fragModule, nullptr);
        throw std::runtime_error("VulkanRenderer: failed to create vertex-color tri-strip pipeline");
    }

    // ── Create line-list pipeline
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &m_vertColorLineListPipeline) != VK_SUCCESS)
    {
        vkDestroyShaderModule(m_device, vertModule, nullptr);
        vkDestroyShaderModule(m_device, fragModule, nullptr);
        throw std::runtime_error("VulkanRenderer: failed to create vertex-color line-list pipeline");
    }

    vkDestroyShaderModule(m_device, vertModule, nullptr);
    vkDestroyShaderModule(m_device, fragModule, nullptr);

    std::cout << "[Vulkan] Vertex-color pipelines created (tri-list, tri-strip, line-list)\n";
}

// ── VMA / buffer helpers ─────────────────────────────────────────────────────

void VulkanRenderer::initAllocator()
{
    VmaVulkanFunctions vulkanFunctions{};
    vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    allocatorInfo.instance         = m_instance;
    allocatorInfo.physicalDevice   = m_physicalDevice;
    allocatorInfo.device           = m_device;
    allocatorInfo.pVulkanFunctions = &vulkanFunctions;

    if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to create VMA allocator");

    std::cout << "[Vulkan] VMA allocator created\n";
}

void VulkanRenderer::createQuadBuffer()
{
    // 6 vertices (two triangles) forming a unit quad [0,0] → [1,1]
    float quadVertices[] = {
        0.f, 0.f,
        1.f, 0.f,
        1.f, 1.f,

        0.f, 0.f,
        1.f, 1.f,
        0.f, 1.f,
    };
    VkDeviceSize bufferSize = sizeof(quadVertices);

    // ── Staging buffer (host-visible) ────────────────────────────────
    VkBufferCreateInfo stagingBufInfo{};
    stagingBufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufInfo.size  = bufferSize;
    stagingBufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    VkBuffer      stagingBuffer;
    VmaAllocation stagingAllocation;
    if (vmaCreateBuffer(m_allocator, &stagingBufInfo, &stagingAllocInfo,
                        &stagingBuffer, &stagingAllocation, nullptr) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to create quad staging buffer");

    void* data;
    vmaMapMemory(m_allocator, stagingAllocation, &data);
    std::memcpy(data, quadVertices, bufferSize);
    vmaUnmapMemory(m_allocator, stagingAllocation);

    // ── GPU-local vertex buffer ──────────────────────────────────────
    VkBufferCreateInfo bufInfo{};
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size  = bufferSize;
    bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    if (vmaCreateBuffer(m_allocator, &bufInfo, &allocInfo,
                        &m_quadBuffer, &m_quadAllocation, nullptr) != VK_SUCCESS)
    {
        vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);
        throw std::runtime_error("VulkanRenderer: failed to create quad vertex buffer");
    }

    // ── One-time copy via transient command buffer ───────────────────
    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool        = m_commandPool;
    cmdAllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.size = bufferSize;
    vkCmdCopyBuffer(cmd, stagingBuffer, m_quadBuffer, 1, &copyRegion);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &cmd;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);
    vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);

    std::cout << "[Vulkan] Unit quad buffer created (" << QUAD_VERTEX_COUNT
              << " vertices, " << bufferSize << " bytes)\n";
}

void VulkanRenderer::createDynamicBuffers()
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        VkBufferCreateInfo bufInfo{};
        bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufInfo.size  = DYNAMIC_BUFFER_SIZE;
        bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                          VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo allocationInfo{};
        if (vmaCreateBuffer(m_allocator, &bufInfo, &allocInfo,
                            &m_dynamicBuffers[i].buffer,
                            &m_dynamicBuffers[i].allocation,
                            &allocationInfo) != VK_SUCCESS)
            throw std::runtime_error("VulkanRenderer: failed to create dynamic buffer " +
                                     std::to_string(i));

        m_dynamicBuffers[i].mapped = allocationInfo.pMappedData;
        m_dynamicBuffers[i].offset = 0;
    }

    std::cout << "[Vulkan] Dynamic vertex buffers created ("
              << MAX_FRAMES_IN_FLIGHT << " x "
              << (DYNAMIC_BUFFER_SIZE / 1024) << " KB)\n";
}

VulkanRenderer::DynamicAllocation
VulkanRenderer::dynamicAllocate(VkDeviceSize size, VkDeviceSize alignment)
{
    auto& buf = m_dynamicBuffers[m_currentFrame];

    // Align the current offset
    VkDeviceSize alignedOffset = (buf.offset + alignment - 1) & ~(alignment - 1);

    if (alignedOffset + size > DYNAMIC_BUFFER_SIZE)
    {
        std::cerr << "[Vulkan] Dynamic buffer overflow — needed "
                  << size << " bytes, only "
                  << (DYNAMIC_BUFFER_SIZE - alignedOffset) << " remain\n";
        return {}; // data == nullptr signals failure
    }

    DynamicAllocation alloc;
    alloc.buffer = buf.buffer;
    alloc.offset = alignedOffset;
    alloc.data   = static_cast<uint8_t*>(buf.mapped) + alignedOffset;

    buf.offset = alignedOffset + size;
    return alloc;
}

// ── One-time command buffer helpers ──────────────────────────────────────────

VkCommandBuffer VulkanRenderer::beginOneTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = m_commandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(m_device, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    return cmd;
}

void VulkanRenderer::endOneTimeCommands(VkCommandBuffer cmd)
{
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &cmd;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);
}

// ── Texture descriptor resources ─────────────────────────────────────────────

void VulkanRenderer::createTextureDescriptorResources()
{
    // Descriptor set layout: binding 0 = diffuse, binding 1 = normal map
    VkDescriptorSetLayoutBinding bindings[2]{};

    bindings[0].binding         = 0;
    bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[1].binding         = 1;
    bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings    = bindings;

    if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr,
                                    &m_textureDescriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to create texture descriptor set layout");

    // Descriptor pool: 2 image samplers per set
    VkDescriptorPoolSize poolSize{};
    poolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = MAX_TEXTURE_DESCRIPTOR_SETS * 2;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets       = MAX_TEXTURE_DESCRIPTOR_SETS;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes    = &poolSize;

    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr,
                               &m_textureDescriptorPool) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to create texture descriptor pool");

    std::cout << "[Vulkan] Texture descriptor resources created (max "
              << MAX_TEXTURE_DESCRIPTOR_SETS << " sets)\n";
}

void VulkanRenderer::cleanupTextureResources()
{
    m_textureDescriptorCache.clear();
    m_normalMaps.clear();

    for (auto& [handle, tex] : m_textures)
    {
        if (tex.sampler != VK_NULL_HANDLE)
            vkDestroySampler(m_device, tex.sampler, nullptr);
        if (tex.view != VK_NULL_HANDLE)
            vkDestroyImageView(m_device, tex.view, nullptr);
        if (tex.image != VK_NULL_HANDLE)
            vmaDestroyImage(m_allocator, tex.image, tex.allocation);
    }
    m_textures.clear();
    m_texturePaths.clear();

    if (m_textureDescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_device, m_textureDescriptorPool, nullptr);
        m_textureDescriptorPool = VK_NULL_HANDLE;
    }
    if (m_textureDescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_device, m_textureDescriptorSetLayout, nullptr);
        m_textureDescriptorSetLayout = VK_NULL_HANDLE;
    }
}

// ── Lighting & sprite resource helpers ───────────────────────────────────────

void VulkanRenderer::createLightingResources()
{
    // Allocate a host-visible, host-coherent UBO per frame in flight
    static constexpr VkDeviceSize LIGHTING_UBO_SIZE = sizeof(GpuLightingUBO);

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        VkBufferCreateInfo bufInfo{};
        bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufInfo.size  = LIGHTING_UBO_SIZE;
        bufInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                          VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo allocationInfo{};
        if (vmaCreateBuffer(m_allocator, &bufInfo, &allocInfo,
                            &m_lightingUBOs[i].buffer,
                            &m_lightingUBOs[i].allocation,
                            &allocationInfo) != VK_SUCCESS)
            throw std::runtime_error("VulkanRenderer: failed to create lighting UBO");

        m_lightingUBOs[i].mapped = allocationInfo.pMappedData;

        // Zero out — no lights, black ambient
        std::memset(m_lightingUBOs[i].mapped, 0, LIGHTING_UBO_SIZE);
    }

    // Descriptor pool for per-frame lighting sets
    VkDescriptorPoolSize poolSize{};
    poolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets       = MAX_FRAMES_IN_FLIGHT;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes    = &poolSize;

    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr,
                               &m_lightingDescriptorPool) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to create lighting descriptor pool");

    // Allocate and write descriptor sets — one per frame in flight
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        VkDescriptorSetAllocateInfo setAllocInfo{};
        setAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        setAllocInfo.descriptorPool     = m_lightingDescriptorPool;
        setAllocInfo.descriptorSetCount = 1;
        setAllocInfo.pSetLayouts        = &m_flatDescriptorSetLayout;

        if (vkAllocateDescriptorSets(m_device, &setAllocInfo,
                                     &m_lightingUBOs[i].descriptorSet) != VK_SUCCESS)
            throw std::runtime_error("VulkanRenderer: failed to allocate lighting descriptor set");

        VkDescriptorBufferInfo lightBufInfo{};
        lightBufInfo.buffer = m_lightingUBOs[i].buffer;
        lightBufInfo.offset = 0;
        lightBufInfo.range  = LIGHTING_UBO_SIZE;

        VkWriteDescriptorSet write{};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet          = m_lightingUBOs[i].descriptorSet;
        write.dstBinding      = 0;
        write.descriptorCount = 1;
        write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo     = &lightBufInfo;

        vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
    }

    std::cout << "[Vulkan] Lighting resources created (per-frame UBOs)\n";
}

void VulkanRenderer::createFlatNormalTexture()
{
    // 1×1 pixel: RGBA (128, 128, 255, 255) → flat normal (0, 0, 1)
    unsigned char pixel[] = { 128, 128, 255, 255 };

    VulkanTextureInfo texInfo{};
    texInfo.width  = 1;
    texInfo.height = 1;
    VkDeviceSize imageSize = 4;

    // Staging buffer
    VkBufferCreateInfo stagingBufInfo{};
    stagingBufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufInfo.size  = imageSize;
    stagingBufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    VkBuffer      stagingBuffer;
    VmaAllocation stagingAllocation;
    if (vmaCreateBuffer(m_allocator, &stagingBufInfo, &stagingAllocInfo,
                        &stagingBuffer, &stagingAllocation, nullptr) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to create flat normal staging buffer");

    void* data;
    vmaMapMemory(m_allocator, stagingAllocation, &data);
    std::memcpy(data, pixel, imageSize);
    vmaUnmapMemory(m_allocator, stagingAllocation);

    // VkImage
    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.format        = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.extent        = {1, 1, 1};
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imgAllocInfo{};
    imgAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    if (vmaCreateImage(m_allocator, &imageInfo, &imgAllocInfo,
                       &texInfo.image, &texInfo.allocation, nullptr) != VK_SUCCESS)
    {
        vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);
        throw std::runtime_error("VulkanRenderer: failed to create flat normal VkImage");
    }

    // Upload via one-time commands
    VkCommandBuffer cmd = beginOneTimeCommands();

    VkImageMemoryBarrier2 toTransfer{};
    toTransfer.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    toTransfer.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    toTransfer.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.image               = texInfo.image;
    toTransfer.subresourceRange    = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    toTransfer.srcStageMask        = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    toTransfer.srcAccessMask       = 0;
    toTransfer.dstStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    toTransfer.dstAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT;

    VkDependencyInfo dep1{};
    dep1.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep1.imageMemoryBarrierCount = 1;
    dep1.pImageMemoryBarriers    = &toTransfer;
    vkCmdPipelineBarrier2(cmd, &dep1);

    VkBufferImageCopy region{};
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent      = {1, 1, 1};
    vkCmdCopyBufferToImage(cmd, stagingBuffer, texInfo.image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    VkImageMemoryBarrier2 toShader{};
    toShader.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    toShader.oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toShader.newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toShader.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toShader.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toShader.image               = texInfo.image;
    toShader.subresourceRange    = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    toShader.srcStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    toShader.srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    toShader.dstStageMask        = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    toShader.dstAccessMask       = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;

    VkDependencyInfo dep2{};
    dep2.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep2.imageMemoryBarrierCount = 1;
    dep2.pImageMemoryBarriers    = &toShader;
    vkCmdPipelineBarrier2(cmd, &dep2);

    endOneTimeCommands(cmd);
    vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);

    // Image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = texInfo.image;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    if (vkCreateImageView(m_device, &viewInfo, nullptr, &texInfo.view) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to create flat normal image view");

    // Sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter               = VK_FILTER_NEAREST;
    samplerInfo.minFilter               = VK_FILTER_NEAREST;
    samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable        = VK_FALSE;
    samplerInfo.maxAnisotropy           = 1.0f;
    samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable           = VK_FALSE;
    samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &texInfo.sampler) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to create flat normal sampler");

    m_flatNormalTexture = m_nextTexHandle++;
    m_textures[m_flatNormalTexture] = texInfo;

    std::cout << "[Vulkan] Flat normal texture created (handle=" << m_flatNormalTexture << ")\n";
}

VkDescriptorSet VulkanRenderer::getOrCreateTextureDescriptorSet(
    TextureHandle diffuse, TextureHandle normal)
{
    DescriptorSetKey key{diffuse, normal};
    auto it = m_textureDescriptorCache.find(key);
    if (it != m_textureDescriptorCache.end())
        return it->second;

    // Allocate new descriptor set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = m_textureDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &m_textureDescriptorSetLayout;

    VkDescriptorSet set;
    if (vkAllocateDescriptorSets(m_device, &allocInfo, &set) != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to allocate texture descriptor set\n";
        return VK_NULL_HANDLE;
    }

    auto& diffuseInfo = m_textures[diffuse];
    auto& normalInfo  = m_textures[normal];

    VkDescriptorImageInfo diffuseImgInfo{};
    diffuseImgInfo.sampler     = diffuseInfo.sampler;
    diffuseImgInfo.imageView   = diffuseInfo.view;
    diffuseImgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorImageInfo normalImgInfo{};
    normalImgInfo.sampler     = normalInfo.sampler;
    normalImgInfo.imageView   = normalInfo.view;
    normalImgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet writes[2]{};

    writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet          = set;
    writes[0].dstBinding      = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].pImageInfo      = &diffuseImgInfo;

    writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet          = set;
    writes[1].dstBinding      = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo      = &normalImgInfo;

    vkUpdateDescriptorSets(m_device, 2, writes, 0, nullptr);

    m_textureDescriptorCache[key] = set;
    return set;
}

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
    glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);

    // Initialize Vulkan instance, device, swap chain, commands, and sync
    initVulkan();
    initAllocator();
    createSwapchain();
    createOffscreenImage();
    createCommandPool();
    createSyncObjects();
    createFlatPipeline();
    createQuadBuffer();
    createDynamicBuffers();
    createTextureDescriptorResources();
    createLightingResources();
    createFlatNormalTexture();
    createTexturedPipeline();
    createTextPipeline();
    createVertexColorPipelines();

    initFreeType();

    m_input = std::make_unique<VulkanInput>(m_window);

    // Initialize default projection (screen-space, Y-down)
    m_projection = glm::ortho(0.f, m_windowW, m_windowH, 0.f, -1.f, 1.f);

    std::cout << "[Vulkan] Renderer initialized successfully\n";
}

VulkanRenderer::~VulkanRenderer()
{
    m_input.reset();

    cleanupFontResources();
    cleanupVulkan();

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

void VulkanRenderer::clear(float r, float g, float b, float a)
{
    m_clearColor = {{r, g, b, a}};
}

void VulkanRenderer::ensureFrameStarted()
{
    if (m_frameStarted)
        return;
    m_frameStarted = true;

    // 1. Wait for this frame's in-flight fence
    vkWaitForFences(m_device, 1, &m_inFlight[m_currentFrame], VK_TRUE, UINT64_MAX);

    // Reset dynamic buffer for this frame (GPU is done with it)
    m_dynamicBuffers[m_currentFrame].offset = 0;

    // 2. Acquire next swap chain image
    VkResult acquireResult = vkAcquireNextImageKHR(
        m_device, m_swapchain, UINT64_MAX,
        m_imageAvailable[m_currentFrame], VK_NULL_HANDLE, &m_currentImageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
        // Retry after recreation
        acquireResult = vkAcquireNextImageKHR(
            m_device, m_swapchain, UINT64_MAX,
            m_imageAvailable[m_currentFrame], VK_NULL_HANDLE, &m_currentImageIndex);
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
        throw std::runtime_error("VulkanRenderer: failed to acquire swap chain image");

    // 3. Reset fence and command buffer
    vkResetFences(m_device, 1, &m_inFlight[m_currentFrame]);

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
    vkResetCommandBuffer(cmd, 0);

    // 4. Begin command buffer recording
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // 5. Transition swap chain image: UNDEFINED → COLOR_ATTACHMENT_OPTIMAL
    transitionImageLayout(cmd, m_swapchainImages[m_currentImageIndex],
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // 6. Begin dynamic rendering to swap chain (skip if world pass —
    //    beginFrame() will start rendering to the off-screen image instead)
    if (!m_worldPass)
    {
        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView   = m_swapchainImageViews[m_currentImageIndex];
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue.color = m_clearColor;

        VkRenderingInfo renderInfo{};
        renderInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderInfo.renderArea.offset    = {0, 0};
        renderInfo.renderArea.extent    = m_swapchainExtent;
        renderInfo.layerCount           = 1;
        renderInfo.colorAttachmentCount = 1;
        renderInfo.pColorAttachments    = &colorAttachment;

        vkCmdBeginRendering(cmd, &renderInfo);
    }
}

void VulkanRenderer::display()
{
    ensureFrameStarted();
    m_frameStarted = false;

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];

    // End whatever dynamic rendering pass is active
    vkCmdEndRendering(cmd);

    // Transition swap chain image for presentation
    transitionImageLayout(cmd, m_swapchainImages[m_currentImageIndex],
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    vkEndCommandBuffer(cmd);

    // Submit command buffer
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = &m_imageAvailable[m_currentFrame];
    submitInfo.pWaitDstStageMask    = &waitStage;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &m_renderFinished[m_currentFrame];

    if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlight[m_currentFrame]) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to submit command buffer");

    // Present
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &m_renderFinished[m_currentFrame];
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &m_swapchain;
    presentInfo.pImageIndices      = &m_currentImageIndex;

    VkResult presentResult = vkQueuePresentKHR(m_presentQueue, &presentInfo);

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR ||
        presentResult == VK_SUBOPTIMAL_KHR || m_framebufferResized)
    {
        m_framebufferResized = false;
        recreateSwapchain();
    }
    else if (presentResult != VK_SUCCESS)
    {
        throw std::runtime_error("VulkanRenderer: failed to present swap chain image");
    }

    m_worldPass = false;
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

// ── Frame / lighting pass ────────────────────────────────────────────────────

void VulkanRenderer::beginFrame()
{
    m_worldPass = true;
    ensureFrameStarted();

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];

    // Transition off-screen image to color attachment
    transitionImageLayout(cmd, m_offscreenImage,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // Begin dynamic rendering targeting the off-screen image
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView   = m_offscreenView;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = m_clearColor;

    VkRenderingInfo renderInfo{};
    renderInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderInfo.renderArea.offset    = {0, 0};
    renderInfo.renderArea.extent    = m_swapchainExtent;
    renderInfo.layerCount           = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments    = &colorAttachment;

    vkCmdBeginRendering(cmd, &renderInfo);

    // Set viewport and scissor to off-screen image dimensions
    VkViewport viewport{};
    viewport.x        = 0.f;
    viewport.y        = 0.f;
    viewport.width    = static_cast<float>(m_swapchainExtent.width);
    viewport.height   = static_cast<float>(m_swapchainExtent.height);
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapchainExtent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}
void VulkanRenderer::endFrame() {}

void VulkanRenderer::addLight(const Light& light)
{
    if (m_lights.size() < MAX_LIGHTS)
        m_lights.push_back(light);
}

void VulkanRenderer::clearLights()
{
    m_lights.clear();
}

void VulkanRenderer::setAmbientColor(float r, float g, float b)
{
    m_ambientColor = glm::vec3(r, g, b);
}

void VulkanRenderer::uploadLightingData()
{
    auto& frame = m_lightingUBOs[m_currentFrame];

    GpuLightingUBO ubo{};

    if (m_worldPass)
    {
        ubo.ambientColor = m_ambientColor;
        ubo.numLights    = static_cast<int32_t>(m_lights.size());

        for (size_t i = 0; i < m_lights.size() && i < MAX_LIGHTS; ++i)
        {
            const Light& src = m_lights[i];
            GpuLight&    dst = ubo.lights[i];
            dst.position  = src.position;
            dst._pad0[0]  = 0.f;
            dst._pad0[1]  = 0.f;
            dst.color     = src.color;
            dst.intensity = src.intensity;
            dst.radius    = src.radius;
            dst.z         = src.z;
            dst.direction = src.direction;
            dst.innerCone = src.innerCone;
            dst.outerCone = src.outerCone;
            dst.type      = static_cast<int32_t>(src.type);
            dst._pad1     = 0.f;
        }
    }
    else
    {
        // UI rendering: full brightness, no dynamic lights
        ubo.ambientColor = glm::vec3(1.f, 1.f, 1.f);
        ubo.numLights    = 0;
    }

    std::memcpy(frame.mapped, &ubo, sizeof(GpuLightingUBO));
}

// ── View / camera ────────────────────────────────────────────────────────────

void VulkanRenderer::setView(float centerX, float centerY,
                             float width, float height)
{
    float halfW = width  * 0.5f;
    float halfH = height * 0.5f;
    // Top-left origin: top = centerY - halfH, bottom = centerY + halfH
    m_projection = glm::ortho(centerX - halfW, centerX + halfW,
                              centerY + halfH, centerY - halfH,
                              -1.f, 1.f);
}

void VulkanRenderer::resetView()
{
    // Top-left origin (Y-down) for 2D screen-space drawing
    m_projection = glm::ortho(0.f, m_windowW, m_windowH, 0.f, -1.f, 1.f);
}

void VulkanRenderer::getWindowSize(float& w, float& h) const
{
    w = m_windowW;
    h = m_windowH;
}

// ── Primitives ───────────────────────────────────────────────────────────────

static constexpr int   kCircleSegments = 32;
static constexpr int   kCornerSegments = 8;
static constexpr float kPi = 3.14159265358979323846f;

// Helper: bind a vertex-color pipeline with viewport/scissor/push constants
static void bindVertColorPipeline(VkCommandBuffer cmd, VkPipeline pipeline,
                                  VkPipelineLayout layout,
                                  VkExtent2D extent, const glm::mat4& proj)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkViewport viewport{};
    viewport.x        = 0.f;
    viewport.y        = 0.f;
    viewport.width    = static_cast<float>(extent.width);
    viewport.height   = static_cast<float>(extent.height);
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(glm::mat4), &proj);
}

void VulkanRenderer::drawRect(float x, float y, float w, float h,
                              float r, float g, float b, float a)
{
    ensureFrameStarted();

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];

    // Upload lighting data and bind the lighting UBO (set 0)
    uploadLightingData();
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_flatPipeline);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_flatPipelineLayout, 0, 1,
                            &m_lightingUBOs[m_currentFrame].descriptorSet,
                            0, nullptr);

    // Set dynamic viewport
    VkViewport viewport{};
    viewport.x        = 0.f;
    viewport.y        = 0.f;
    viewport.width    = static_cast<float>(m_swapchainExtent.width);
    viewport.height   = static_cast<float>(m_swapchainExtent.height);
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    // Set dynamic scissor
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapchainExtent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Build push constants: projection(mat4) + model(mat4) + color(vec4)
    struct PushConstants {
        glm::mat4 projection;
        glm::mat4 model;
        glm::vec4 color;
    } pc{};

    pc.projection = m_projection;

    glm::mat4 model(1.f);
    model = glm::translate(model, glm::vec3(x, y, 0.f));
    model = glm::scale(model, glm::vec3(w, h, 1.f));
    pc.model = model;

    pc.color = glm::vec4(r, g, b, a);

    vkCmdPushConstants(cmd, m_flatPipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(pc), &pc);

    // Bind the unit-quad vertex buffer
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_quadBuffer, &offset);

    // Draw the quad (6 vertices = 2 triangles)
    vkCmdDraw(cmd, QUAD_VERTEX_COUNT, 1, 0, 0);
}

void VulkanRenderer::drawRectOutlined(float x, float y, float w, float h,
                                      float fillR, float fillG, float fillB, float fillA,
                                      float outR, float outG, float outB, float outA,
                                      float outlineThickness)
{
    // Draw the filled rectangle
    drawRect(x, y, w, h, fillR, fillG, fillB, fillA);

    if (outlineThickness <= 0.f)
        return;

    float t = outlineThickness;

    // Top edge
    drawRect(x - t, y - t, w + 2 * t, t, outR, outG, outB, outA);
    // Bottom edge
    drawRect(x - t, y + h, w + 2 * t, t, outR, outG, outB, outA);
    // Left edge
    drawRect(x - t, y, t, h, outR, outG, outB, outA);
    // Right edge
    drawRect(x + w, y, t, h, outR, outG, outB, outA);
}

void VulkanRenderer::drawCircle(float cx, float cy, float radius,
                                float r, float g, float b, float a,
                                float outR, float outG, float outB, float outA,
                                float outlineThickness)
{
    ensureFrameStarted();

    // ── Filled circle as triangle list (convert fan → individual triangles)
    std::vector<Vertex> verts;
    verts.reserve(kCircleSegments * 3);

    for (int i = 0; i < kCircleSegments; ++i)
    {
        float a0 = 2.f * kPi * static_cast<float>(i) / static_cast<float>(kCircleSegments);
        float a1 = 2.f * kPi * static_cast<float>(i + 1) / static_cast<float>(kCircleSegments);

        verts.push_back({cx, cy, r, g, b, a});
        verts.push_back({cx + radius * std::cos(a0), cy + radius * std::sin(a0), r, g, b, a});
        verts.push_back({cx + radius * std::cos(a1), cy + radius * std::sin(a1), r, g, b, a});
    }

    VkDeviceSize dataSize = verts.size() * sizeof(Vertex);
    auto alloc = dynamicAllocate(dataSize);
    if (!alloc.data) return;
    std::memcpy(alloc.data, verts.data(), dataSize);

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
    bindVertColorPipeline(cmd, m_vertColorTriListPipeline,
                          m_vertColorPipelineLayout, m_swapchainExtent,
                          m_projection);

    vkCmdBindVertexBuffers(cmd, 0, 1, &alloc.buffer, &alloc.offset);
    vkCmdDraw(cmd, static_cast<uint32_t>(verts.size()), 1, 0, 0);

    // ── Outline as triangle-strip ring
    if (outlineThickness > 0.f && outA > 0.f)
    {
        float innerR = radius;
        float outerR = radius + outlineThickness;

        std::vector<Vertex> ring;
        ring.reserve(2 * (kCircleSegments + 1));

        for (int i = 0; i <= kCircleSegments; ++i)
        {
            float angle = 2.f * kPi * static_cast<float>(i) / static_cast<float>(kCircleSegments);
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);

            ring.push_back({cx + innerR * cosA, cy + innerR * sinA, outR, outG, outB, outA});
            ring.push_back({cx + outerR * cosA, cy + outerR * sinA, outR, outG, outB, outA});
        }

        VkDeviceSize ringSize = ring.size() * sizeof(Vertex);
        auto ringAlloc = dynamicAllocate(ringSize);
        if (!ringAlloc.data) return;
        std::memcpy(ringAlloc.data, ring.data(), ringSize);

        bindVertColorPipeline(cmd, m_vertColorTriStripPipeline,
                              m_vertColorPipelineLayout, m_swapchainExtent,
                              m_projection);

        vkCmdBindVertexBuffers(cmd, 0, 1, &ringAlloc.buffer, &ringAlloc.offset);
        vkCmdDraw(cmd, static_cast<uint32_t>(ring.size()), 1, 0, 0);
    }
}

void VulkanRenderer::drawRoundedRect(float x, float y, float w, float h,
                                     float cornerRadius,
                                     float r, float g, float b, float a,
                                     float outR, float outG, float outB, float outA,
                                     float outlineThickness)
{
    ensureFrameStarted();

    // Clamp corner radius
    float maxR = std::min(w * 0.5f, h * 0.5f);
    float cr = std::min(cornerRadius, maxR);
    if (cr < 0.f) cr = 0.f;

    float cxr = x + w * 0.5f;
    float cyr = y + h * 0.5f;

    // Corner arc centers and start angles
    struct CornerArc { float cx, cy, startAngle; };
    CornerArc corners[4] = {
        { x + w - cr, y + cr,     -kPi * 0.5f },  // top-right
        { x + w - cr, y + h - cr,  0.f },           // bottom-right
        { x + cr,     y + h - cr,  kPi * 0.5f },    // bottom-left
        { x + cr,     y + cr,      kPi },            // top-left
    };

    // Build perimeter points (used for both fill and outline)
    std::vector<std::pair<float,float>> perim;
    perim.reserve(4 * (kCornerSegments + 1) + 1);

    for (int c = 0; c < 4; ++c)
    {
        float arcCx = corners[c].cx;
        float arcCy = corners[c].cy;
        float start = corners[c].startAngle;

        for (int i = 0; i <= kCornerSegments; ++i)
        {
            float angle = start + (kPi * 0.5f) * static_cast<float>(i) / static_cast<float>(kCornerSegments);
            perim.push_back({arcCx + cr * std::cos(angle),
                             arcCy + cr * std::sin(angle)});
        }
    }

    // ── Filled rounded rect as triangle list (fan → individual triangles)
    std::vector<Vertex> verts;
    int perimCount = static_cast<int>(perim.size());
    verts.reserve(perimCount * 3);

    for (int i = 0; i < perimCount; ++i)
    {
        int next = (i + 1) % perimCount;
        verts.push_back({cxr, cyr, r, g, b, a});
        verts.push_back({perim[i].first, perim[i].second, r, g, b, a});
        verts.push_back({perim[next].first, perim[next].second, r, g, b, a});
    }

    VkDeviceSize dataSize = verts.size() * sizeof(Vertex);
    auto alloc = dynamicAllocate(dataSize);
    if (!alloc.data) return;
    std::memcpy(alloc.data, verts.data(), dataSize);

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
    bindVertColorPipeline(cmd, m_vertColorTriListPipeline,
                          m_vertColorPipelineLayout, m_swapchainExtent,
                          m_projection);

    vkCmdBindVertexBuffers(cmd, 0, 1, &alloc.buffer, &alloc.offset);
    vkCmdDraw(cmd, static_cast<uint32_t>(verts.size()), 1, 0, 0);

    // ── Outline as triangle-strip ring
    if (outlineThickness > 0.f && outA > 0.f)
    {
        float ot = outlineThickness;

        std::vector<Vertex> ring;
        ring.reserve(2 * (perimCount + 1));

        for (int c = 0; c < 4; ++c)
        {
            float arcCx = corners[c].cx;
            float arcCy = corners[c].cy;
            float start = corners[c].startAngle;

            for (int i = 0; i <= kCornerSegments; ++i)
            {
                float angle = start + (kPi * 0.5f) * static_cast<float>(i) / static_cast<float>(kCornerSegments);
                float cosA = std::cos(angle);
                float sinA = std::sin(angle);

                ring.push_back({arcCx + cr * cosA, arcCy + cr * sinA,
                                outR, outG, outB, outA});
                ring.push_back({arcCx + (cr + ot) * cosA, arcCy + (cr + ot) * sinA,
                                outR, outG, outB, outA});
            }
        }

        // Close the ring
        ring.push_back(ring[0]);
        ring.push_back(ring[1]);

        VkDeviceSize ringSize = ring.size() * sizeof(Vertex);
        auto ringAlloc = dynamicAllocate(ringSize);
        if (!ringAlloc.data) return;
        std::memcpy(ringAlloc.data, ring.data(), ringSize);

        bindVertColorPipeline(cmd, m_vertColorTriStripPipeline,
                              m_vertColorPipelineLayout, m_swapchainExtent,
                              m_projection);

        vkCmdBindVertexBuffers(cmd, 0, 1, &ringAlloc.buffer, &ringAlloc.offset);
        vkCmdDraw(cmd, static_cast<uint32_t>(ring.size()), 1, 0, 0);
    }
}

// ── Textured sprites ─────────────────────────────────────────────────────────

Renderer::TextureHandle VulkanRenderer::loadTexture(const std::string& path)
{
    // 1. Check texture cache
    auto it = m_texturePaths.find(path);
    if (it != m_texturePaths.end())
        return it->second;

    TextureHandle handle = m_nextTexHandle++;
    VulkanTextureInfo texInfo{};

    // 2. Load image via stb_image — force RGBA
    stbi_set_flip_vertically_on_load(0);
    int channels = 0;
    unsigned char* pixels = stbi_load(path.c_str(), &texInfo.width, &texInfo.height,
                                      &channels, 4);

    bool useFallback = false;
    if (!pixels)
    {
        // 3. Magenta fallback (2×2)
        std::cerr << "VulkanRenderer: failed to load texture: " << path << "\n";
        useFallback = true;
        texInfo.width  = 2;
        texInfo.height = 2;
    }

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(texInfo.width) *
                             static_cast<VkDeviceSize>(texInfo.height) * 4;

    // Prepare pixel data — use magenta fallback if load failed
    unsigned char magenta[2 * 2 * 4] = {
        255, 0, 255, 255,   255, 0, 255, 255,
        255, 0, 255, 255,   255, 0, 255, 255,
    };
    unsigned char* srcPixels = useFallback ? magenta : pixels;

    // 5. Create staging buffer
    VkBufferCreateInfo stagingBufInfo{};
    stagingBufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufInfo.size  = imageSize;
    stagingBufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    VkBuffer      stagingBuffer;
    VmaAllocation stagingAllocation;
    if (vmaCreateBuffer(m_allocator, &stagingBufInfo, &stagingAllocInfo,
                        &stagingBuffer, &stagingAllocation, nullptr) != VK_SUCCESS)
    {
        if (pixels) stbi_image_free(pixels);
        throw std::runtime_error("VulkanRenderer: failed to create texture staging buffer");
    }

    void* data;
    vmaMapMemory(m_allocator, stagingAllocation, &data);
    std::memcpy(data, srcPixels, imageSize);
    vmaUnmapMemory(m_allocator, stagingAllocation);

    // 4. Create VkImage (R8G8B8A8_SRGB, SAMPLED | TRANSFER_DST) via VMA
    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.format        = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.extent.width  = static_cast<uint32_t>(texInfo.width);
    imageInfo.extent.height = static_cast<uint32_t>(texInfo.height);
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage         = VK_IMAGE_USAGE_SAMPLED_BIT |
                              VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imgAllocInfo{};
    imgAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    if (vmaCreateImage(m_allocator, &imageInfo, &imgAllocInfo,
                       &texInfo.image, &texInfo.allocation, nullptr) != VK_SUCCESS)
    {
        vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);
        if (pixels) stbi_image_free(pixels);
        throw std::runtime_error("VulkanRenderer: failed to create VkImage for texture");
    }

    // 6. Transition UNDEFINED → TRANSFER_DST, copy, then TRANSFER_DST → SHADER_READ_ONLY
    VkCommandBuffer cmd = beginOneTimeCommands();

    // Transition to TRANSFER_DST_OPTIMAL
    VkImageMemoryBarrier2 toTransferBarrier{};
    toTransferBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    toTransferBarrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    toTransferBarrier.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toTransferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransferBarrier.image               = texInfo.image;
    toTransferBarrier.subresourceRange    = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    toTransferBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    toTransferBarrier.srcAccessMask       = 0;
    toTransferBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    toTransferBarrier.dstAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT;

    VkDependencyInfo depInfo1{};
    depInfo1.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo1.imageMemoryBarrierCount = 1;
    depInfo1.pImageMemoryBarriers    = &toTransferBarrier;
    vkCmdPipelineBarrier2(cmd, &depInfo1);

    // Copy staging buffer to image
    VkBufferImageCopy region{};
    region.bufferOffset      = 0;
    region.bufferRowLength   = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource  = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageOffset       = {0, 0, 0};
    region.imageExtent       = {static_cast<uint32_t>(texInfo.width),
                                static_cast<uint32_t>(texInfo.height), 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, texInfo.image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition to SHADER_READ_ONLY_OPTIMAL
    VkImageMemoryBarrier2 toShaderBarrier{};
    toShaderBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    toShaderBarrier.oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toShaderBarrier.newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toShaderBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toShaderBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toShaderBarrier.image               = texInfo.image;
    toShaderBarrier.subresourceRange    = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    toShaderBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    toShaderBarrier.srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    toShaderBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    toShaderBarrier.dstAccessMask       = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;

    VkDependencyInfo depInfo2{};
    depInfo2.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo2.imageMemoryBarrierCount = 1;
    depInfo2.pImageMemoryBarriers    = &toShaderBarrier;
    vkCmdPipelineBarrier2(cmd, &depInfo2);

    endOneTimeCommands(cmd);

    // 10. Free staging buffer and stbi data
    vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);
    if (pixels) stbi_image_free(pixels);

    // 7. Create VkImageView
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = texInfo.image;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    if (vkCreateImageView(m_device, &viewInfo, nullptr, &texInfo.view) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to create texture image view");

    // 8. Create VkSampler: nearest filtering (pixel art), clamp-to-edge
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter               = VK_FILTER_NEAREST;
    samplerInfo.minFilter               = VK_FILTER_NEAREST;
    samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable        = VK_FALSE;
    samplerInfo.maxAnisotropy           = 1.0f;
    samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable           = VK_FALSE;
    samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipLodBias              = 0.f;
    samplerInfo.minLod                  = 0.f;
    samplerInfo.maxLod                  = 0.f;

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &texInfo.sampler) != VK_SUCCESS)
        throw std::runtime_error("VulkanRenderer: failed to create texture sampler");

    // 9. Store in cache
    m_textures[handle]   = texInfo;
    m_texturePaths[path] = handle;

    std::cout << "[Vulkan] Texture loaded: " << path << " ("
              << texInfo.width << "x" << texInfo.height << ") handle=" << handle << "\n";

    // 10. Auto-load companion normal map (_n.png)
    loadNormalMapForDiffuse(path, handle);

    return handle;
}

Renderer::TextureHandle VulkanRenderer::loadNormalMapForDiffuse(
    const std::string& diffusePath, TextureHandle diffuseHandle)
{
    auto it = m_normalMaps.find(diffuseHandle);
    if (it != m_normalMaps.end())
        return it->second;

    // Build the _n.png path: "assets/foo.png" → "assets/foo_n.png"
    std::string normalPath;
    auto dotPos = diffusePath.rfind('.');
    if (dotPos != std::string::npos)
        normalPath = diffusePath.substr(0, dotPos) + "_n" + diffusePath.substr(dotPos);
    else
        normalPath = diffusePath + "_n";

    stbi_set_flip_vertically_on_load(0);
    int w = 0, h = 0, channels = 0;
    unsigned char* pixels = stbi_load(normalPath.c_str(), &w, &h, &channels, 4);

    if (!pixels)
    {
        // No normal map found — store sentinel to avoid re-checking
        m_normalMaps[diffuseHandle] = 0;
        return 0;
    }

    TextureHandle handle = m_nextTexHandle++;
    VulkanTextureInfo texInfo{};
    texInfo.width  = w;
    texInfo.height = h;
    VkDeviceSize imageSize = static_cast<VkDeviceSize>(w) * static_cast<VkDeviceSize>(h) * 4;

    // Staging buffer
    VkBufferCreateInfo stagingBufInfo{};
    stagingBufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufInfo.size  = imageSize;
    stagingBufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    VkBuffer      stagingBuffer;
    VmaAllocation stagingAllocation;
    if (vmaCreateBuffer(m_allocator, &stagingBufInfo, &stagingAllocInfo,
                        &stagingBuffer, &stagingAllocation, nullptr) != VK_SUCCESS)
    {
        stbi_image_free(pixels);
        m_normalMaps[diffuseHandle] = 0;
        return 0;
    }

    void* data;
    vmaMapMemory(m_allocator, stagingAllocation, &data);
    std::memcpy(data, pixels, imageSize);
    vmaUnmapMemory(m_allocator, stagingAllocation);
    stbi_image_free(pixels);

    // Normal maps use UNORM (linear data, not sRGB)
    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.format        = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.extent.width  = static_cast<uint32_t>(w);
    imageInfo.extent.height = static_cast<uint32_t>(h);
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage         = VK_IMAGE_USAGE_SAMPLED_BIT |
                              VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imgAllocInfo{};
    imgAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    if (vmaCreateImage(m_allocator, &imageInfo, &imgAllocInfo,
                       &texInfo.image, &texInfo.allocation, nullptr) != VK_SUCCESS)
    {
        vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);
        m_normalMaps[diffuseHandle] = 0;
        return 0;
    }

    // Upload: UNDEFINED → TRANSFER_DST, copy, then → SHADER_READ_ONLY
    VkCommandBuffer cmd = beginOneTimeCommands();

    VkImageMemoryBarrier2 toTransfer{};
    toTransfer.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    toTransfer.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    toTransfer.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.image               = texInfo.image;
    toTransfer.subresourceRange    = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    toTransfer.srcStageMask        = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    toTransfer.srcAccessMask       = 0;
    toTransfer.dstStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    toTransfer.dstAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT;

    VkDependencyInfo dep1{};
    dep1.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep1.imageMemoryBarrierCount = 1;
    dep1.pImageMemoryBarriers    = &toTransfer;
    vkCmdPipelineBarrier2(cmd, &dep1);

    VkBufferImageCopy region{};
    region.bufferOffset      = 0;
    region.bufferRowLength   = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource  = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageOffset       = {0, 0, 0};
    region.imageExtent       = {static_cast<uint32_t>(w),
                                static_cast<uint32_t>(h), 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, texInfo.image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    VkImageMemoryBarrier2 toShader{};
    toShader.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    toShader.oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toShader.newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toShader.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toShader.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toShader.image               = texInfo.image;
    toShader.subresourceRange    = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    toShader.srcStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    toShader.srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    toShader.dstStageMask        = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    toShader.dstAccessMask       = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;

    VkDependencyInfo dep2{};
    dep2.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep2.imageMemoryBarrierCount = 1;
    dep2.pImageMemoryBarriers    = &toShader;
    vkCmdPipelineBarrier2(cmd, &dep2);

    endOneTimeCommands(cmd);
    vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);

    // Image view (UNORM format for normal map)
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = texInfo.image;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    if (vkCreateImageView(m_device, &viewInfo, nullptr, &texInfo.view) != VK_SUCCESS)
    {
        vmaDestroyImage(m_allocator, texInfo.image, texInfo.allocation);
        m_normalMaps[diffuseHandle] = 0;
        return 0;
    }

    // Sampler: nearest filtering for pixel art, clamp-to-edge
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter               = VK_FILTER_NEAREST;
    samplerInfo.minFilter               = VK_FILTER_NEAREST;
    samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable        = VK_FALSE;
    samplerInfo.maxAnisotropy           = 1.0f;
    samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable           = VK_FALSE;
    samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipLodBias              = 0.f;
    samplerInfo.minLod                  = 0.f;
    samplerInfo.maxLod                  = 0.f;

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &texInfo.sampler) != VK_SUCCESS)
    {
        vkDestroyImageView(m_device, texInfo.view, nullptr);
        vmaDestroyImage(m_allocator, texInfo.image, texInfo.allocation);
        m_normalMaps[diffuseHandle] = 0;
        return 0;
    }

    m_textures[handle] = texInfo;
    m_normalMaps[diffuseHandle] = handle;

    std::cout << "[Vulkan] Normal map loaded: " << normalPath << " ("
              << w << "x" << h << ") handle=" << handle << "\n";

    return handle;
}

void VulkanRenderer::drawSprite(TextureHandle tex, float x, float y,
                                int frameX, int frameY, int frameW, int frameH,
                                float originX, float originY)
{
    auto it = m_textures.find(tex);
    if (it == m_textures.end())
        return;

    ensureFrameStarted();

    const VulkanTextureInfo& info = it->second;
    float texW = static_cast<float>(info.width);
    float texH = static_cast<float>(info.height);

    // UV coordinates from the frame rect
    float u0 = static_cast<float>(frameX) / texW;
    float v0 = static_cast<float>(frameY) / texH;
    float u1 = static_cast<float>(frameX + frameW) / texW;
    float v1 = static_cast<float>(frameY + frameH) / texH;

    // Position quad with origin offset (top-left origin)
    float px = x - originX;
    float py = y - originY;
    float pw = static_cast<float>(frameW);
    float ph = static_cast<float>(frameH);

    // 6 vertices (2 triangles), each with (x, y, u, v)
    float verts[] = {
        px,      py,      u0, v0,
        px + pw, py,      u1, v0,
        px + pw, py + ph, u1, v1,

        px,      py,      u0, v0,
        px + pw, py + ph, u1, v1,
        px,      py + ph, u0, v1,
    };

    // Upload to dynamic buffer
    DynamicAllocation alloc = dynamicAllocate(sizeof(verts));
    if (!alloc.data)
        return;
    std::memcpy(alloc.data, verts, sizeof(verts));

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_texturedPipeline);

    VkViewport viewport{};
    viewport.x        = 0.f;
    viewport.y        = 0.f;
    viewport.width    = static_cast<float>(m_swapchainExtent.width);
    viewport.height   = static_cast<float>(m_swapchainExtent.height);
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapchainExtent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Push projection matrix + hasNormalMap flag
    struct {
        glm::mat4 projection;
        int32_t   hasNormalMap;
    } pushData;

    // Look up the normal map for this diffuse texture
    TextureHandle normalHandle = m_flatNormalTexture;
    int32_t hasNormal = 0;
    auto normalIt = m_normalMaps.find(tex);
    if (normalIt != m_normalMaps.end() && normalIt->second != 0)
    {
        normalHandle = normalIt->second;
        hasNormal = 1;
    }

    pushData.projection   = m_projection;
    pushData.hasNormalMap  = hasNormal;
    vkCmdPushConstants(cmd, m_texturedPipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(pushData), &pushData);

    // Upload lighting data and bind lighting UBO (set 0)
    uploadLightingData();
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_texturedPipelineLayout, 0, 1,
                            &m_lightingUBOs[m_currentFrame].descriptorSet,
                            0, nullptr);

    // Bind texture descriptor set (set 1): diffuse + normal map
    VkDescriptorSet texSet = getOrCreateTextureDescriptorSet(tex, normalHandle);
    if (texSet == VK_NULL_HANDLE)
        return;
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_texturedPipelineLayout, 1, 1,
                            &texSet, 0, nullptr);

    // Bind vertex buffer from dynamic allocation
    VkDeviceSize offset = alloc.offset;
    vkCmdBindVertexBuffers(cmd, 0, 1, &alloc.buffer, &offset);

    vkCmdDraw(cmd, 6, 1, 0, 0);
}

// ── Text ─────────────────────────────────────────────────────────────────────

void VulkanRenderer::initFreeType()
{
    if (FT_Init_FreeType(&m_ftLib))
    {
        std::cerr << "VulkanRenderer: failed to initialize FreeType\n";
        m_ftLib = nullptr;
    }
}

void VulkanRenderer::cleanupFontResources()
{
    if (m_device != VK_NULL_HANDLE)
        vkDeviceWaitIdle(m_device);

    for (auto& [handle, fontData] : m_fonts)
    {
        for (auto& [size, atlas] : fontData.atlases)
        {
            if (atlas.sampler != VK_NULL_HANDLE)
                vkDestroySampler(m_device, atlas.sampler, nullptr);
            if (atlas.view != VK_NULL_HANDLE)
                vkDestroyImageView(m_device, atlas.view, nullptr);
            if (atlas.image != VK_NULL_HANDLE)
                vmaDestroyImage(m_allocator, atlas.image, atlas.allocation);
        }
        if (fontData.face)
            FT_Done_Face(fontData.face);
    }
    m_fonts.clear();
    m_fontPaths.clear();

    if (m_ftLib)
    {
        FT_Done_FreeType(m_ftLib);
        m_ftLib = nullptr;
    }
}

VulkanRenderer::GlyphAtlas& VulkanRenderer::getOrBuildAtlas(FontData& font,
                                                             unsigned int size)
{
    auto it = font.atlases.find(size);
    if (it != font.atlases.end())
        return it->second;

    FT_Set_Pixel_Sizes(font.face, 0, size);

    // First pass: measure total atlas dimensions
    int totalWidth = 0;
    int maxHeight  = 0;
    for (int c = 32; c < 127; ++c)
    {
        if (FT_Load_Char(font.face, c, FT_LOAD_RENDER))
            continue;
        totalWidth += static_cast<int>(font.face->glyph->bitmap.width) + 1;
        int h = static_cast<int>(font.face->glyph->bitmap.rows);
        if (h > maxHeight) maxHeight = h;
    }

    GlyphAtlas atlas{};
    atlas.atlasWidth  = totalWidth > 0 ? totalWidth : 1;
    atlas.atlasHeight = maxHeight  > 0 ? maxHeight  : 1;
    atlas.ascender    = static_cast<int>(font.face->size->metrics.ascender >> 6);
    atlas.lineHeight  = static_cast<int>((font.face->size->metrics.ascender -
                                          font.face->size->metrics.descender) >> 6);

    VkDeviceSize bufferSize = static_cast<VkDeviceSize>(atlas.atlasWidth) *
                              static_cast<VkDeviceSize>(atlas.atlasHeight);

    // Create staging buffer
    VkBufferCreateInfo stagingBufInfo{};
    stagingBufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufInfo.size  = bufferSize;
    stagingBufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    VkBuffer      stagingBuffer;
    VmaAllocation stagingAllocation;
    if (vmaCreateBuffer(m_allocator, &stagingBufInfo, &stagingAllocInfo,
                        &stagingBuffer, &stagingAllocation, nullptr) != VK_SUCCESS)
    {
        std::cerr << "VulkanRenderer: failed to create glyph atlas staging buffer\n";
        auto [insertIt, _] = font.atlases.emplace(size, atlas);
        return insertIt->second;
    }

    // Map and clear to zero
    void* mapped = nullptr;
    vmaMapMemory(m_allocator, stagingAllocation, &mapped);
    std::memset(mapped, 0, bufferSize);

    // Second pass: rasterize each glyph into the staging buffer
    int xOffset = 0;
    for (int c = 32; c < 127; ++c)
    {
        if (FT_Load_Char(font.face, c, FT_LOAD_RENDER))
        {
            atlas.glyphs[c] = {};
            continue;
        }

        FT_GlyphSlot g = font.face->glyph;
        int bw = static_cast<int>(g->bitmap.width);
        int bh = static_cast<int>(g->bitmap.rows);

        if (bw > 0 && bh > 0)
        {
            // Copy glyph bitmap row-by-row into the staging buffer
            auto* dst = static_cast<unsigned char*>(mapped);
            for (int row = 0; row < bh; ++row)
            {
                std::memcpy(dst + row * atlas.atlasWidth + xOffset,
                            g->bitmap.buffer + row * g->bitmap.pitch,
                            static_cast<size_t>(bw));
            }
        }

        GlyphInfo& gi = atlas.glyphs[c];
        gi.width    = bw;
        gi.height   = bh;
        gi.bearingX = g->bitmap_left;
        gi.bearingY = g->bitmap_top;
        gi.advance  = static_cast<int>(g->advance.x);
        gi.u0 = static_cast<float>(xOffset) / atlas.atlasWidth;
        gi.v0 = 0.f;
        gi.u1 = static_cast<float>(xOffset + bw) / atlas.atlasWidth;
        gi.v1 = static_cast<float>(bh) / atlas.atlasHeight;

        xOffset += bw + 1;
    }

    vmaUnmapMemory(m_allocator, stagingAllocation);

    // Create VkImage (R8_UNORM, SAMPLED | TRANSFER_DST) via VMA
    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.format        = VK_FORMAT_R8_UNORM;
    imageInfo.extent.width  = static_cast<uint32_t>(atlas.atlasWidth);
    imageInfo.extent.height = static_cast<uint32_t>(atlas.atlasHeight);
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage         = VK_IMAGE_USAGE_SAMPLED_BIT |
                              VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imgAllocInfo{};
    imgAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    if (vmaCreateImage(m_allocator, &imageInfo, &imgAllocInfo,
                       &atlas.image, &atlas.allocation, nullptr) != VK_SUCCESS)
    {
        vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);
        std::cerr << "VulkanRenderer: failed to create glyph atlas VkImage\n";
        auto [insertIt, _] = font.atlases.emplace(size, atlas);
        return insertIt->second;
    }

    // Transition UNDEFINED → TRANSFER_DST, copy, then → SHADER_READ_ONLY
    VkCommandBuffer cmd = beginOneTimeCommands();

    VkImageMemoryBarrier2 toTransferBarrier{};
    toTransferBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    toTransferBarrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    toTransferBarrier.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toTransferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransferBarrier.image               = atlas.image;
    toTransferBarrier.subresourceRange    = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    toTransferBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    toTransferBarrier.srcAccessMask       = 0;
    toTransferBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    toTransferBarrier.dstAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT;

    VkDependencyInfo depInfo1{};
    depInfo1.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo1.imageMemoryBarrierCount = 1;
    depInfo1.pImageMemoryBarriers    = &toTransferBarrier;
    vkCmdPipelineBarrier2(cmd, &depInfo1);

    VkBufferImageCopy region{};
    region.bufferOffset      = 0;
    region.bufferRowLength   = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource  = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageOffset       = {0, 0, 0};
    region.imageExtent       = {static_cast<uint32_t>(atlas.atlasWidth),
                                static_cast<uint32_t>(atlas.atlasHeight), 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, atlas.image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    VkImageMemoryBarrier2 toShaderBarrier{};
    toShaderBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    toShaderBarrier.oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toShaderBarrier.newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toShaderBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toShaderBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toShaderBarrier.image               = atlas.image;
    toShaderBarrier.subresourceRange    = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    toShaderBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    toShaderBarrier.srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    toShaderBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    toShaderBarrier.dstAccessMask       = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;

    VkDependencyInfo depInfo2{};
    depInfo2.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo2.imageMemoryBarrierCount = 1;
    depInfo2.pImageMemoryBarriers    = &toShaderBarrier;
    vkCmdPipelineBarrier2(cmd, &depInfo2);

    endOneTimeCommands(cmd);
    vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);

    // Create VkImageView
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = atlas.image;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = VK_FORMAT_R8_UNORM;
    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    if (vkCreateImageView(m_device, &viewInfo, nullptr, &atlas.view) != VK_SUCCESS)
        std::cerr << "VulkanRenderer: failed to create glyph atlas image view\n";

    // Create VkSampler: linear filtering, clamp-to-edge
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter               = VK_FILTER_LINEAR;
    samplerInfo.minFilter               = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable        = VK_FALSE;
    samplerInfo.maxAnisotropy           = 1.0f;
    samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable           = VK_FALSE;
    samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipLodBias              = 0.f;
    samplerInfo.minLod                  = 0.f;
    samplerInfo.maxLod                  = 0.f;

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &atlas.sampler) != VK_SUCCESS)
        std::cerr << "VulkanRenderer: failed to create glyph atlas sampler\n";

    // Allocate and update descriptor set for this atlas
    if (m_textDescriptorPool != VK_NULL_HANDLE &&
        m_textDescriptorSetLayout != VK_NULL_HANDLE &&
        atlas.view != VK_NULL_HANDLE && atlas.sampler != VK_NULL_HANDLE)
    {
        VkDescriptorSetAllocateInfo dsAllocInfo{};
        dsAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dsAllocInfo.descriptorPool     = m_textDescriptorPool;
        dsAllocInfo.descriptorSetCount = 1;
        dsAllocInfo.pSetLayouts        = &m_textDescriptorSetLayout;

        if (vkAllocateDescriptorSets(m_device, &dsAllocInfo,
                                     &atlas.descriptorSet) == VK_SUCCESS)
        {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.sampler     = atlas.sampler;
            imageInfo.imageView   = atlas.view;
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkWriteDescriptorSet write{};
            write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet          = atlas.descriptorSet;
            write.dstBinding      = 0;
            write.dstArrayElement = 0;
            write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.descriptorCount = 1;
            write.pImageInfo      = &imageInfo;

            vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
        }
        else
        {
            std::cerr << "VulkanRenderer: failed to allocate text descriptor set\n";
        }
    }

    std::cout << "[Vulkan] Glyph atlas built: size=" << size
              << " (" << atlas.atlasWidth << "x" << atlas.atlasHeight << ")\n";

    auto [insertIt, _] = font.atlases.emplace(size, atlas);
    return insertIt->second;
}

Renderer::FontHandle VulkanRenderer::loadFont(const std::string& path)
{
    // Return cached handle if already loaded
    auto it = m_fontPaths.find(path);
    if (it != m_fontPaths.end())
        return it->second;

    if (!m_ftLib)
    {
        std::cerr << "VulkanRenderer: FreeType not initialized, cannot load font: "
                  << path << "\n";
        return 0;
    }

    FontData fontData;
    if (FT_New_Face(m_ftLib, path.c_str(), 0, &fontData.face))
    {
        std::cerr << "VulkanRenderer: failed to load font: " << path << "\n";
        return 0;
    }

    FontHandle handle = m_nextFontHandle++;
    m_fonts[handle] = std::move(fontData);
    m_fontPaths[path] = handle;

    std::cout << "[Vulkan] Font loaded: " << path << " handle=" << handle << "\n";
    return handle;
}

void VulkanRenderer::drawText(FontHandle font, const std::string& str,
                              float x, float y, unsigned int size,
                              float r, float g, float b, float a)
{
    auto fontIt = m_fonts.find(font);
    if (fontIt == m_fonts.end() || str.empty())
        return;

    GlyphAtlas& atlas = getOrBuildAtlas(fontIt->second, size);
    if (atlas.view == VK_NULL_HANDLE || atlas.descriptorSet == VK_NULL_HANDLE)
        return;

    // Build vertex data: 6 vertices per character (x, y, u, v)
    std::vector<float> vertices;
    vertices.reserve(str.size() * 6 * 4);

    float cursorX = x;
    float cursorY = y + static_cast<float>(atlas.ascender);

    for (char ch : str)
    {
        if (ch == '\n')
        {
            cursorX = x;
            cursorY += static_cast<float>(atlas.lineHeight);
            continue;
        }

        int c = static_cast<unsigned char>(ch);
        if (c < 32 || c >= 127)
            c = '?';

        const GlyphInfo& gi = atlas.glyphs[c];

        float xpos = cursorX + static_cast<float>(gi.bearingX);
        float ypos = cursorY - static_cast<float>(gi.bearingY);
        float w = static_cast<float>(gi.width);
        float h = static_cast<float>(gi.height);

        if (w > 0 && h > 0)
        {
            vertices.insert(vertices.end(), {xpos,     ypos,     gi.u0, gi.v0});
            vertices.insert(vertices.end(), {xpos + w, ypos,     gi.u1, gi.v0});
            vertices.insert(vertices.end(), {xpos + w, ypos + h, gi.u1, gi.v1});

            vertices.insert(vertices.end(), {xpos,     ypos,     gi.u0, gi.v0});
            vertices.insert(vertices.end(), {xpos + w, ypos + h, gi.u1, gi.v1});
            vertices.insert(vertices.end(), {xpos,     ypos + h, gi.u0, gi.v1});
        }

        cursorX += static_cast<float>(gi.advance >> 6);
    }

    if (vertices.empty())
        return;

    VkDeviceSize dataSize = static_cast<VkDeviceSize>(vertices.size() * sizeof(float));
    DynamicAllocation alloc = dynamicAllocate(dataSize);
    if (!alloc.data)
        return;
    std::memcpy(alloc.data, vertices.data(), dataSize);

    ensureFrameStarted();

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_textPipeline);

    VkViewport viewport{};
    viewport.x        = 0.f;
    viewport.y        = 0.f;
    viewport.width    = static_cast<float>(m_swapchainExtent.width);
    viewport.height   = static_cast<float>(m_swapchainExtent.height);
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapchainExtent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Push constants: projection (mat4) + text color (vec4) = 80 bytes
    struct TextPushConstants {
        glm::mat4 projection;
        glm::vec4 textColor;
    } pc{};
    pc.projection = m_projection;
    pc.textColor  = glm::vec4(r, g, b, a);

    vkCmdPushConstants(cmd, m_textPipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(pc), &pc);

    // Upload lighting data and bind lighting UBO (set 0)
    uploadLightingData();
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_textPipelineLayout, 0, 1,
                            &m_lightingUBOs[m_currentFrame].descriptorSet,
                            0, nullptr);

    // Bind glyph atlas descriptor set (set 1)
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_textPipelineLayout, 1, 1,
                            &atlas.descriptorSet, 0, nullptr);

    VkDeviceSize offset = alloc.offset;
    vkCmdBindVertexBuffers(cmd, 0, 1, &alloc.buffer, &offset);

    uint32_t vertexCount = static_cast<uint32_t>(vertices.size()) / 4;
    vkCmdDraw(cmd, vertexCount, 1, 0, 0);
}

void VulkanRenderer::measureText(FontHandle font, const std::string& str,
                                 unsigned int size,
                                 float& outWidth, float& outHeight)
{
    outWidth = outHeight = 0.f;

    auto fontIt = m_fonts.find(font);
    if (fontIt == m_fonts.end() || str.empty())
        return;

    GlyphAtlas& atlas = getOrBuildAtlas(fontIt->second, size);

    float maxLineWidth = 0.f;
    float curLineWidth = 0.f;
    int lineCount = 1;

    for (char ch : str)
    {
        if (ch == '\n')
        {
            if (curLineWidth > maxLineWidth)
                maxLineWidth = curLineWidth;
            curLineWidth = 0.f;
            ++lineCount;
            continue;
        }

        int c = static_cast<unsigned char>(ch);
        if (c < 32 || c >= 127)
            c = '?';

        curLineWidth += static_cast<float>(atlas.glyphs[c].advance >> 6);
    }

    if (curLineWidth > maxLineWidth)
        maxLineWidth = curLineWidth;

    outWidth  = maxLineWidth;
    outHeight = static_cast<float>(atlas.lineHeight * lineCount);
}

// ── Vertex-colored geometry ──────────────────────────────────────────────────

void VulkanRenderer::drawTriangleStrip(const std::vector<Vertex>& verts)
{
    if (verts.size() < 3) return;
    ensureFrameStarted();

    VkDeviceSize dataSize = verts.size() * sizeof(Vertex);
    auto alloc = dynamicAllocate(dataSize);
    if (!alloc.data) return;
    std::memcpy(alloc.data, verts.data(), dataSize);

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
    bindVertColorPipeline(cmd, m_vertColorTriStripPipeline,
                          m_vertColorPipelineLayout, m_swapchainExtent,
                          m_projection);

    vkCmdBindVertexBuffers(cmd, 0, 1, &alloc.buffer, &alloc.offset);
    vkCmdDraw(cmd, static_cast<uint32_t>(verts.size()), 1, 0, 0);
}

void VulkanRenderer::drawLines(const std::vector<Vertex>& verts)
{
    if (verts.size() < 2) return;
    ensureFrameStarted();

    VkDeviceSize dataSize = verts.size() * sizeof(Vertex);
    auto alloc = dynamicAllocate(dataSize);
    if (!alloc.data) return;
    std::memcpy(alloc.data, verts.data(), dataSize);

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
    bindVertColorPipeline(cmd, m_vertColorLineListPipeline,
                          m_vertColorPipelineLayout, m_swapchainExtent,
                          m_projection);

    vkCmdBindVertexBuffers(cmd, 0, 1, &alloc.buffer, &alloc.offset);
    vkCmdDraw(cmd, static_cast<uint32_t>(verts.size()), 1, 0, 0);
}
