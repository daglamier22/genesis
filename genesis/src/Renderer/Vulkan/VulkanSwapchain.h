#pragma once

#include "Platform/GLFWWindow.h"
#include "VulkanDevice.h"
#include "VulkanImage.h"
#include "VulkanTypes.h"

namespace Genesis {
    class VulkanSwapchain {
        public:
            VulkanSwapchain();
            ~VulkanSwapchain();

            VulkanSwapchain(const VulkanSwapchain&) = delete;
            VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

            vk::SwapchainKHR const& swapchain() const { return m_vkSwapchain; }
            std::vector<vk::Framebuffer> const& framebuffers() const { return m_vkSwapchainFramebuffers; }
            vk::Format const& format() const { return m_vkSwapchainImageFormat; }
            vk::Extent2D const& extent() const { return m_vkSwapchainExtent; }
            vk::Format const& depthFormat() const { return m_vkDepthFormat; }

            void createSwapChain(VulkanDevice& vulkanDevice, const vk::SurfaceKHR& surface, std::shared_ptr<Window> window);
            void createImageViews(VulkanDevice& vulkanDevice);
            void createColorResources(VulkanDevice& vulkanDevice);
            void createDepthResources(VulkanDevice& vulkanDevice, vk::CommandPool commandPool);
            void createFramebuffers(VulkanDevice& vulkanDevice, vk::RenderPass renderPass);
            void recreateSwapChain(VulkanDevice& vulkanDevice,
                                   const vk::SurfaceKHR& surface,
                                   std::shared_ptr<Window> window,
                                   vk::RenderPass renderpass,
                                   vk::CommandPool commandPool);
            void cleanupSwapChain(VulkanDevice& vulkanDevice);
            vk::Format findSupportedFormat(VulkanDevice& vulkanDevice,
                                           const std::vector<vk::Format>& candidates,
                                           vk::ImageTiling tiling,
                                           vk::FormatFeatureFlags features);
            vk::Format findDepthFormat(VulkanDevice& vulkanDevice);

        private:
            vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
            vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
            vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, std::shared_ptr<Window> window);

            vk::SwapchainKHR m_vkSwapchain;
            std::vector<VulkanImage> m_swapchainImages;
            std::vector<vk::Framebuffer> m_vkSwapchainFramebuffers;
            vk::Format m_vkSwapchainImageFormat;
            vk::Extent2D m_vkSwapchainExtent;

            VulkanImage m_colorImage;
            VulkanImage m_depthImage;
            vk::Format m_vkDepthFormat;
    };
}  // namespace Genesis
