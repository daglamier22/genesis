#pragma once

#include "Core/Scene.h"
#include "Platform/GLFWWindow.h"
#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanImage.h"
#include "VulkanTypes.h"

namespace Genesis {
    struct SwapChainFrame {
            VulkanImage vulkanImage;
            vk::Framebuffer framebuffer;

            vk::CommandBuffer commandBuffer;

            vk::Semaphore imageAvailableSemaphore;
            vk::Semaphore renderFinishedSemaphore;
            vk::Fence inFlightFence;

            UniformBufferObject cameraData;
            VulkanBuffer cameraDataBuffer;
            void* cameraDataWriteLocation;
            std::vector<glm::mat4> modelTransforms;
            VulkanBuffer modelBuffer;
            void* modelBufferWriteLocation;

            vk::DescriptorBufferInfo uniformBufferDescriptor;
            vk::DescriptorBufferInfo modelBufferDescriptor;
            vk::DescriptorSet descriptorSet;
    };

    class VulkanSwapchain {
        public:
            VulkanSwapchain();
            ~VulkanSwapchain();

            VulkanSwapchain(const VulkanSwapchain&) = delete;
            VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

            vk::SwapchainKHR const& swapchain() const { return m_vkSwapchain; }
            std::vector<SwapChainFrame> const& swapchainFrames() { return m_swapchainFrames; }
            vk::Format const& format() const { return m_vkSwapchainImageFormat; }
            vk::Extent2D const& extent() const { return m_vkSwapchainExtent; }
            vk::Format const& depthFormat() const { return m_vkDepthFormat; }
            vk::DescriptorSetLayout const& frameDescriptorSetLayout() const { return m_vkFrameDescriptorSetLayout; }
            vk::DescriptorPool const& frameDescriptorPool() const { return m_vkFrameDescriptorPool; }
            vk::DescriptorSetLayout const& meshDescriptorSetLayout() const { return m_vkMeshDescriptorSetLayout; }
            vk::DescriptorPool const& meshDescriptorPool() const { return m_vkMeshDescriptorPool; }

            void createSwapChain(VulkanDevice& vulkanDevice, const vk::SurfaceKHR& surface, std::shared_ptr<Window> window);
            void createFrameResources(VulkanDevice& vulkanDevice, vk::RenderPass renderPass, vk::CommandPool commandPool, vk::CommandBuffer commandBuffer);
            void createDescriptorSetLayouts(VulkanDevice& vulkanDevice);
            void createFrameDescriptorPool(VulkanDevice& vulkanDevice);
            void createMeshDescriptorPool(VulkanDevice& vulkanDevice);
            void prepareFrame(VulkanDevice& vulkanDevice, uint32_t imageIndex, std::shared_ptr<Scene> scene);
            void writeDescriptorSets(VulkanDevice& vulkanDevice, uint32_t imageIndex);

            void recreateSwapChain(VulkanDevice& vulkanDevice,
                                   const vk::SurfaceKHR& surface,
                                   std::shared_ptr<Window> window,
                                   vk::RenderPass renderpass,
                                   vk::CommandPool commandPool,
                                   vk::CommandBuffer commandBuffer);
            void cleanupSwapChain(VulkanDevice& vulkanDevice, vk::CommandPool commandPool);
            vk::Format findSupportedFormat(VulkanDevice& vulkanDevice,
                                           const std::vector<vk::Format>& candidates,
                                           vk::ImageTiling tiling,
                                           vk::FormatFeatureFlags features);
            vk::Format findDepthFormat(VulkanDevice& vulkanDevice);

        private:
            void createImageViews(VulkanDevice& vulkanDevice);
            void createColorResources(VulkanDevice& vulkanDevice);
            void createDepthResources(VulkanDevice& vulkanDevice, vk::CommandBuffer commandBuffer);
            void createFramebuffers(VulkanDevice& vulkanDevice, vk::RenderPass renderPass);
            void createCommandBuffers(VulkanDevice& vulkanDevice, vk::CommandPool& commandPool);
            void createSyncObjects(VulkanDevice& vulkanDevice);
            void createDescriptorResources(VulkanDevice& vulkanDevice);
            void createDescriptorSets(VulkanDevice& vulkanDevice);

            vk::Semaphore createSemaphore(VulkanDevice& vulkanDevice);
            vk::Fence createFence(VulkanDevice& vulkanDevice);
            vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
            vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
            vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, std::shared_ptr<Window> window);

            vk::SwapchainKHR m_vkSwapchain;
            vk::Format m_vkSwapchainImageFormat;
            vk::Extent2D m_vkSwapchainExtent;

            std::vector<SwapChainFrame> m_swapchainFrames;

            vk::DescriptorPool m_vkFrameDescriptorPool;
            vk::DescriptorSetLayout m_vkFrameDescriptorSetLayout;
            vk::DescriptorPool m_vkMeshDescriptorPool;
            vk::DescriptorSetLayout m_vkMeshDescriptorSetLayout;

            VulkanImage m_colorImage;
            VulkanImage m_depthImage;
            vk::Format m_vkDepthFormat;
    };
}  // namespace Genesis
