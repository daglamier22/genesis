#include "VulkanRenderLoop.h"

#include "Core/Logger.h"

namespace Genesis {
    VulkanRenderLoop::VulkanRenderLoop() {
    }

    VulkanRenderLoop::~VulkanRenderLoop() {
    }

    void VulkanRenderLoop::createCommandPool(VulkanDevice& vulkanDevice, vk::SurfaceKHR& surface) {
        QueueFamilyIndices queueFamilyIndices = vulkanDevice.findQueueFamilies(vulkanDevice.physicalDevice(), surface);

        vk::CommandPoolCreateInfo poolInfo = {};
        poolInfo.flags = vk::CommandPoolCreateFlags() | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        try {
            m_vkCommandPool = vulkanDevice.logicalDevice().createCommandPool(poolInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create command pool: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        GN_CORE_INFO("Vulkan command pool created successfully.");
    }

    void VulkanRenderLoop::createCommandBuffers(VulkanDevice& vulkanDevice, VulkanSwapchain& vulkanSwapchain) {
        // create swapchain command buffers
        vulkanSwapchain.createCommandBuffers(vulkanDevice, m_vkCommandPool);

        // then create main command buffer
        vk::CommandBufferAllocateInfo allocInfo = {};
        allocInfo.commandPool = m_vkCommandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = 1;

        try {
            m_vkMainCommandBuffer = vulkanDevice.logicalDevice().allocateCommandBuffers(allocInfo)[0];
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to allocate command buffers: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        GN_CORE_INFO("Vulkan command buffer created successfully.");
    }
}  // namespace Genesis
