#include "VulkanCommandBuffer.h"

#include "Core/Logger.h"

namespace Genesis {
    VulkanCommandBuffer::VulkanCommandBuffer() {
    }

    VulkanCommandBuffer::~VulkanCommandBuffer() {
    }

    void VulkanCommandBuffer::create(VulkanDevice& vulkanDevice, vk::CommandPool commandPool) {
        vk::CommandBufferAllocateInfo allocInfo = {};
        allocInfo.commandPool = commandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = 1;

        try {
            m_vkCommandBuffer = vulkanDevice.logicalDevice().allocateCommandBuffers(allocInfo)[0];
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to allocate command buffers: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        GN_CORE_INFO("Vulkan command buffer created successfully.");
    }

    void VulkanCommandBuffer::beginSingleTimeCommands(VulkanDevice& vulkanDevice) {
        m_vkCommandBuffer.reset();

        vk::CommandBufferBeginInfo beginInfo = {};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        try {
            m_vkCommandBuffer.begin(beginInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Unable to being recording single use command buffer: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }
    }

    void VulkanCommandBuffer::endSingleTimeCommands(VulkanDevice& vulkanDevice) {
        try {
            m_vkCommandBuffer.end();
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to record single use command buffer: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        vk::SubmitInfo submitInfo = {};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_vkCommandBuffer;

        try {
            auto result = vulkanDevice.graphicsQueue().submit(1, &submitInfo, nullptr);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to submit sungle use command buffer: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        vulkanDevice.graphicsQueue().waitIdle();
    }
}  // namespace Genesis
