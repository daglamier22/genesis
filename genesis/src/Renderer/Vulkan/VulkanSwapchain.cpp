#include "VulkanSwapchain.h"

#include "Core/Logger.h"
#include "Platform/GLFWWindow.h"

namespace Genesis {
    VulkanSwapchain::VulkanSwapchain() {
    }

    VulkanSwapchain::~VulkanSwapchain() {
    }

    void VulkanSwapchain::createSwapChain(VulkanDevice& vulkanDevice, const vk::SurfaceKHR& surface, std::shared_ptr<Window> window) {
        SwapChainSupportDetails swapChainSupport = vulkanDevice.querySwapChainSupport(vulkanDevice.physicalDevice(), surface);

        vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR createInfo = vk::SwapchainCreateInfoKHR(vk::SwapchainCreateFlagsKHR(),
                                                                           surface,
                                                                           imageCount,
                                                                           surfaceFormat.format,
                                                                           surfaceFormat.colorSpace,
                                                                           extent,
                                                                           1,
                                                                           vk::ImageUsageFlagBits::eColorAttachment);

        QueueFamilyIndices indices = vulkanDevice.findQueueFamilies(vulkanDevice.physicalDevice(), surface);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = vk::SharingMode::eExclusive;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        createInfo.presentMode = presentMode;
        createInfo.clipped = true;

        createInfo.oldSwapchain = vk::SwapchainKHR(nullptr);

        try {
            m_vkSwapchain = vulkanDevice.logicalDevice().createSwapchainKHR(createInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create swap chain: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        m_vkSwapchainImageFormat = surfaceFormat.format;
        m_vkSwapchainExtent = extent;

        GN_CORE_INFO("Vulkan swapchain created successfully.");
    }

    void VulkanSwapchain::createFrameResources(VulkanDevice& vulkanDevice,
                                               vk::RenderPass renderpass,
                                               vk::CommandPool commandPool,
                                               VulkanCommandBuffer& vulkanCommandBuffer) {
        createImageViews(vulkanDevice);
        createColorResources(vulkanDevice);
        createDepthResources(vulkanDevice, vulkanCommandBuffer);
        createFramebuffers(vulkanDevice, renderpass);
        createCommandBuffers(vulkanDevice, commandPool);
        createSyncObjects(vulkanDevice);
        createDescriptorResources(vulkanDevice);
        createFrameDescriptorPool(vulkanDevice);
        createDescriptorSets(vulkanDevice);
    }

    void VulkanSwapchain::createDescriptorSetLayouts(VulkanDevice& vulkanDevice) {
        vk::DescriptorSetLayoutBinding uboLayoutBinding = {};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
        uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

        vk::DescriptorSetLayoutBinding storageBufferLayoutBinding = {};
        storageBufferLayoutBinding.binding = 1;
        storageBufferLayoutBinding.descriptorCount = 1;
        storageBufferLayoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
        storageBufferLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

        std::array<vk::DescriptorSetLayoutBinding, 2> frameBindings = {uboLayoutBinding, storageBufferLayoutBinding};
        vk::DescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.flags = vk::DescriptorSetLayoutCreateFlags();
        layoutInfo.bindingCount = static_cast<uint32_t>(frameBindings.size());
        layoutInfo.pBindings = frameBindings.data();

        try {
            m_vkFrameDescriptorSetLayout = vulkanDevice.logicalDevice().createDescriptorSetLayout(layoutInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create frame descriptor set layout: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        vk::DescriptorSetLayoutBinding samplerLayoutBinding = {};
        samplerLayoutBinding.binding = 0;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

        std::array<vk::DescriptorSetLayoutBinding, 1> meshBindings = {samplerLayoutBinding};
        layoutInfo.flags = vk::DescriptorSetLayoutCreateFlags();
        layoutInfo.bindingCount = static_cast<uint32_t>(meshBindings.size());
        layoutInfo.pBindings = meshBindings.data();

        try {
            m_vkMeshDescriptorSetLayout = vulkanDevice.logicalDevice().createDescriptorSetLayout(layoutInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create mesh descriptor set layout: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        GN_CORE_INFO("Vulkan descriptor set layouts created successfully.");
    }

    void VulkanSwapchain::createFrameDescriptorPool(VulkanDevice& vulkanDevice) {
        std::array<vk::DescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(m_swapchainFrames.size());
        poolSizes[1].type = vk::DescriptorType::eStorageBuffer;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(m_swapchainFrames.size());
        // poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
        // poolSizes[1].descriptorCount = static_cast<uint32_t>(m_maxFramesInFlight);

        vk::DescriptorPoolCreateInfo poolInfo = {};
        poolInfo.flags = vk::DescriptorPoolCreateFlags();
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(m_swapchainFrames.size());

        try {
            m_vkFrameDescriptorPool = vulkanDevice.logicalDevice().createDescriptorPool(poolInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create frame descriptor pool: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        GN_CORE_INFO("Vulkan frame descriptor pool created successfully.");
    }

    void VulkanSwapchain::createMeshDescriptorPool(VulkanDevice& vulkanDevice) {
        std::array<vk::DescriptorPoolSize, 1> poolSizes{};
        poolSizes[0].type = vk::DescriptorType::eCombinedImageSampler;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(m_swapchainFrames.size());

        vk::DescriptorPoolCreateInfo poolInfo = {};
        poolInfo.flags = vk::DescriptorPoolCreateFlags();
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(m_swapchainFrames.size());

        try {
            m_vkMeshDescriptorPool = vulkanDevice.logicalDevice().createDescriptorPool(poolInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create mesh descriptor pool: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        GN_CORE_INFO("Vulkan mesh descriptor pool created successfully.");
    }

    void VulkanSwapchain::createImageViews(VulkanDevice& vulkanDevice) {
        std::vector<vk::Image> images = vulkanDevice.logicalDevice().getSwapchainImagesKHR(m_vkSwapchain);
        m_swapchainFrames.resize(images.size());
        for (size_t i = 0; i < images.size(); ++i) {
            m_swapchainFrames[i].vulkanImage.setImage(images[i]);
            m_swapchainFrames[i].vulkanImage.createImageView(vulkanDevice, images[i], m_vkSwapchainImageFormat, vk::ImageAspectFlagBits::eColor, 1);
        }
    }

    void VulkanSwapchain::createColorResources(VulkanDevice& vulkanDevice) {
        vk::Format colorFormat = m_vkSwapchainImageFormat;
        m_colorImage.createImage(vulkanDevice,
                                 extent().width,
                                 extent().height,
                                 1,
                                 vulkanDevice.msaaSamples(),
                                 colorFormat,
                                 vk::ImageTiling::eOptimal,
                                 vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
                                 vk::MemoryPropertyFlagBits::eDeviceLocal);
        m_colorImage.createImageView(vulkanDevice,
                                     m_colorImage.image(),
                                     colorFormat,
                                     vk::ImageAspectFlagBits::eColor,
                                     1);

        GN_CORE_INFO("Vulkan color resources created successfuly.");
    }

    void VulkanSwapchain::createDepthResources(VulkanDevice& vulkanDevice, VulkanCommandBuffer& vulkanCommandBuffer) {
        vk::Format depthFormat = findDepthFormat(vulkanDevice);

        for (size_t i = 0; i < m_swapchainFrames.size(); ++i) {
            m_swapchainFrames[i].depthFormat = depthFormat;
            m_swapchainFrames[i].depthBuffer.createImage(vulkanDevice,
                                                         extent().width,
                                                         extent().height,
                                                         1,
                                                         vulkanDevice.msaaSamples(),
                                                         depthFormat,
                                                         vk::ImageTiling::eOptimal,
                                                         vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                                         vk::MemoryPropertyFlagBits::eDeviceLocal);
            m_swapchainFrames[i].depthBuffer.createImageView(vulkanDevice,
                                                             m_swapchainFrames[i].depthBuffer.image(),
                                                             depthFormat,
                                                             vk::ImageAspectFlagBits::eDepth,
                                                             1);

            m_swapchainFrames[i].depthBuffer.transitionImageLayout(vulkanDevice,
                                                                   m_swapchainFrames[i].depthBuffer.image(),
                                                                   depthFormat,
                                                                   vk::ImageLayout::eUndefined,
                                                                   vk::ImageLayout::eDepthStencilAttachmentOptimal,
                                                                   1,
                                                                   vulkanCommandBuffer);
        }

        GN_CORE_INFO("Vulkan depth resources created successfully.");
    }

    void VulkanSwapchain::createFramebuffers(VulkanDevice& vulkanDevice, vk::RenderPass renderPass) {
        for (size_t i = 0; i < m_swapchainFrames.size(); i++) {
            std::array<vk::ImageView, 3> attachments = {m_colorImage.imageView(), m_swapchainFrames[i].depthBuffer.imageView(), m_swapchainFrames[i].vulkanImage.imageView()};

            vk::FramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = m_vkSwapchainExtent.width;
            framebufferInfo.height = m_vkSwapchainExtent.height;
            framebufferInfo.layers = 1;

            try {
                m_swapchainFrames[i].framebuffer = vulkanDevice.logicalDevice().createFramebuffer(framebufferInfo);
            } catch (vk::SystemError err) {
                std::string errMsg = "Failed to create framebuffer: ";
                GN_CORE_ERROR("{}{}", errMsg, err.what());
                throw std::runtime_error(errMsg + err.what());
            }
        }

        GN_CORE_INFO("Vulkan framebuffers created successfully.");
    }

    void VulkanSwapchain::createCommandBuffers(VulkanDevice& vulkanDevice, vk::CommandPool& commandPool) {
        for (size_t i = 0; i < m_swapchainFrames.size(); ++i) {
            m_swapchainFrames[i].vulkanCommandBuffer.create(vulkanDevice, commandPool);
        }
    }

    void VulkanSwapchain::createSyncObjects(VulkanDevice& vulkanDevice) {
        for (size_t i = 0; i < m_swapchainFrames.size(); i++) {
            m_swapchainFrames[i].imageAvailableSemaphore = createSemaphore(vulkanDevice);
            m_swapchainFrames[i].renderFinishedSemaphore = createSemaphore(vulkanDevice);
            m_swapchainFrames[i].inFlightFence = createFence(vulkanDevice);
        }

        GN_CORE_INFO("Vulkan semaphores and fences created successfully.");
    }

    void VulkanSwapchain::createDescriptorResources(VulkanDevice& vulkanDevice) {
        vk::DeviceSize cameraBufferSize = sizeof(UniformBufferObject);
        vk::DeviceSize storageBufferSize = 1024 * sizeof(glm::mat4);

        for (size_t i = 0; i < m_swapchainFrames.size(); i++) {
            m_swapchainFrames[i].cameraDataBuffer.createBuffer(vulkanDevice,
                                                               cameraBufferSize,
                                                               vk::BufferUsageFlagBits::eUniformBuffer,
                                                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
            m_swapchainFrames[i].cameraDataWriteLocation = vulkanDevice.logicalDevice().mapMemory(m_swapchainFrames[i].cameraDataBuffer.memory(),
                                                                                                  vk::DeviceSize(0),
                                                                                                  cameraBufferSize,
                                                                                                  vk::MemoryMapFlags());
            m_swapchainFrames[i].modelBuffer.createBuffer(vulkanDevice,
                                                          storageBufferSize,
                                                          vk::BufferUsageFlagBits::eStorageBuffer,
                                                          vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
            m_swapchainFrames[i].modelBufferWriteLocation = vulkanDevice.logicalDevice().mapMemory(m_swapchainFrames[i].modelBuffer.memory(),
                                                                                                   vk::DeviceSize(0),
                                                                                                   storageBufferSize,
                                                                                                   vk::MemoryMapFlags());
            m_swapchainFrames[i].modelTransforms.reserve(1024);
            for (int j = 0; j < 1024; ++j) {
                m_swapchainFrames[i].modelTransforms.push_back(glm::mat4(1.0f));
            }

            m_swapchainFrames[i].uniformBufferDescriptor.buffer = m_swapchainFrames[i].cameraDataBuffer.buffer();
            m_swapchainFrames[i].uniformBufferDescriptor.offset = 0;
            m_swapchainFrames[i].uniformBufferDescriptor.range = sizeof(UniformBufferObject);

            m_swapchainFrames[i].modelBufferDescriptor.buffer = m_swapchainFrames[i].modelBuffer.buffer();
            m_swapchainFrames[i].modelBufferDescriptor.offset = 0;
            m_swapchainFrames[i].modelBufferDescriptor.range = 1024 * sizeof(glm::mat4);
        }

        GN_CORE_INFO("Vulkan uniform buffers created successfully.");
    }

    void VulkanSwapchain::prepareFrame(VulkanDevice& vulkanDevice, uint32_t imageIndex, std::shared_ptr<Scene> scene) {
        // static auto startTime = std::chrono::high_resolution_clock::now();

        SwapChainFrame frame = m_swapchainFrames[imageIndex];

        // auto currentTime = std::chrono::high_resolution_clock::now();
        // float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        glm::vec3 eye = {0.0f, 0.0f, 1.0f};
        glm::vec3 center = {1.0f, 0.0f, 1.0f};
        glm::vec3 up = {0.0f, 0.0f, 1.0f};
        ubo.view = glm::lookAt(eye, center, up);
        ubo.projection = glm::perspective(glm::radians(45.0f), m_vkSwapchainExtent.width / (float)m_vkSwapchainExtent.height, 0.1f, 100.0f);
        ubo.projection[1][1] *= -1;
        // ubo.viewProjection = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

        frame.cameraData.view = ubo.view;
        frame.cameraData.projection = ubo.projection;
        frame.cameraData.viewProjection = ubo.projection * ubo.view;
        memcpy(frame.cameraDataWriteLocation, &frame.cameraData, sizeof(UniformBufferObject));

        size_t i = 0;
        for (auto pair : scene->positions) {
            for (glm::vec3& position : pair.second) {
                frame.modelTransforms[i++] = glm::translate(glm::mat4(1.0f), position);
            }
        }
        memcpy(frame.modelBufferWriteLocation, frame.modelTransforms.data(), i * sizeof(glm::mat4));

        writeDescriptorSets(vulkanDevice, imageIndex);
    }

    vk::Semaphore VulkanSwapchain::createSemaphore(VulkanDevice& vulkanDevice) {
        vk::SemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.flags = vk::SemaphoreCreateFlags();

        try {
            return vulkanDevice.logicalDevice().createSemaphore(semaphoreInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create semaphore: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }
    }

    vk::Fence VulkanSwapchain::createFence(VulkanDevice& vulkanDevice) {
        vk::FenceCreateInfo fenceInfo = {};
        fenceInfo.flags = vk::FenceCreateFlags() | vk::FenceCreateFlagBits::eSignaled;

        try {
            return vulkanDevice.logicalDevice().createFence(fenceInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create fence: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }
    }

    void VulkanSwapchain::createDescriptorSets(VulkanDevice& vulkanDevice) {
        std::vector<vk::DescriptorSetLayout> layouts(m_swapchainFrames.size(), m_vkFrameDescriptorSetLayout);
        vk::DescriptorSetAllocateInfo allocInfo = {};
        allocInfo.descriptorPool = m_vkFrameDescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(m_swapchainFrames.size());
        allocInfo.pSetLayouts = layouts.data();

        std::vector<vk::DescriptorSet> descriptorSets;
        descriptorSets.resize(m_swapchainFrames.size());
        try {
            descriptorSets = vulkanDevice.logicalDevice().allocateDescriptorSets(allocInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to allocate descriptor sets: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        for (size_t i = 0; i < m_swapchainFrames.size(); i++) {
            m_swapchainFrames[i].descriptorSet = descriptorSets[i];

            // m_swapchainFrames[i].uniformBufferDescriptor.buffer = m_swapchainFrames[i].cameraDataBuffer.buffer();
            // m_swapchainFrames[i].uniformBufferDescriptor.offset = 0;
            // m_swapchainFrames[i].uniformBufferDescriptor.range = sizeof(UniformBufferObject);
        }

        GN_CORE_INFO("Vulkan descriptor sets created successfully.");
    }

    void VulkanSwapchain::writeDescriptorSets(VulkanDevice& vulkanDevice, uint32_t imageIndex) {
        // vk::DescriptorImageInfo imageInfo = {};
        // imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        // imageInfo.imageView = m_textureImage.imageView();
        // imageInfo.sampler = m_vkTextureSampler;

        std::array<vk::WriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites[0].dstSet = m_swapchainFrames[imageIndex].descriptorSet;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &m_swapchainFrames[imageIndex].uniformBufferDescriptor;

        descriptorWrites[1].dstSet = m_swapchainFrames[imageIndex].descriptorSet;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = vk::DescriptorType::eStorageBuffer;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &m_swapchainFrames[imageIndex].modelBufferDescriptor;

        // descriptorWrites[1].dstSet = m_vkDescriptorSets[i];
        // descriptorWrites[1].dstBinding = 1;
        // descriptorWrites[1].dstArrayElement = 0;
        // descriptorWrites[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        // descriptorWrites[1].descriptorCount = 1;
        // descriptorWrites[1].pImageInfo = &imageInfo;

        try {
            vulkanDevice.logicalDevice().updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to update descriptor sets: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        GN_CORE_TRACE2("Vulkan descriptor sets written successfully.");
    }

    vk::SurfaceFormatKHR VulkanSwapchain::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    vk::PresentModeKHR VulkanSwapchain::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == vk::PresentModeKHR::eFifoRelaxed) {  // NOTE: consider switching to eMailbox later
                return availablePresentMode;
            }
        }

        return vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D VulkanSwapchain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, std::shared_ptr<Window> window) {
        std::shared_ptr<GLFWWindow> glfwwindow = std::dynamic_pointer_cast<GLFWWindow>(window);
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize((GLFWwindow*)glfwwindow->getWindow(), &width, &height);

            vk::Extent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)};

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    void VulkanSwapchain::recreateSwapChain(VulkanDevice& vulkanDevice,
                                            const vk::SurfaceKHR& surface,
                                            std::shared_ptr<Window> window,
                                            vk::RenderPass renderPass,
                                            vk::CommandPool commandPool,
                                            VulkanCommandBuffer& vulkanCommandBuffer) {
        std::shared_ptr<GLFWWindow> glfwwindow = std::dynamic_pointer_cast<GLFWWindow>(window);
        int width = (int)glfwwindow->getWindowWidth();
        int height = (int)glfwwindow->getWindowHeight();
        while (width == 0 || height == 0) {
            glfwwindow->waitForWindowToBeRestored(&width, &height);
        }

        vulkanDevice.logicalDevice().waitIdle();

        cleanupSwapChain(vulkanDevice, commandPool);

        createSwapChain(vulkanDevice, surface, window);
        createFrameResources(vulkanDevice, renderPass, commandPool, vulkanCommandBuffer);

        GN_CORE_INFO("Vulkan swapchain recreated.");
    }

    void VulkanSwapchain::cleanupSwapChain(VulkanDevice& vulkanDevice, vk::CommandPool commandPool) {
        m_colorImage.destroyImageView(vulkanDevice);
        m_colorImage.destroyImage(vulkanDevice);
        m_colorImage.freeImageMemory(vulkanDevice);

        for (auto frame : m_swapchainFrames) {
            vulkanDevice.logicalDevice().destroyImageView(frame.depthBuffer.imageView());
            vulkanDevice.logicalDevice().destroyImage(frame.depthBuffer.image());
            vulkanDevice.logicalDevice().freeMemory(frame.depthBuffer.imageMemory());

            vulkanDevice.logicalDevice().unmapMemory(frame.cameraDataBuffer.memory());
            vulkanDevice.logicalDevice().freeMemory(frame.cameraDataBuffer.memory());
            vulkanDevice.logicalDevice().destroyBuffer(frame.cameraDataBuffer.buffer());

            vulkanDevice.logicalDevice().unmapMemory(frame.modelBuffer.memory());
            vulkanDevice.logicalDevice().freeMemory(frame.modelBuffer.memory());
            vulkanDevice.logicalDevice().destroyBuffer(frame.modelBuffer.buffer());

            vulkanDevice.logicalDevice().destroyFence(frame.inFlightFence);
            vulkanDevice.logicalDevice().destroySemaphore(frame.renderFinishedSemaphore);
            vulkanDevice.logicalDevice().destroySemaphore(frame.imageAvailableSemaphore);

            vulkanDevice.logicalDevice().freeCommandBuffers(commandPool, frame.vulkanCommandBuffer.commandBuffer());
            vulkanDevice.logicalDevice().destroyFramebuffer(frame.framebuffer);
            frame.vulkanImage.destroyImageView(vulkanDevice);
        }

        vulkanDevice.logicalDevice().destroyDescriptorPool(m_vkFrameDescriptorPool);

        vulkanDevice.logicalDevice().destroySwapchainKHR(m_vkSwapchain);
    }

    vk::Format VulkanSwapchain::findSupportedFormat(VulkanDevice& vulkanDevice,
                                                    const std::vector<vk::Format>& candidates,
                                                    vk::ImageTiling tiling,
                                                    vk::FormatFeatureFlags features) {
        for (vk::Format format : candidates) {
            vk::FormatProperties props = vulkanDevice.physicalDevice().getFormatProperties(format);

            if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        std::string errMsg = "Failed to find supported format.";
        GN_CORE_ERROR("{}", errMsg);
        throw std::runtime_error(errMsg);
    }

    vk::Format VulkanSwapchain::findDepthFormat(VulkanDevice& vulkanDevice) {
        return findSupportedFormat(vulkanDevice,
                                   {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
                                   vk::ImageTiling::eOptimal,
                                   vk::FormatFeatureFlagBits::eDepthStencilAttachment);
    }
}  // namespace Genesis
