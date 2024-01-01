#pragma once

#include "VulkanDevice.h"
#include "VulkanTypes.h"

namespace Genesis {
    class VulkanCommandBuffer {
        public:
            VulkanCommandBuffer();
            ~VulkanCommandBuffer();

            vk::CommandBuffer const& commandBuffer() const { return m_vkCommandBuffer; }

            void create(VulkanDevice& vulkanDevice, vk::CommandPool commandPool);
            void beginSingleTimeCommands(VulkanDevice& vulkanDevice);
            void endSingleTimeCommands(VulkanDevice& vulkanDevice);

        private:
            vk::CommandBuffer m_vkCommandBuffer;
    };
}  // namespace Genesis
