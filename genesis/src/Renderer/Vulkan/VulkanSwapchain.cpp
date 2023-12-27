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

    void VulkanSwapchain::createDepthResources(VulkanDevice& vulkanDevice, vk::CommandPool commandPool) {
        m_vkDepthFormat = findDepthFormat(vulkanDevice);

        m_depthImage.createImage(vulkanDevice,
                                 extent().width,
                                 extent().height,
                                 1,
                                 vulkanDevice.msaaSamples(),
                                 m_vkDepthFormat,
                                 vk::ImageTiling::eOptimal,
                                 vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                 vk::MemoryPropertyFlagBits::eDeviceLocal);
        m_depthImage.createImageView(vulkanDevice,
                                     m_depthImage.image(),
                                     m_vkDepthFormat,
                                     vk::ImageAspectFlagBits::eDepth,
                                     1);

        m_depthImage.transitionImageLayout(vulkanDevice,
                                           m_depthImage.image(),
                                           m_vkDepthFormat,
                                           vk::ImageLayout::eUndefined,
                                           vk::ImageLayout::eDepthStencilAttachmentOptimal,
                                           1,
                                           commandPool);

        GN_CORE_INFO("Vulkan depth resources created successfully.");
    }

    void VulkanSwapchain::createFramebuffers(VulkanDevice& vulkanDevice, vk::RenderPass renderPass) {
        for (size_t i = 0; i < m_swapchainFrames.size(); i++) {
            std::array<vk::ImageView, 3> attachments = {m_colorImage.imageView(), m_depthImage.imageView(), m_swapchainFrames[i].vulkanImage.imageView()};

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
        vk::CommandBufferAllocateInfo allocInfo = {};
        allocInfo.commandPool = commandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = m_swapchainFrames.size();

        try {
            auto commandBuffers = vulkanDevice.logicalDevice().allocateCommandBuffers(allocInfo);
            for (auto i = 0; i < m_swapchainFrames.size(); ++i) {
                m_swapchainFrames[i].commandBuffer = commandBuffers[i];
            }
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to allocate command buffers: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        GN_CORE_INFO("Vulkan command buffer created successfully.");
    }

    void VulkanSwapchain::createSyncObjects(VulkanDevice& vulkanDevice) {
        for (size_t i = 0; i < m_swapchainFrames.size(); i++) {
            m_swapchainFrames[i].imageAvailableSemaphore = createSemaphore(vulkanDevice);
            m_swapchainFrames[i].renderFinishedSemaphore = createSemaphore(vulkanDevice);
            m_swapchainFrames[i].inFlightFence = createFence(vulkanDevice);
        }

        GN_CORE_INFO("Vulkan semaphores and fences created successfully.");
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

    void VulkanSwapchain::recreateSwapChain(VulkanDevice& vulkanDevice, const vk::SurfaceKHR& surface, std::shared_ptr<Window> window, vk::RenderPass renderpass, vk::CommandPool commandPool) {
        std::shared_ptr<GLFWWindow> glfwwindow = std::dynamic_pointer_cast<GLFWWindow>(window);
        int width = (int)glfwwindow->getWindowWidth();
        int height = (int)glfwwindow->getWindowHeight();
        while (width == 0 || height == 0) {
            glfwwindow->waitForWindowToBeRestored(&width, &height);
        }

        vulkanDevice.logicalDevice().waitIdle();

        cleanupSwapChain(vulkanDevice, commandPool);

        createSwapChain(vulkanDevice, surface, window);
        createImageViews(vulkanDevice);
        createColorResources(vulkanDevice);
        createDepthResources(vulkanDevice, commandPool);
        createFramebuffers(vulkanDevice, renderpass);
        createCommandBuffers(vulkanDevice, commandPool);
        createSyncObjects(vulkanDevice);

        GN_CORE_INFO("Vulkan swapchain recreated.");
    }

    void VulkanSwapchain::cleanupSwapChain(VulkanDevice& vulkanDevice, vk::CommandPool commandPool) {
        m_depthImage.destroyImageView(vulkanDevice);
        m_depthImage.destroyImage(vulkanDevice);
        m_depthImage.freeImageMemory(vulkanDevice);

        m_colorImage.destroyImageView(vulkanDevice);
        m_colorImage.destroyImage(vulkanDevice);
        m_colorImage.freeImageMemory(vulkanDevice);

        for (auto frame : m_swapchainFrames) {
            vulkanDevice.logicalDevice().destroyFence(frame.inFlightFence);
            vulkanDevice.logicalDevice().destroySemaphore(frame.renderFinishedSemaphore);
            vulkanDevice.logicalDevice().destroySemaphore(frame.imageAvailableSemaphore);
            vulkanDevice.logicalDevice().freeCommandBuffers(commandPool, frame.commandBuffer);
            vulkanDevice.logicalDevice().destroyFramebuffer(frame.framebuffer);
            frame.vulkanImage.destroyImageView(vulkanDevice);
        }

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

        GN_CORE_ERROR("Failed to find supported format.");
        return vk::Format();
    }

    vk::Format VulkanSwapchain::findDepthFormat(VulkanDevice& vulkanDevice) {
        return findSupportedFormat(vulkanDevice,
                                   {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
                                   vk::ImageTiling::eOptimal,
                                   vk::FormatFeatureFlagBits::eDepthStencilAttachment);
    }
}  // namespace Genesis
