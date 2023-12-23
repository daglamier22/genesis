#include "VulkanRenderer.h"

// this code is to work around a GCC bug when also using FMT (which is included by quill logger)
#if defined(__GNUC__) && !defined(NDEBUG) && defined(__OPTIMIZE__)
    #undef __OPTIMIZE__
#endif
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <chrono>

#include "Core/Logger.h"
#include "Platform/GLFWWindow.h"

namespace Genesis {
    VulkanRenderer::VulkanRenderer(std::shared_ptr<Window> window) : Renderer(window) {
        init();
    }

    VulkanRenderer::~VulkanRenderer() {
        shutdown();
    }

    void VulkanRenderer::onResizeEvent(Event& e) {
        GN_CORE_TRACE("Resized");
        m_framebufferResized = true;
    }

    void VulkanRenderer::init() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        m_vulkanDevice.pickPhysicalDevice(m_vkInstance, m_vkSurface);
        m_vulkanDevice.createLogicalDevice(m_vkSurface);
        if (!createSwapChain()) {
            return;
        }
        if (!createImageViews()) {
            return;
        }
        if (!createRenderPass()) {
            return;
        }
        if (!createDescriptorSetLayout()) {
            return;
        }
        if (!createGraphicsPipeline()) {
            return;
        }
        if (!createCommandPool()) {
            return;
        }
        if (!createColorResources()) {
            return;
        }
        if (!createDepthResources()) {
            return;
        }
        if (!createFramebuffers()) {
            return;
        }
        if (!createTextureImage()) {
            return;
        }
        if (!createTextureImageView()) {
            return;
        }
        if (!createTextureSampler()) {
            return;
        }
        if (!loadModel()) {
            return;
        }
        if (!createVertexBuffer()) {
            return;
        }
        if (!createIndexBuffer()) {
            return;
        }
        if (!createUniformBuffers()) {
            return;
        }
        if (!createDescriptorPool()) {
            return;
        }
        if (!createDescriptorSets()) {
            return;
        }
        if (!createCommandBuffers()) {
            return;
        }
        if (!createSyncObjects()) {
            return;
        }
        EventSystem::registerEvent(EventType::WindowResize, this, GN_BIND_EVENT_FN(VulkanRenderer::onResizeEvent));
    }

    bool VulkanRenderer::drawFrame() {
        vkWaitForFences(m_vulkanDevice.logicalDevice(), 1, &m_vkInFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(m_vulkanDevice.logicalDevice(), m_vkSwapchain, UINT64_MAX, m_vkImageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return true;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            GN_CORE_ERROR("Failed to acquire swwap chain image.");
            return false;
        }

        updateUniformBuffer(m_currentFrame);

        // Only reset the fence if we are submitting work
        vkResetFences(m_vulkanDevice.logicalDevice(), 1, &m_vkInFlightFences[m_currentFrame]);

        vkResetCommandBuffer(m_vkCommandBuffers[m_currentFrame], 0);
        recordCommandBuffer(m_vkCommandBuffers[m_currentFrame], imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {m_vkImageAvailableSemaphores[m_currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_vkCommandBuffers[m_currentFrame];

        VkSemaphore signalSemaphores[] = {m_vkRenderFinishedSemaphores[m_currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(m_vulkanDevice.graphicsQueue(), 1, &submitInfo, m_vkInFlightFences[m_currentFrame]) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to submit draw command buffer.");
            return false;
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {m_vkSwapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        presentInfo.pResults = nullptr;  // Optional

        result = vkQueuePresentKHR(m_vulkanDevice.presentQueue(), &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized) {
            m_framebufferResized = false;
            recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to present swap chain image.");
            return false;
        }

        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        return true;
    }

    void VulkanRenderer::shutdown() {
        EventSystem::unregisterEvent(EventType::WindowResize, this, GN_BIND_EVENT_FN(VulkanRenderer::onResizeEvent));

        cleanupSwapChain();

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(m_vulkanDevice.logicalDevice(), m_vkImageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(m_vulkanDevice.logicalDevice(), m_vkRenderFinishedSemaphores[i], nullptr);
            vkDestroyFence(m_vulkanDevice.logicalDevice(), m_vkInFlightFences[i], nullptr);
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(m_vulkanDevice.logicalDevice(), m_vkUniformBuffers[i], nullptr);
            vkFreeMemory(m_vulkanDevice.logicalDevice(), m_vkUniformBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorPool(m_vulkanDevice.logicalDevice(), m_vkDescriptorPool, nullptr);

        vkDestroySampler(m_vulkanDevice.logicalDevice(), m_vkTextureSampler, nullptr);
        vkDestroyImageView(m_vulkanDevice.logicalDevice(), m_vkTextureImageView, nullptr);
        vkDestroyImage(m_vulkanDevice.logicalDevice(), m_vkTextureImage, nullptr);
        vkFreeMemory(m_vulkanDevice.logicalDevice(), m_vkTextureImageMemory, nullptr);

        vkDestroyDescriptorSetLayout(m_vulkanDevice.logicalDevice(), m_vkDescriptorSetLayout, nullptr);

        vkDestroyBuffer(m_vulkanDevice.logicalDevice(), m_vkIndexBuffer, nullptr);
        vkFreeMemory(m_vulkanDevice.logicalDevice(), m_vkIndexBufferMemory, nullptr);

        vkDestroyBuffer(m_vulkanDevice.logicalDevice(), m_vkVertexBuffer, nullptr);
        vkFreeMemory(m_vulkanDevice.logicalDevice(), m_vkVertexBufferMemory, nullptr);

        vkDestroyCommandPool(m_vulkanDevice.logicalDevice(), m_vkCommandPool, nullptr);

        vkDestroyPipeline(m_vulkanDevice.logicalDevice(), m_vkGraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(m_vulkanDevice.logicalDevice(), m_vkPipelineLayout, nullptr);
        vkDestroyRenderPass(m_vulkanDevice.logicalDevice(), m_vkRenderPass, nullptr);

        m_vulkanDevice.logicalDevice().destroy();

        if (m_enableValidationLayers) {
            m_vkInstance.destroyDebugUtilsMessengerEXT(m_vkDebugMessenger, nullptr, m_vkDldi);
        }
        m_vkInstance.destroySurfaceKHR(m_vkSurface);
        m_vkInstance.destroy();
    }

    void VulkanRenderer::cleanupSwapChain() {
        vkDestroyImageView(m_vulkanDevice.logicalDevice(), m_vkDepthImageView, nullptr);
        vkDestroyImage(m_vulkanDevice.logicalDevice(), m_vkDepthImage, nullptr);
        vkFreeMemory(m_vulkanDevice.logicalDevice(), m_vkDepthImageMemory, nullptr);

        vkDestroyImageView(m_vulkanDevice.logicalDevice(), m_vkColorImageView, nullptr);
        vkDestroyImage(m_vulkanDevice.logicalDevice(), m_vkColorImage, nullptr);
        vkFreeMemory(m_vulkanDevice.logicalDevice(), m_vkColorImageMemory, nullptr);

        for (auto framebuffer : m_vkSwapchainFramebuffers) {
            vkDestroyFramebuffer(m_vulkanDevice.logicalDevice(), framebuffer, nullptr);
        }

        for (auto imageView : m_vkSwapchainImageViews) {
            vkDestroyImageView(m_vulkanDevice.logicalDevice(), imageView, nullptr);
        }

        vkDestroySwapchainKHR(m_vulkanDevice.logicalDevice(), m_vkSwapchain, nullptr);
    }

    void VulkanRenderer::waitForIdle() {
        vkDeviceWaitIdle(m_vulkanDevice.logicalDevice());
    }

    void VulkanRenderer::createInstance() {
        std::shared_ptr<GLFWWindow> window = std::dynamic_pointer_cast<GLFWWindow>(m_window);
        vk::ApplicationInfo appInfo(window->getWindowTitle().c_str(),
                                    VK_MAKE_VERSION(1, 0, 0),
                                    "Genesis",
                                    VK_MAKE_VERSION(1, 0, 0),
                                    VK_API_VERSION_1_0);

        // confirm required extension support
        std::vector<const char*> extensions = getRequiredExtensions();
        if (!checkInstanceExtensionSupport(extensions)) {
            const std::string errMsg = "Instance extensions requested, but not available.";
            GN_CORE_ERROR("{}", errMsg);
            throw std::runtime_error(errMsg);
        }
#if defined(GN_PLATFORM_MACOS)
        extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

        // confirm required validation layer support
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        std::vector<const char*> validationLayers;
        if (m_enableValidationLayers) {
            validationLayers.push_back("VK_LAYER_KHRONOS_validation");
        }
        if (m_enableValidationLayers && !checkValidationLayerSupport(validationLayers)) {
            const std::string errMsg = "Validation layers requested, but not available.";
            GN_CORE_ERROR("{}", errMsg);
            throw std::runtime_error(errMsg);
        }

        vk::InstanceCreateInfo createInfo(vk::InstanceCreateFlags(),
                                          &appInfo,
                                          static_cast<uint32_t>(validationLayers.size()),
                                          validationLayers.data(),
                                          static_cast<uint32_t>(extensions.size()),
                                          extensions.data());

        try {
            m_vkInstance = vk::createInstance(createInfo);
        } catch (vk::SystemError err) {
            const std::string errMsg = "Unable to create Vulkan instance: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        GN_CORE_INFO("Vulkan instance created successfully.");
    }

    bool VulkanRenderer::checkInstanceExtensionSupport(std::vector<const char*>& extensions) {
        std::vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();
        std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());

        GN_CORE_TRACE("Available Vulkan instance extensions:");
        for (vk::ExtensionProperties availableExtension : availableExtensions) {
            GN_CORE_TRACE("\t{}", availableExtension.extensionName.data());
            requiredExtensions.erase(availableExtension.extensionName);
        }
        if (requiredExtensions.empty()) {
            GN_CORE_TRACE("Vulkan instance extensions found.");
        }
        return requiredExtensions.empty();
    }

    bool VulkanRenderer::checkValidationLayerSupport(std::vector<const char*>& layers) {
        std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();
        std::set<std::string> requiredLayers(layers.begin(), layers.end());

        GN_CORE_TRACE("Available Vulkan instance layers:");
        for (vk::LayerProperties availableLayer : availableLayers) {
            GN_CORE_TRACE("\t{}", availableLayer.layerName.data());
            requiredLayers.erase(availableLayer.layerName);
        }
        if (requiredLayers.empty()) {
            GN_CORE_TRACE("Vulkan instance layers found.");
        }
        return requiredLayers.empty();
    }

    std::vector<const char*> VulkanRenderer::getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;

        std::shared_ptr<GLFWWindow> window = std::dynamic_pointer_cast<GLFWWindow>(m_window);
        glfwExtensions = window->getRequiredVulkanInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (m_enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    void VulkanRenderer::createSurface() {
        std::shared_ptr<GLFWWindow> window = std::dynamic_pointer_cast<GLFWWindow>(m_window);
        // create a temp C style surface to retrieve from GLFW
        VkSurfaceKHR cStyleSurface;
        if (!window->createVulkanSurface(m_vkInstance, &cStyleSurface)) {
            std::string errMsg = "Failed to create window surface.";
            GN_CORE_ERROR("{}", errMsg);
            throw std::runtime_error(errMsg);
        }

        // then convert to the HPP style surface
        m_vkSurface = cStyleSurface;

        GN_CORE_INFO("Vulkan surface created successfully.");
    }

    bool VulkanRenderer::createSwapChain() {
        SwapChainSupportDetails swapChainSupport = m_vulkanDevice.querySwapChainSupport(m_vulkanDevice.physicalDevice(), m_vkSurface);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_vkSurface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = m_vulkanDevice.findQueueFamilies(m_vulkanDevice.physicalDevice(), m_vkSurface);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(m_vulkanDevice.logicalDevice(), &createInfo, nullptr, &m_vkSwapchain) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to create swap chain.");
            return false;
        }

        vkGetSwapchainImagesKHR(m_vulkanDevice.logicalDevice(), m_vkSwapchain, &imageCount, nullptr);
        m_vkSwapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_vulkanDevice.logicalDevice(), m_vkSwapchain, &imageCount, m_vkSwapchainImages.data());

        m_vkSwapchainImageFormat = surfaceFormat.format;
        m_vkSwapchainExtent = extent;

        GN_CORE_INFO("Vulkan swapchain created successfully.");
        return true;
    }

    bool VulkanRenderer::createImageViews() {
        bool success = false;

        m_vkSwapchainImageViews.resize(m_vkSwapchainImages.size());

        for (size_t i = 0; i < m_vkSwapchainImages.size(); i++) {
            success = createImageView(m_vkSwapchainImages[i], m_vkSwapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, &m_vkSwapchainImageViews[i]);
        }

        GN_CORE_INFO("Image views created successfully.");
        return success;
    }

    bool VulkanRenderer::createFramebuffers() {
        m_vkSwapchainFramebuffers.resize(m_vkSwapchainImageViews.size());

        for (size_t i = 0; i < m_vkSwapchainImageViews.size(); i++) {
            std::array<VkImageView, 3> attachments = {m_vkColorImageView, m_vkDepthImageView, m_vkSwapchainImageViews[i]};

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_vkRenderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = m_vkSwapchainExtent.width;
            framebufferInfo.height = m_vkSwapchainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(m_vulkanDevice.logicalDevice(), &framebufferInfo, nullptr, &m_vkSwapchainFramebuffers[i]) != VK_SUCCESS) {
                GN_CORE_ERROR("Failed to create framebuffer.");
                return false;
            }
        }

        GN_CORE_INFO("Vulkan framebuffers created successfully.");
        return true;
    }

    vk::SurfaceFormatKHR VulkanRenderer::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    vk::PresentModeKHR VulkanRenderer::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
                return availablePresentMode;
            }
        }

        return vk::PresentModeKHR::eFifo;
    }

    VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        std::shared_ptr<GLFWWindow> window = std::dynamic_pointer_cast<GLFWWindow>(m_window);
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize((GLFWwindow*)window->getWindow(), &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)};

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    void VulkanRenderer::recreateSwapChain() {
        std::shared_ptr<GLFWWindow> window = std::dynamic_pointer_cast<GLFWWindow>(m_window);
        int width = (int)window->getWindowWidth();
        int height = (int)window->getWindowHeight();
        while (width == 0 || height == 0) {
            window->waitForWindowToBeRestored(&width, &height);
        }

        vkDeviceWaitIdle(m_vulkanDevice.logicalDevice());

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createColorResources();
        createDepthResources();
        createFramebuffers();

        GN_CORE_INFO("Vulkan swapchain recreated.");
    }

    bool VulkanRenderer::createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr;  // Optional

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(m_vulkanDevice.logicalDevice(), &layoutInfo, nullptr, &m_vkDescriptorSetLayout) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to create descriptor set layout.");
            return false;
        }

        GN_CORE_INFO("Vulkan descriptor set layour created successfully.");
        return true;
    }

    bool VulkanRenderer::createGraphicsPipeline() {
        auto vertShaderCode = readFile("assets/shaders/shader.vert.spv");
        if (vertShaderCode.size() == 0) {
            return false;
        }
        auto fragShaderCode = readFile("assets/shaders/shader.frag.spv");
        if (fragShaderCode.size() == 0) {
            return false;
        }

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)m_vkSwapchainExtent.width;
        viewport.height = (float)m_vkSwapchainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = m_vkSwapchainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f;  // Optional
        rasterizer.depthBiasClamp = 0.0f;           // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f;     // Optional

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_TRUE;  // NOTE: expensive! enable sample shading in the pipeline
        multisampling.minSampleShading = .2f;         // NOTE: expensive! min fraction for sample shading; closer to one is smoother
        // multisampling.rasterizationSamples = m_vulkanDevice.msaaSamples(); // TODO: fix
        multisampling.minSampleShading = 1.0f;           // Optional
        multisampling.pSampleMask = nullptr;             // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
        multisampling.alphaToOneEnable = VK_FALSE;       // Optional

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f;  // Optional
        depthStencil.maxDepthBounds = 1.0f;  // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {};  // Optional
        depthStencil.back = {};   // Optional

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;  // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;  // Optional
        colorBlending.blendConstants[1] = 0.0f;  // Optional
        colorBlending.blendConstants[2] = 0.0f;  // Optional
        colorBlending.blendConstants[3] = 0.0f;  // Optional

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_vkDescriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0;     // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr;  // Optional

        if (vkCreatePipelineLayout(m_vulkanDevice.logicalDevice(), &pipelineLayoutInfo, nullptr, &m_vkPipelineLayout) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to create pipeline layout.");
            return false;
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = m_vkPipelineLayout;
        pipelineInfo.renderPass = m_vkRenderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;  // Optional
        pipelineInfo.basePipelineIndex = -1;               // Optional

        if (vkCreateGraphicsPipelines(m_vulkanDevice.logicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_vkGraphicsPipeline) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to create graphics pipeline.");
            return false;
        }

        vkDestroyShaderModule(m_vulkanDevice.logicalDevice(), vertShaderModule, nullptr);
        vkDestroyShaderModule(m_vulkanDevice.logicalDevice(), fragShaderModule, nullptr);

        GN_CORE_INFO("Vulkan pipeline created successfully.");
        return true;
    }

    bool VulkanRenderer::createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = m_vkSwapchainImageFormat;
        // colorAttachment.samples = m_vulkanDevice.msaaSamples(); // TODO: fix
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat();
        // depthAttachment.samples = m_vulkanDevice.msaaSamples(); // TODO: fix
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachmentResolve{};
        colorAttachmentResolve.format = m_vkSwapchainImageFormat;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentResolveRef{};
        colorAttachmentResolveRef.attachment = 2;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        subpass.pResolveAttachments = &colorAttachmentResolveRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(m_vulkanDevice.logicalDevice(), &renderPassInfo, nullptr, &m_vkRenderPass) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to create render pass.");
            return false;
        }

        GN_CORE_INFO("Vulkan renderpass created successfully.");
        return true;
    }

    VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(m_vulkanDevice.logicalDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to create shader module.");
        }

        return shaderModule;
    }

    std::vector<char> VulkanRenderer::readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            GN_CORE_ERROR("Failed to open file {}.", filename);
            std::vector<char> emptyContents(0);
            return emptyContents;
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    bool VulkanRenderer::createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = m_vulkanDevice.findQueueFamilies(m_vulkanDevice.physicalDevice(), m_vkSurface);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(m_vulkanDevice.logicalDevice(), &poolInfo, nullptr, &m_vkCommandPool) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to create command pool.");
            return false;
        }

        GN_CORE_INFO("Vulkan command pool created successfully.");
        return true;
    }

    bool VulkanRenderer::createColorResources() {
        VkFormat colorFormat = m_vkSwapchainImageFormat;
        if (!createImage(m_vkSwapchainExtent.width,
                         m_vkSwapchainExtent.height,
                         1,
                         VK_SAMPLE_COUNT_1_BIT,  // m_vulkanDevice.msaaSamples(), // TODO: fix
                         colorFormat,
                         VK_IMAGE_TILING_OPTIMAL,
                         VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                         m_vkColorImage,
                         m_vkColorImageMemory)) {
            return false;
        }
        if (!createImageView(m_vkColorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, &m_vkColorImageView)) {
            return false;
        }

        GN_CORE_INFO("Vulkan color resources created successfuly.");
        return true;
    }

    bool VulkanRenderer::createTextureImage() {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;
        m_vkMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        if (!pixels) {
            GN_CORE_ERROR("Failed to load texture image.");
            return false;
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(imageSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer,
                     stagingBufferMemory);

        void* data;
        vkMapMemory(m_vulkanDevice.logicalDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(m_vulkanDevice.logicalDevice(), stagingBufferMemory);

        stbi_image_free(pixels);

        createImage(texWidth,
                    texHeight,
                    m_vkMipLevels,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_FORMAT_R8G8B8A8_SRGB,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    m_vkTextureImage,
                    m_vkTextureImageMemory);

        transitionImageLayout(m_vkTextureImage,
                              VK_FORMAT_R8G8B8A8_SRGB,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              m_vkMipLevels);
        copyBufferToImage(stagingBuffer, m_vkTextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        // transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
        // transitionImageLayout(m_vkTextureImage,
        //                       VK_FORMAT_R8G8B8A8_SRGB,
        //                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        //                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        //                       m_vkMipLevels);

        vkDestroyBuffer(m_vulkanDevice.logicalDevice(), stagingBuffer, nullptr);
        vkFreeMemory(m_vulkanDevice.logicalDevice(), stagingBufferMemory, nullptr);

        if (!generateMipmaps(m_vkTextureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, m_vkMipLevels)) {
            return false;
        }

        GN_CORE_INFO("Texture successfully loaded: {}", TEXTURE_PATH.c_str());
        return true;
    }

    bool VulkanRenderer::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
        // Check if image format supports linear blitting
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(m_vulkanDevice.physicalDevice(), imageFormat, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            GN_CORE_ERROR("Texture image format does not support linear blitting.");
            return false;
        }

        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer,
                           image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &blit,
                           VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            if (mipWidth > 1) {
                mipWidth /= 2;
            }
            if (mipHeight > 1) {
                mipHeight /= 2;
            }
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

        endSignleTimeCommands(commandBuffer);

        GN_CORE_INFO("Vulkan mip maps generated successfully.");
        return true;
    }

    bool VulkanRenderer::createTextureImageView() {
        return createImageView(m_vkTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, m_vkMipLevels, &m_vkTextureImageView);
    }

    bool VulkanRenderer::createImageView(VkImage image,
                                         VkFormat format,
                                         VkImageAspectFlags aspectFlags,
                                         uint32_t mipLevels,
                                         VkImageView* imageView) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_vulkanDevice.logicalDevice(), &viewInfo, nullptr, imageView) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to create image view.");
            return false;
        }

        GN_CORE_INFO("Vulkan image view created successfully.");
        return true;
    }

    bool VulkanRenderer::createImage(uint32_t width,
                                     uint32_t height,
                                     uint32_t mipLevels,
                                     VkSampleCountFlagBits numSamples,
                                     VkFormat format,
                                     VkImageTiling tiling,
                                     VkImageUsageFlags usage,
                                     VkMemoryPropertyFlags properties,
                                     VkImage& image,
                                     VkDeviceMemory& imageMemory) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = numSamples;
        imageInfo.flags = 0;  // Optional

        if (vkCreateImage(m_vulkanDevice.logicalDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to create image.");
            return false;
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_vulkanDevice.logicalDevice(), image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(m_vulkanDevice.logicalDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to allocate image memory.");
            return false;
        }

        vkBindImageMemory(m_vulkanDevice.logicalDevice(), image, imageMemory, 0);

        GN_CORE_INFO("Image loaded successfully.");
        return true;
    }

    bool VulkanRenderer::createTextureSampler() {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = m_vulkanDevice.physicalDeviceProperties().limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;  // Optional
        samplerInfo.minLod = 0.0f;      // Optional
        samplerInfo.maxLod = static_cast<float>(m_vkMipLevels);

        if (vkCreateSampler(m_vulkanDevice.logicalDevice(), &samplerInfo, nullptr, &m_vkTextureSampler) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to create texture sampler.");
            return false;
        }

        GN_CORE_INFO("Vulkan texture sampler created succesfully.");
        return true;
    }

    bool VulkanRenderer::transitionImageLayout(VkImage image,
                                               VkFormat format,
                                               VkImageLayout oldLayout,
                                               VkImageLayout newLayout,
                                               uint32_t mipLevels) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;

        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            if (hasStencilComponent(format)) {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        } else {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        } else {
            GN_CORE_ERROR("Unsupported layout transition.");
            return false;
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        endSignleTimeCommands(commandBuffer);

        return true;
    }

    void VulkanRenderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            width,
            height,
            1};

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region);

        endSignleTimeCommands(commandBuffer);
    }

    bool VulkanRenderer::createCommandBuffers() {
        m_vkCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_vkCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)m_vkCommandBuffers.size();

        if (vkAllocateCommandBuffers(m_vulkanDevice.logicalDevice(), &allocInfo, m_vkCommandBuffers.data()) != VK_SUCCESS) {
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
        renderPassInfo.renderPass = m_vkRenderPass;
        renderPassInfo.framebuffer = m_vkSwapchainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_vkSwapchainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkGraphicsPipeline);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_vkSwapchainExtent.width);
        viewport.height = static_cast<float>(m_vkSwapchainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = m_vkSwapchainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkBuffer vertexBuffers[] = {m_vkVertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(commandBuffer, m_vkIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipelineLayout, 0, 1, &m_vkDescriptorSets[m_currentFrame], 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to record command buffer.");
            return false;
        }

        return true;
    }

    bool VulkanRenderer::createSyncObjects() {
        m_vkImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_vkRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_vkInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(m_vulkanDevice.logicalDevice(), &semaphoreInfo, nullptr, &m_vkImageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_vulkanDevice.logicalDevice(), &semaphoreInfo, nullptr, &m_vkRenderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_vulkanDevice.logicalDevice(), &fenceInfo, nullptr, &m_vkInFlightFences[i]) != VK_SUCCESS) {
                GN_CORE_ERROR("Failed to create semaphores.");
                return false;
            }
        }

        return true;
    }

    bool VulkanRenderer::loadModel() {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
            GN_CORE_ERROR("{}{}", warn, err);
            return false;
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};

                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2],
                };

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
                };

                vertex.color = {1.0f, 1.0f, 1.0f};

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
                    m_vertices.push_back(vertex);
                }

                m_indices.push_back(uniqueVertices[vertex]);
            }
        }

        GN_CORE_INFO("Model loadded successfully.");
        return true;
    }

    bool VulkanRenderer::createVertexBuffer() {
        VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(m_vulkanDevice.logicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, m_vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(m_vulkanDevice.logicalDevice(), stagingBufferMemory);

        createBuffer(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     m_vkVertexBuffer,
                     m_vkVertexBufferMemory);

        copyBuffer(stagingBuffer, m_vkVertexBuffer, bufferSize);

        vkDestroyBuffer(m_vulkanDevice.logicalDevice(), stagingBuffer, nullptr);
        vkFreeMemory(m_vulkanDevice.logicalDevice(), stagingBufferMemory, nullptr);

        GN_CORE_INFO("Vulkan vertex buffer created.");
        return true;
    }

    bool VulkanRenderer::createIndexBuffer() {
        VkDeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer,
                     stagingBufferMemory);

        void* data;
        vkMapMemory(m_vulkanDevice.logicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, m_indices.data(), (size_t)bufferSize);
        vkUnmapMemory(m_vulkanDevice.logicalDevice(), stagingBufferMemory);

        createBuffer(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     m_vkIndexBuffer,
                     m_vkIndexBufferMemory);

        copyBuffer(stagingBuffer, m_vkIndexBuffer, bufferSize);

        vkDestroyBuffer(m_vulkanDevice.logicalDevice(), stagingBuffer, nullptr);
        vkFreeMemory(m_vulkanDevice.logicalDevice(), stagingBufferMemory, nullptr);

        return true;
    }

    bool VulkanRenderer::createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        m_vkUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        m_vkUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        m_vkUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(bufferSize,
                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         m_vkUniformBuffers[i], m_vkUniformBuffersMemory[i]);
            vkMapMemory(m_vulkanDevice.logicalDevice(), m_vkUniformBuffersMemory[i], 0, bufferSize, 0, &m_vkUniformBuffersMapped[i]);
        }

        GN_CORE_INFO("Vulkan uniform buffers created successfully.");
        return true;
    }

    bool VulkanRenderer::createDescriptorPool() {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(m_vulkanDevice.logicalDevice(), &poolInfo, nullptr, &m_vkDescriptorPool) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to create descriptor pool.");
            return false;
        }

        GN_CORE_INFO("Vulkan descriptor pool created successfully.");
        return true;
    }

    bool VulkanRenderer::createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_vkDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_vkDescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        m_vkDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(m_vulkanDevice.logicalDevice(), &allocInfo, m_vkDescriptorSets.data()) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to allocate descriptor sets.");
            return false;
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = m_vkUniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = m_vkTextureImageView;
            imageInfo.sampler = m_vkTextureSampler;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = m_vkDescriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;
            descriptorWrites[0].pImageInfo = nullptr;        // Optional
            descriptorWrites[0].pTexelBufferView = nullptr;  // Optional

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = m_vkDescriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(m_vulkanDevice.logicalDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }

        GN_CORE_INFO("Vulkan descriptor sets created successfully.");
        return true;
    }

    void VulkanRenderer::updateUniformBuffer(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.projection = glm::perspective(glm::radians(45.0f), m_vkSwapchainExtent.width / (float)m_vkSwapchainExtent.height, 0.1f, 10.0f);
        ubo.projection[1][1] *= -1;

        memcpy(m_vkUniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

    bool VulkanRenderer::createBuffer(VkDeviceSize size,
                                      VkBufferUsageFlags usage,
                                      VkMemoryPropertyFlags properties,
                                      VkBuffer& buffer,
                                      VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(m_vulkanDevice.logicalDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to create vertex buffer.");
            return false;
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_vulkanDevice.logicalDevice(), buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(m_vulkanDevice.logicalDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to allocate vertex buffer memory.");
            return false;
        }

        vkBindBufferMemory(m_vulkanDevice.logicalDevice(), buffer, bufferMemory, 0);

        return true;
    }

    bool VulkanRenderer::createDepthResources() {
        VkFormat depthFormat = findDepthFormat();

        createImage(m_vkSwapchainExtent.width,
                    m_vkSwapchainExtent.height,
                    1,
                    VK_SAMPLE_COUNT_1_BIT,  // m_vulkanDevice.msaaSamples(), // TODO: fix
                    depthFormat,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    m_vkDepthImage,
                    m_vkDepthImageMemory);
        createImageView(m_vkDepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1, &m_vkDepthImageView);

        transitionImageLayout(m_vkDepthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);

        GN_CORE_INFO("Vulkan depth resources created successfully.");
        return true;
    }

    VkFormat VulkanRenderer::findSupportedFormat(const std::vector<VkFormat>& candidates,
                                                 VkImageTiling tiling,
                                                 VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_vulkanDevice.physicalDevice(), format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        GN_CORE_ERROR("Failed to find supported format.");
        return VkFormat();
    }

    VkFormat VulkanRenderer::findDepthFormat() {
        return findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                   VK_IMAGE_TILING_OPTIMAL,
                                   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    bool VulkanRenderer::hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    VkCommandBuffer VulkanRenderer::beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_vkCommandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(m_vulkanDevice.logicalDevice(), &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void VulkanRenderer::endSignleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(m_vulkanDevice.graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_vulkanDevice.graphicsQueue());

        vkFreeCommandBuffers(m_vulkanDevice.logicalDevice(), m_vkCommandPool, 1, &commandBuffer);
    }

    void VulkanRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;  // Optional
        copyRegion.dstOffset = 0;  // Optional
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSignleTimeCommands(commandBuffer);
    }

    uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_vulkanDevice.physicalDevice(), &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        GN_CORE_ERROR("Failed to find suitable memory type.");
        return 999999;
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

    void VulkanRenderer::setupDebugMessenger() {
        if (!m_enableValidationLayers) {
            return;
        }

        m_vkDldi = vk::DispatchLoaderDynamic(m_vkInstance, vkGetInstanceProcAddr);

        vk::DebugUtilsMessengerCreateInfoEXT createInfo = vk::DebugUtilsMessengerCreateInfoEXT(
            vk::DebugUtilsMessengerCreateFlagsEXT(),
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            VulkanRenderer::debugCallback,
            nullptr);

        try {
            m_vkDebugMessenger = m_vkInstance.createDebugUtilsMessengerEXT(createInfo, nullptr, m_vkDldi);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to setup debug messenger: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        GN_CORE_INFO("Vulkan debug messenger setup successfully.");
    }
}  // namespace Genesis
