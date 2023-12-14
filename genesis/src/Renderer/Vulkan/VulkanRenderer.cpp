#include "VulkanRenderer.h"

#include "Core/Logger.h"

namespace Genesis {
    VulkanRenderer::VulkanRenderer(std::shared_ptr<Window> window) : Renderer(window) {
        init(window);
    }

    VulkanRenderer::~VulkanRenderer() {
        shutdown();
    }

    void VulkanRenderer::init(std::shared_ptr<Window> window) {
        std::shared_ptr<GLFWWindow> glfwWindow = std::dynamic_pointer_cast<GLFWWindow>(window);
        if (!createInstance()) {
            return;
        }
        if (!setupDebugMessenger()) {
            return;
        }
        if (!m_vkSwapChain.createSurface(glfwWindow, m_vkInstance)) {
            return;
        }
        if (!m_vkDevice.pickPhysicalDevice(m_vkInstance, m_vkSwapChain.getSurface())) {
            return;
        }
        if (!m_vkDevice.createLogicalDevice(m_vkSwapChain.getSurface())) {
            return;
        }
        if (!m_vkSwapChain.createSwapChain(glfwWindow, m_vkDevice)) {
            return;
        }
        if (!m_vkSwapChain.createImageViews(m_vkDevice)) {
            return;
        }
        if (!m_vkPipeline.createRenderPass(m_vkDevice, m_vkSwapChain)) {
            return;
        }
        if (!m_vkPipeline.createGraphicsPipeline(m_vkDevice, m_vkSwapChain)) {
            return;
        }
        if (!m_vkSwapChain.createFramebuffers(m_vkDevice, m_vkPipeline)) {
            return;
        }
        if (!createCommandPool()) {
            return;
        }
        if (!createCommandBuffer()) {
            return;
        }
        if (!createSyncObjects()) {
            return;
        }
    }

