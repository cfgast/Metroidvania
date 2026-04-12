#include "VulkanRenderer.h"
#include "../Input/InputSystem.h"

#include <VkBootstrap.h>

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
    createSwapchain();

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

    VkDependencyInfo depInfo{};
    depInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers    = &barrier;

    vkCmdPipelineBarrier2(cmd, &depInfo);
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
    createSwapchain();
    createCommandPool();
    createSyncObjects();

    m_input = std::make_unique<VulkanInput>(m_window);

    std::cout << "[Vulkan] Renderer initialized successfully\n";
}

VulkanRenderer::~VulkanRenderer()
{
    m_input.reset();

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

void VulkanRenderer::display()
{
    // 1. Wait for this frame's in-flight fence
    vkWaitForFences(m_device, 1, &m_inFlight[m_currentFrame], VK_TRUE, UINT64_MAX);

    // 2. Acquire next swap chain image
    uint32_t imageIndex = 0;
    VkResult acquireResult = vkAcquireNextImageKHR(
        m_device, m_swapchain, UINT64_MAX,
        m_imageAvailable[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
        return;
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
    transitionImageLayout(cmd, m_swapchainImages[imageIndex],
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // 6. Begin dynamic rendering
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView   = m_swapchainImageViews[imageIndex];
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

    // (Future tasks will insert draw commands here)

    // 7. End dynamic rendering
    vkCmdEndRendering(cmd);

    // 8. Transition image for presentation: COLOR_ATTACHMENT_OPTIMAL → PRESENT_SRC
    transitionImageLayout(cmd, m_swapchainImages[imageIndex],
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    vkEndCommandBuffer(cmd);

    // 9. Submit command buffer
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

    // 10. Present
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &m_renderFinished[m_currentFrame];
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &m_swapchain;
    presentInfo.pImageIndices      = &imageIndex;

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

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

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
