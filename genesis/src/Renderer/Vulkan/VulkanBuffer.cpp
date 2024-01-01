#include "VulkanBuffer.h"

#include "Core/Logger.h"

namespace Genesis {
    VulkanBuffer::VulkanBuffer() {
    }

    VulkanBuffer::~VulkanBuffer() {
    }

    void VulkanBuffer::createBuffer(VulkanDevice& vulkanDevice,
                                    vk::DeviceSize size,
                                    vk::BufferUsageFlags usage,
                                    vk::MemoryPropertyFlags properties) {
        vk::BufferCreateInfo bufferInfo = {};
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        try {
            m_vkBuffer = vulkanDevice.logicalDevice().createBuffer(bufferInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create buffer: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        vk::MemoryRequirements memRequirements = vulkanDevice.logicalDevice().getBufferMemoryRequirements(m_vkBuffer);

        vk::MemoryAllocateInfo allocInfo = {};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = vulkanDevice.findMemoryType(memRequirements.memoryTypeBits, properties);

        try {
            m_vkBufferMemory = vulkanDevice.logicalDevice().allocateMemory(allocInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to allocate buffer memory: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        vulkanDevice.logicalDevice().bindBufferMemory(m_vkBuffer, m_vkBufferMemory, 0);
    }

    void VulkanBuffer::copyBufferFrom(vk::Buffer srcBuffer, vk::DeviceSize size, VulkanDevice& vulkanDevice, VulkanCommandBuffer& commandBuffer) {
        commandBuffer.beginSingleTimeCommands(vulkanDevice);

        vk::BufferCopy copyRegion = {};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;
        try {
            commandBuffer.commandBuffer().copyBuffer(srcBuffer, m_vkBuffer, 1, &copyRegion);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to copy buffer: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        commandBuffer.endSingleTimeCommands(vulkanDevice);
    }
}  // namespace Genesis