    bool VulkanRenderer::drawFrame() {
        vkWaitForFences(m_vkDevice.getDevice(), 1, &m_vkInFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(m_vkDevice.getDevice(), 1, &m_vkInFlightFence);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(m_vkDevice.getDevice(), m_vkSwapChain.getSwapchain(), UINT64_MAX, m_vkImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

        vkResetCommandBuffer(m_vkCommandBuffer, 0);
        recordCommandBuffer(m_vkCommandBuffer, imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {m_vkImageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_vkCommandBuffer;

        VkSemaphore signalSemaphores[] = {m_vkRenderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(m_vkDevice.getGraphicsQueue(), 1, &submitInfo, m_vkInFlightFence) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to submit draw command buffer.");
            return false;
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {m_vkSwapChain.getSwapchain()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        presentInfo.pResults = nullptr;  // Optional

        vkQueuePresentKHR(m_vkDevice.getPresentQueue(), &presentInfo);

        return true;
    }

    void VulkanRenderer::shutdown() {
        vkDestroySemaphore(m_vkDevice.getDevice(), m_vkImageAvailableSemaphore, nullptr);
        vkDestroySemaphore(m_vkDevice.getDevice(), m_vkRenderFinishedSemaphore, nullptr);
        vkDestroyFence(m_vkDevice.getDevice(), m_vkInFlightFence, nullptr);

        vkDestroyCommandPool(m_vkDevice.getDevice(), m_vkCommandPool, nullptr);

        for (auto framebuffer : m_vkSwapChain.getSwapchainFramebuffers()) {
            vkDestroyFramebuffer(m_vkDevice.getDevice(), framebuffer, nullptr);
        }

        vkDestroyPipeline(m_vkDevice.getDevice(), m_vkPipeline.getGraphicsPipeline(), nullptr);
        vkDestroyPipelineLayout(m_vkDevice.getDevice(), m_vkPipeline.getPipelineLayout(), nullptr);
        vkDestroyRenderPass(m_vkDevice.getDevice(), m_vkPipeline.getRenderPass(), nullptr);

        for (auto imageView : m_vkSwapChain.getSwapchainImageViews()) {
            vkDestroyImageView(m_vkDevice.getDevice(), imageView, nullptr);
        }

        vkDestroySwapchainKHR(m_vkDevice.getDevice(), m_vkSwapChain.getSwapchain(), nullptr);
        vkDestroyDevice(m_vkDevice.getDevice(), nullptr);

        if (m_enableValidationLayers) {
            VulkanRenderer::destroyDebugUtilsMessengerEXT(m_vkInstance, m_debugMessenger, nullptr);
        }
        vkDestroySurfaceKHR(m_vkInstance, m_vkSwapChain.getSurface(), nullptr);
        vkDestroyInstance(m_vkInstance, nullptr);
    }

    void VulkanRenderer::waitForIdle() {
        vkDeviceWaitIdle(m_vkDevice.getDevice());
    }

    bool VulkanRenderer::createInstance() {
        if (m_enableValidationLayers && !checkValidationLayerSupport()) {
            GN_CORE_ERROR("Validation layers requested, but not available.");
            return false;
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";  // TODO: pass in program name from window
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Genesis";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        std::vector<const char*> extensions = getRequiredExtensions();
#if defined(GN_PLATFORM_MACOS)
        extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (m_enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
            createInfo.ppEnabledLayerNames = m_validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &m_vkInstance) != VK_SUCCESS) {
            GN_CORE_ERROR("Unable to create Vulkan instance.");
            return false;
        }
        GN_CORE_INFO("Vulkan instance created successfully.");
        return true;
    }

    bool VulkanRenderer::checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : m_validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    GN_CORE_TRACE("Vulkan validation layer found.");
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }
        return true;
    }

    std::vector<const char*> VulkanRenderer::getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (m_enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    bool VulkanRenderer::createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = m_vkDevice.findQueueFamilies(m_vkDevice.getPhysicalDevice(), m_vkSwapChain.getSurface());

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(m_vkDevice.getDevice(), &poolInfo, nullptr, &m_vkCommandPool) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to create command pool.");
            return false;
        }

        GN_CORE_INFO("Vulkan command pool created successfully.");
        return true;
    }

    bool VulkanRenderer::createCommandBuffer() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_vkCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(m_vkDevice.getDevice(), &allocInfo, &m_vkCommandBuffer) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to allocate command buffers.");
            return false;
        }

        GN_CORE_INFO("Vulkan command buffer created successfully.");
        return true;
    }

    bool VulkanRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;                   // Optional
        beginInfo.pInheritanceInfo = nullptr;  // Optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            GN_CORE_ERROR("Unable to begin recording command buffer.");
            return false;
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_vkPipeline.getRenderPass();
        renderPassInfo.framebuffer = m_vkSwapChain.getSwapchainFramebuffers()[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_vkSwapChain.getSwapchainExtent();

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipeline.getGraphicsPipeline());

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_vkSwapChain.getSwapchainExtent().width);
        viewport.height = static_cast<float>(m_vkSwapChain.getSwapchainExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = m_vkSwapChain.getSwapchainExtent();
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to record command buffer.");
            return false;
        }

        return true;
    }

    bool VulkanRenderer::createSyncObjects() {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(m_vkDevice.getDevice(), &semaphoreInfo, nullptr, &m_vkImageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(m_vkDevice.getDevice(), &semaphoreInfo, nullptr, &m_vkRenderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(m_vkDevice.getDevice(), &fenceInfo, nullptr, &m_vkInFlightFence) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to create semaphores.");
            return false;
        }

        return true;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderer::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                                 VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                                 const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                                 void* pUserData) {
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            GN_CORE_ERROR("Validation layer: {}", pCallbackData->pMessage);
        } else {
            GN_CORE_TRACE("Validation layer: {}", pCallbackData->pMessage);
        }

        return VK_FALSE;
    }

    VkResult VulkanRenderer::createDebugUtilsMessengerEXT(VkInstance instance,
                                                          const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                          const VkAllocationCallbacks* pAllocator,
                                                          VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    void VulkanRenderer::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }

    bool VulkanRenderer::setupDebugMessenger() {
        if (!m_enableValidationLayers) {
            return true;
        }

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessengerCreateInfo(createInfo);

        if (VulkanRenderer::createDebugUtilsMessengerEXT(m_vkInstance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to setup debug messenger.");
            return false;
        }

        GN_CORE_INFO("Vulkan debug messenger setup successfully.");
        return true;
    }

    void VulkanRenderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = VulkanRenderer::debugCallback;
        createInfo.pUserData = nullptr;
    }
}  // namespace Genesis
