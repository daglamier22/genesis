#pragma once

#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanTypes.h"

namespace Genesis {
    class VulkanMesh {
        public:
            VulkanMesh();
            ~VulkanMesh();

            VulkanBuffer const& vertexBuffer() const { return m_vertexBuffer; }

            static vk::VertexInputBindingDescription getBindingDescription();
            static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions();

            void createMesh(VulkanDevice& vulkanDevice, VulkanCommandBuffer& vulkanCommandBuffer, std::vector<float> vertices);

        private:
            VulkanBuffer m_vertexBuffer;
    };
}  // namespace Genesis
