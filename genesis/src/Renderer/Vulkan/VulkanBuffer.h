#pragma once

#include "VulkanDevice.h"
#include "VulkanTypes.h"

namespace Genesis {
    class VulkanBuffer {
        public:
            VulkanBuffer();
            ~VulkanBuffer();

            vk::Buffer const& buffer() const { return m_vkBuffer; }
            vk::DeviceMemory const& memory() const { return m_vkBufferMemory; }

            void createBuffer(VulkanDevice& vulkanDevice,
                              vk::DeviceSize size,
                              vk::BufferUsageFlags usage,
                              vk::MemoryPropertyFlags properties);
            void copyBufferFrom(vk::Buffer srcBuffer, vk::DeviceSize size, VulkanDevice& vulkanDevice, vk::CommandBuffer commandBuffer);

        private:
            void beginSingleTimeCommands(VulkanDevice& vulkanDevice, vk::CommandBuffer commandBuffer);
            void endSingleTimeCommands(VulkanDevice& vulkanDevice, vk::CommandBuffer commandBuffer);

            vk::Buffer m_vkBuffer;
            vk::DeviceMemory m_vkBufferMemory;
    };
}  // namespace Genesis
