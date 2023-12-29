#pragma once

#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanTypes.h"

namespace Genesis {
    class VulkanVertexMenagerie {
        public:
            VulkanVertexMenagerie();
            ~VulkanVertexMenagerie();

            VulkanBuffer const& vertexBuffer() const { return m_vertexBuffer; }
            VulkanBuffer const& indexBuffer() const { return m_indexBuffer; }

            void consume(meshTypes type, std::vector<float> vertexData, std::vector<uint32_t> indexData);
            void finalize(VulkanDevice& vulkanDevice, vk::CommandBuffer commandBuffer);

            std::unordered_map<meshTypes, int> m_firstIndices;
            std::unordered_map<meshTypes, int> m_indexCounts;

        private:
            VulkanBuffer m_vertexBuffer;
            VulkanBuffer m_indexBuffer;
            int m_indexOffset;
            std::vector<float> m_vertexLump;
            std::vector<uint32_t> m_indexLump;
    };
}  // namespace Genesis
