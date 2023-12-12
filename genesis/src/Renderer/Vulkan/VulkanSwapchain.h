#pragma once

#include "Platform/GLFWWindow.h"
#include "VulkanDevice.h"

namespace Genesis {
    class VulkanSwapchain {
        public:
            VulkanSwapchain();
            ~VulkanSwapchain();

            VkSurfaceKHR getSurface() const { return m_vkSurface; }
            VkSwapchainKHR getSwapchain() const { return m_vkSwapchain; }
            std::vector<VkImage> getSwapchainImages() const { return m_vkSwapchainImages; }
            std::vector<VkImageView> getSwapchainImageViews() const { return m_vkSwapchainImageViews; }

            bool createSurface(std::shared_ptr<GLFWWindow> window, VkInstance instance);
            bool createSwapChain(std::shared_ptr<GLFWWindow> window, VulkanDevice device);
            bool createImageViews(VulkanDevice device);

        private:
            VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
            VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
            VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, std::shared_ptr<GLFWWindow> window);

            VkSurfaceKHR m_vkSurface;
            VkSwapchainKHR m_vkSwapchain;
            std::vector<VkImage> m_vkSwapchainImages;
            std::vector<VkImageView> m_vkSwapchainImageViews;
            VkFormat m_vkSwapchainImageFormat;
            VkExtent2D m_vkSwapchainExtent;
    };
}  // namespace Genesis
