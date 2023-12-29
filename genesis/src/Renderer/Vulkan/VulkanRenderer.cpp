#include "VulkanRenderer.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <chrono>

#include "Core/Logger.h"
#include "Platform/GLFWWindow.h"
#include "VulkanShader.h"

namespace Genesis {
    VulkanRenderer::VulkanRenderer(std::shared_ptr<Window> window) : Renderer(window) {
        init();
    }

    VulkanRenderer::~VulkanRenderer() {
        shutdown();
    }

    void VulkanRenderer::onResizeEvent(Event& e) {
        GN_CORE_TRACE("Resized");
        // m_framebufferResized = true;
    }

    void VulkanRenderer::init() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        m_vulkanDevice.pickPhysicalDevice(m_vkInstance, m_vkSurface);
        m_vulkanDevice.createLogicalDevice(m_vkSurface);
        m_vulkanSwapchain.createSwapChain(m_vulkanDevice, m_vkSurface, m_window);
        m_vulkanPipeline.createRenderPass(m_vulkanDevice, m_vulkanSwapchain);
        m_vulkanSwapchain.createDescriptorSetLayouts(m_vulkanDevice);
        m_vulkanPipeline.createGraphicsPipeline(m_vulkanDevice, m_vulkanSwapchain);
        createCommandPool();
        createCommandBuffers();
        m_vulkanSwapchain.createFrameResources(m_vulkanDevice, m_vulkanPipeline.renderPass(), m_vkCommandPool, m_vkMainCommandBuffer);
        // loadModel();
        createAssets();
        EventSystem::registerEvent(EventType::WindowResize, this, GN_BIND_EVENT_FN(VulkanRenderer::onResizeEvent));
    }

    bool VulkanRenderer::drawFrame(std::shared_ptr<Scene> scene) {
        renderFrame(scene);
        return true;
    }

    void VulkanRenderer::shutdown() {
        EventSystem::unregisterEvent(EventType::WindowResize, this, GN_BIND_EVENT_FN(VulkanRenderer::onResizeEvent));

        m_vulkanDevice.logicalDevice().destroyDescriptorPool(m_vulkanSwapchain.meshDescriptorPool());
        m_vulkanSwapchain.cleanupSwapChain(m_vulkanDevice, m_vkCommandPool);

        m_vulkanDevice.logicalDevice().destroyDescriptorSetLayout(m_vulkanSwapchain.frameDescriptorSetLayout());

        m_vulkanDevice.logicalDevice().destroyBuffer(m_vulkanMeshes.vertexBuffer().buffer());
        m_vulkanDevice.logicalDevice().freeMemory(m_vulkanMeshes.vertexBuffer().memory());
        m_vulkanDevice.logicalDevice().destroyBuffer(m_vulkanMeshes.indexBuffer().buffer());
        m_vulkanDevice.logicalDevice().freeMemory(m_vulkanMeshes.indexBuffer().memory());

        for (const auto& [key, texture] : m_materials) {
            delete texture;
        }

        m_vulkanDevice.logicalDevice().destroyDescriptorSetLayout(m_vulkanSwapchain.meshDescriptorSetLayout());

        m_vulkanDevice.logicalDevice().destroyCommandPool(m_vkCommandPool);

        m_vulkanDevice.logicalDevice().destroyPipeline(m_vulkanPipeline.pipeline());
        m_vulkanDevice.logicalDevice().destroyPipelineLayout(m_vulkanPipeline.layout());
        m_vulkanDevice.logicalDevice().destroyRenderPass(m_vulkanPipeline.renderPass());

        m_vulkanDevice.logicalDevice().destroy();

        if (m_enableValidationLayers) {
            m_vkInstance.destroyDebugUtilsMessengerEXT(m_vkDebugMessenger, nullptr, m_vkDldi);
        }
        m_vkInstance.destroySurfaceKHR(m_vkSurface);
        m_vkInstance.destroy();
    }

    void VulkanRenderer::waitForIdle() {
        m_vulkanDevice.logicalDevice().waitIdle();
    }

    void VulkanRenderer::createInstance() {
        std::shared_ptr<GLFWWindow> window = std::dynamic_pointer_cast<GLFWWindow>(m_window);
        vk::ApplicationInfo appInfo(window->getWindowTitle().c_str(),
                                    VK_MAKE_VERSION(1, 0, 0),
                                    "Genesis",
                                    VK_MAKE_VERSION(1, 0, 0),
                                    VK_API_VERSION_1_1);

        // confirm required extension support
        std::vector<const char*> extensions = getRequiredExtensions();
        if (!checkInstanceExtensionSupport(extensions)) {
            const std::string errMsg = "Instance extensions requested, but not available.";
            GN_CORE_ERROR("{}", errMsg);
            throw std::runtime_error(errMsg);
        }

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

#if defined(GN_PLATFORM_MACOS)
        extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        createInfo.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

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

    void VulkanRenderer::createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = m_vulkanDevice.findQueueFamilies(m_vulkanDevice.physicalDevice(), m_vkSurface);

        vk::CommandPoolCreateInfo poolInfo = {};
        poolInfo.flags = vk::CommandPoolCreateFlags() | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        try {
            m_vkCommandPool = m_vulkanDevice.logicalDevice().createCommandPool(poolInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create command pool: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        GN_CORE_INFO("Vulkan command pool created successfully.");
    }

    void VulkanRenderer::createCommandBuffers() {
        vk::CommandBufferAllocateInfo allocInfo = {};
        allocInfo.commandPool = m_vkCommandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = 1;

        try {
            m_vkMainCommandBuffer = m_vulkanDevice.logicalDevice().allocateCommandBuffers(allocInfo)[0];
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to allocate command buffers: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        GN_CORE_INFO("Vulkan command buffer created successfully.");
    }

    void VulkanRenderer::renderFrame(std::shared_ptr<Scene> scene) {
        auto currentFrame = m_vulkanSwapchain.swapchainFrames()[m_currentFrame];
        vk::Result result;
        try {
            result = m_vulkanDevice.logicalDevice().waitForFences(1, &currentFrame.inFlightFence, VK_TRUE, UINT64_MAX);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to wait for fence: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        uint32_t imageIndex;
        try {
            auto result = m_vulkanDevice.logicalDevice().acquireNextImageKHR(m_vulkanSwapchain.swapchain(), UINT64_MAX, currentFrame.imageAvailableSemaphore, nullptr);
            imageIndex = result.value;
        } catch (vk::OutOfDateKHRError err) {
            m_vulkanSwapchain.recreateSwapChain(m_vulkanDevice, m_vkSurface, m_window, m_vulkanPipeline.renderPass(), m_vkCommandPool, m_vkMainCommandBuffer);
            return;
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to aquire swap chain image: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        // Only reset the fence if we are submitting work
        try {
            result = m_vulkanDevice.logicalDevice().resetFences(1, &currentFrame.inFlightFence);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to reset fence: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        m_vulkanSwapchain.swapchainFrames()[m_currentFrame].commandBuffer.reset();

        try {
            m_vulkanSwapchain.prepareFrame(m_vulkanDevice, m_currentFrame, scene);
        } catch (std::exception err) {
            GN_CORE_ERROR("{}", err.what());
        }

        recordCommandBuffer(currentFrame.commandBuffer, imageIndex, scene);

        vk::SubmitInfo submitInfo = {};

        vk::Semaphore waitSemaphores[] = {currentFrame.imageAvailableSemaphore};
        vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &currentFrame.commandBuffer;

        vk::Semaphore signalSemaphores[] = {currentFrame.renderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        try {
            m_vulkanDevice.graphicsQueue().submit(submitInfo, currentFrame.inFlightFence);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to submit draw command buffer: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        vk::PresentInfoKHR presentInfo = {};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        vk::SwapchainKHR swapChains[] = {m_vulkanSwapchain.swapchain()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        try {
            auto result = m_vulkanDevice.presentQueue().presentKHR(presentInfo);
            if (result == vk::Result::eSuboptimalKHR || m_framebufferResized) {
                m_framebufferResized = false;
                m_vulkanSwapchain.recreateSwapChain(m_vulkanDevice, m_vkSurface, m_window, m_vulkanPipeline.renderPass(), m_vkCommandPool, m_vkMainCommandBuffer);
            }
        } catch (vk::OutOfDateKHRError err) {
            m_framebufferResized = false;
            m_vulkanSwapchain.recreateSwapChain(m_vulkanDevice, m_vkSurface, m_window, m_vulkanPipeline.renderPass(), m_vkCommandPool, m_vkMainCommandBuffer);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to present swap chain image: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        m_currentFrame = (m_currentFrame + 1) % m_vulkanSwapchain.swapchainFrames().size();
    }

    void VulkanRenderer::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex, std::shared_ptr<Scene> scene) {
        vk::CommandBufferBeginInfo beginInfo = {};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        try {
            commandBuffer.begin(beginInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Unable to being recording command buffer: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        vk::RenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.renderPass = m_vulkanPipeline.renderPass();
        renderPassInfo.framebuffer = m_vulkanSwapchain.swapchainFrames()[imageIndex].framebuffer;
        renderPassInfo.renderArea.offset.x = 0;
        renderPassInfo.renderArea.offset.y = 0;
        renderPassInfo.renderArea.extent = m_vulkanSwapchain.extent();

        std::array<vk::ClearValue, 2> clearValues{};
        clearValues[0].color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
        clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_vulkanPipeline.pipeline());

        // vk::Viewport viewport = {};
        // viewport.x = 0.0f;
        // viewport.y = 0.0f;
        // viewport.width = static_cast<float>(m_vulkanSwapchain.extent().width);
        // viewport.height = static_cast<float>(m_vulkanSwapchain.extent().height);
        // viewport.minDepth = 0.0f;
        // viewport.maxDepth = 1.0f;
        // commandBuffer.setViewport(0, 1, &viewport);

        // vk::Rect2D scissor = {};
        // scissor.offset.x = 0;
        // scissor.offset.y = 0;
        // scissor.extent = m_vulkanSwapchain.extent();
        // commandBuffer.setScissor(0, 1, &scissor);

        vk::Buffer vertexBuffers[] = {m_vulkanMeshes.vertexBuffer().buffer()};
        vk::DeviceSize offsets[] = {0};
        commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
        commandBuffer.bindIndexBuffer(m_vulkanMeshes.indexBuffer().buffer(), 0, vk::IndexType::eUint32);

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                         m_vulkanPipeline.layout(),
                                         0,
                                         1,
                                         &m_vulkanSwapchain.swapchainFrames()[m_currentFrame].descriptorSet,
                                         0,
                                         nullptr);

        uint32_t startInstance = 0;
        // Triangles
        uint32_t instanceCount = static_cast<uint32_t>(scene->trianglePositions.size());
        renderObjects(commandBuffer, meshTypes::TRIANGLE, startInstance, instanceCount);

        // Squares
        instanceCount = static_cast<uint32_t>(scene->squarePositions.size());
        renderObjects(commandBuffer, meshTypes::SQUARE, startInstance, instanceCount);

        // Stars
        instanceCount = static_cast<uint32_t>(scene->starPositions.size());
        renderObjects(commandBuffer, meshTypes::STAR, startInstance, instanceCount);

        commandBuffer.endRenderPass();

        try {
            commandBuffer.end();
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to record command buffer: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }
    }

    void VulkanRenderer::renderObjects(vk::CommandBuffer commandBuffer, meshTypes objectType, uint32_t& startInstance, uint32_t instanceCount) {
        int indexCount = m_vulkanMeshes.m_indexCounts.find(objectType)->second;
        int firstIndex = m_vulkanMeshes.m_firstIndices.find(objectType)->second;
        m_materials[objectType]->use(commandBuffer, m_vulkanPipeline.layout());
        commandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, 0, startInstance);
        startInstance += instanceCount;
    }

    // void VulkanRenderer::loadModel() {
    //     tinyobj::attrib_t attrib;
    //     std::vector<tinyobj::shape_t> shapes;
    //     std::vector<tinyobj::material_t> materials;
    //     std::string warn, err;

    //     if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
    //         GN_CORE_ERROR("{}{}", warn, err);
    //         throw std::runtime_error(warn + err);
    //     }

    //     std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    //     for (const auto& shape : shapes) {
    //         for (const auto& index : shape.mesh.indices) {
    //             Vertex vertex{};

    //             vertex.pos = {
    //                 attrib.vertices[3 * index.vertex_index + 0],
    //                 attrib.vertices[3 * index.vertex_index + 1],
    //                 attrib.vertices[3 * index.vertex_index + 2],
    //             };

    //             vertex.texCoord = {
    //                 attrib.texcoords[2 * index.texcoord_index + 0],
    //                 1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
    //             };

    //             vertex.color = {1.0f, 1.0f, 1.0f};

    //             if (uniqueVertices.count(vertex) == 0) {
    //                 uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
    //                 m_vertices.push_back(vertex);
    //             }

    //             m_indices.push_back(uniqueVertices[vertex]);
    //         }
    //     }

    //     GN_CORE_INFO("Model loadded successfully.");
    // }

    void VulkanRenderer::createAssets() {
        std::vector<float> triangleVertices = {{0.0f, -0.1f, 1.0f, 1.0f, 1.0f, 0.5f, 0.0f,    // 0
                                                0.1f, 0.1f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,     // 1
                                                -0.1f, 0.1f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f}};  // 2
        std::vector<uint32_t> indices = {{0, 1, 2}};
        meshTypes type = meshTypes::TRIANGLE;
        m_vulkanMeshes.consume(type, triangleVertices, indices);

        std::vector<float> squareVertices = {{-0.1f, 0.1f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,   // 0
                                              -0.1f, -0.1f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,  // 1
                                              0.1f, -0.1f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,   // 2
                                              0.1f, 0.1f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}};  // 3
        indices = {{0, 1, 2,
                    2, 3, 0}};
        type = meshTypes::SQUARE;
        m_vulkanMeshes.consume(type, squareVertices, indices);

        std::vector<float> starVertices = {{-0.1f, -0.05f, 1.0f, 1.0f, 1.0f, 0.0f, 0.25f,   // 0
                                            -0.04f, -0.05f, 1.0f, 1.0f, 1.0f, 0.3f, 0.25f,  // 1
                                            -0.06f, 0.0f, 1.0f, 1.0f, 1.0f, 0.2f, 0.5f,     // 2
                                            0.0f, -0.1f, 1.0f, 1.0f, 1.0f, 0.5f, 0.0f,      // 3
                                            0.04f, -0.05f, 1.0f, 1.0f, 1.0f, 0.7f, 0.25f,   // 4
                                            0.1f, -0.05f, 1.0f, 1.0f, 1.0f, 1.0f, 0.25f,    // 5
                                            0.06f, 0.0f, 1.0f, 1.0f, 1.0f, 0.8f, 0.5f,      // 6
                                            0.08f, 0.1f, 1.0f, 1.0f, 1.0f, 0.9f, 1.0f,      // 7
                                            0.0f, 0.02f, 1.0f, 1.0f, 1.0f, 0.5f, 0.6f,      // 8
                                            -0.08f, 0.1f, 1.0f, 1.0f, 1.0f, 0.1f, 1.0f}};   // 9
        indices = {{0, 1, 2,
                    1, 3, 4,
                    2, 1, 4,
                    4, 5, 6,
                    2, 4, 6,
                    6, 7, 8,
                    2, 6, 8,
                    2, 8, 9}};
        type = meshTypes::STAR;
        m_vulkanMeshes.consume(type, starVertices, indices);
        m_vulkanMeshes.finalize(m_vulkanDevice, m_vkMainCommandBuffer);

        std::unordered_map<meshTypes, std::string> filenames = {
            {meshTypes::TRIANGLE, "assets/textures/face.jpg"},
            {meshTypes::SQUARE, "assets/textures/haus.jpg"},
            {meshTypes::STAR, "assets/textures/noroi.png"},
        };

        m_vulkanSwapchain.createMeshDescriptorPool(m_vulkanDevice);

        for (const auto& [object, filename] : filenames) {
            m_materials[object] = new VulkanTexture(m_vulkanDevice,
                                                    filename,
                                                    m_vkMainCommandBuffer,
                                                    m_vulkanDevice.graphicsQueue(),
                                                    m_vulkanSwapchain.meshDescriptorSetLayout(),
                                                    m_vulkanSwapchain.meshDescriptorPool());
        }

        GN_CORE_INFO("Vulkan vertex buffer created.");
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
