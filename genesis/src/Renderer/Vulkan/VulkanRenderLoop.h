#pragma once

#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanTypes.h"

namespace Genesis {
    class VulkanRenderLoop {
        public:
            VulkanRenderLoop();
            ~VulkanRenderLoop();

            VulkanRenderLoop(const VulkanRenderLoop&) = delete;
            VulkanRenderLoop& operator=(const VulkanRenderLoop&) = delete;

            vk::CommandPool const& commandPool() const { return m_vkCommandPool; }
            vk::CommandBuffer const& mainCommandBuffer() const { return m_vkMainCommandBuffer; }

            void createCommandPool(VulkanDevice& vulkanDevice, vk::SurfaceKHR& surface);
            void createCommandBuffers(VulkanDevice& vulkandDevice, VulkanSwapchain& vulkanSwapchain);

        private:
            vk::CommandPool m_vkCommandPool;
            vk::CommandBuffer m_vkMainCommandBuffer;
    };
}  // namespace Genesis
