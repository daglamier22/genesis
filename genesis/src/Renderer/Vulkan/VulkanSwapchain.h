#pragma once

#include "Platform/GLFWWindow.h"

namespace Genesis {
    class VulkanDevice;
    class VulkanPipeline;

    class VulkanSwapchain {
        public:
            VulkanSwapchain();
            ~VulkanSwapchain();

            VkSurfaceKHR getSurface() const { return m_vkSurface; }
            VkSwapchainKHR getSwapchain() const { return m_vkSwapchain; }
            std::vector<VkImage> getSwapchainImages() const { return m_vkSwapchainImages; }
            std::vector<VkImageView> getSwapchainImageViews() const { return m_vkSwapchainImageViews; }
            std::vector<VkFramebuffer> getSwapchainFramebuffers() const { return m_vkSwapchainFramebuffers; }
            VkFormat getSwapchainImageFormat() const { return m_vkSwapchainImageFormat; }
            VkExtent2D getSwapchainExtent() const { return m_vkSwapchainExtent; }

            bool createSurface(std::shared_ptr<GLFWWindow> window, VkInstance instance);
            bool createSwapChain(std::shared_ptr<GLFWWindow> window, VulkanDevice device);
            bool createImageViews(VulkanDevice device);
            bool createFramebuffers(VulkanDevice device, VulkanPipeline pipeline);

        private:
            VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
            VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
            VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, std::shared_ptr<GLFWWindow> window);

            VkSurfaceKHR m_vkSurface;
            VkSwapchainKHR m_vkSwapchain;
            std::vector<VkImage> m_vkSwapchainImages;
            std::vector<VkImageView> m_vkSwapchainImageViews;
            std::vector<VkFramebuffer> m_vkSwapchainFramebuffers;
            VkFormat m_vkSwapchainImageFormat;
            VkExtent2D m_vkSwapchainExtent;
    };
}  // namespace Genesis
