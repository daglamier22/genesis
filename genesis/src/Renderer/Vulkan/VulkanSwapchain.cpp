#include "VulkanSwapchain.h"

#include "Core/Logger.h"
#include "VulkanDevice.h"
#include "VulkanPipeline.h"

namespace Genesis {
    VulkanSwapchain::VulkanSwapchain() {
    }

    VulkanSwapchain::~VulkanSwapchain() {
    }

    bool VulkanSwapchain::createSurface(std::shared_ptr<GLFWWindow> window, VkInstance instance) {
        if (glfwCreateWindowSurface(instance, (GLFWwindow*)window->getWindow(), nullptr, &m_vkSurface) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to create window surface.");
            return false;
        }

        GN_CORE_INFO("Vulkan surface created successfully.");
        return true;
    }

    bool VulkanSwapchain::createSwapChain(std::shared_ptr<GLFWWindow> window, VulkanDevice device) {
        SwapChainSupportDetails swapChainSupport = device.querySwapChainSupport(device.getPhysicalDevice(), m_vkSurface);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

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

        QueueFamilyIndices indices = device.findQueueFamilies(device.getPhysicalDevice(), m_vkSurface);
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

        if (vkCreateSwapchainKHR(device.getDevice(), &createInfo, nullptr, &m_vkSwapchain) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to create swap chain.");
            return false;
        }

        vkGetSwapchainImagesKHR(device.getDevice(), m_vkSwapchain, &imageCount, nullptr);
        m_vkSwapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device.getDevice(), m_vkSwapchain, &imageCount, m_vkSwapchainImages.data());

        m_vkSwapchainImageFormat = surfaceFormat.format;
        m_vkSwapchainExtent = extent;

        GN_CORE_INFO("Vulkan swapchain created successfully.");
        return true;
    }

    bool VulkanSwapchain::createImageViews(VulkanDevice device) {
        m_vkSwapchainImageViews.resize(m_vkSwapchainImages.size());

        for (size_t i = 0; i < m_vkSwapchainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = m_vkSwapchainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = m_vkSwapchainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device.getDevice(), &createInfo, nullptr, &m_vkSwapchainImageViews[i]) != VK_SUCCESS) {
                GN_CORE_ERROR("Failed to create image views.");
                return false;
            }
        }

        GN_CORE_INFO("Image views created successfully.");
        return true;
    }

    bool VulkanSwapchain::createFramebuffers(VulkanDevice device, VulkanPipeline pipeline) {
        m_vkSwapchainFramebuffers.resize(m_vkSwapchainImageViews.size());

        for (size_t i = 0; i < m_vkSwapchainImageViews.size(); i++) {
            VkImageView attachments[] = {
                m_vkSwapchainImageViews[i]};

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = pipeline.getRenderPass();
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = m_vkSwapchainExtent.width;
            framebufferInfo.height = m_vkSwapchainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device.getDevice(), &framebufferInfo, nullptr, &m_vkSwapchainFramebuffers[i]) != VK_SUCCESS) {
                GN_CORE_ERROR("Failed to create framebuffer.");
                return false;
            }
        }

        GN_CORE_INFO("Vulkan framebuffers created successfully.");
        return true;
    }

    VkSurfaceFormatKHR VulkanSwapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR VulkanSwapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanSwapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, std::shared_ptr<GLFWWindow> window) {
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
}  // namespace Genesis
