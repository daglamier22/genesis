#include "VulkanVertexMenagerie.h"

#include "Core/Logger.h"

namespace Genesis {
    VulkanVertexMenagerie::VulkanVertexMenagerie() {
        m_indexOffset = 0;
    }

    VulkanVertexMenagerie::~VulkanVertexMenagerie() {
    }

    void VulkanVertexMenagerie::consume(meshTypes type, std::vector<float> vertexData, std::vector<uint32_t> indexData) {
        int vertexCount = static_cast<int>(vertexData.size() / 11);
        int indexCount = static_cast<int>(indexData.size());
        int lastIndex = static_cast<int>(m_indexLump.size());

        m_firstIndices.insert(std::make_pair(type, lastIndex));
        m_indexCounts.insert(std::make_pair(type, indexCount));

        for (float attribute : vertexData) {
            m_vertexLump.push_back(attribute);
        }
        for (uint32_t index : indexData) {
            m_indexLump.push_back(index + m_indexOffset);
        }

        m_indexOffset += vertexCount;
    }

    void VulkanVertexMenagerie::finalize(VulkanDevice& vulkanDevice, VulkanCommandBuffer& vulkanCommandBuffer) {
        // create staging buffer for vertices
        uint32_t bufferSize = sizeof(float) * m_vertexLump.size();
        VulkanBuffer stagingBuffer;
        stagingBuffer.createBuffer(vulkanDevice,
                                   bufferSize,
                                   vk::BufferUsageFlagBits::eTransferSrc,
                                   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        // fill staging buffer with vertex data
        try {
            void* data = vulkanDevice.logicalDevice().mapMemory(stagingBuffer.memory(), vk::DeviceSize(0), bufferSize, vk::MemoryMapFlags());
            memcpy(data, m_vertexLump.data(), (size_t)bufferSize);
            vulkanDevice.logicalDevice().unmapMemory(stagingBuffer.memory());
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to copy memory: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        // create vertex buffer
        m_vertexBuffer.createBuffer(vulkanDevice,
                                    bufferSize,
                                    vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                                    vk::MemoryPropertyFlagBits::eDeviceLocal);

        // fill vertex buffer by copying from staging
        m_vertexBuffer.copyBufferFrom(stagingBuffer.buffer(), bufferSize, vulkanDevice, vulkanCommandBuffer);

        // destroy staging buffer
        vulkanDevice.logicalDevice().destroyBuffer(stagingBuffer.buffer());
        vulkanDevice.logicalDevice().freeMemory(stagingBuffer.memory());

        // create staging buffer for indices
        bufferSize = sizeof(uint32_t) * m_indexLump.size();
        stagingBuffer.createBuffer(vulkanDevice,
                                   bufferSize,
                                   vk::BufferUsageFlagBits::eTransferSrc,
                                   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        // fill staging buffer with index data
        try {
            void* data = vulkanDevice.logicalDevice().mapMemory(stagingBuffer.memory(), vk::DeviceSize(0), bufferSize, vk::MemoryMapFlags());
            memcpy(data, m_indexLump.data(), (size_t)bufferSize);
            vulkanDevice.logicalDevice().unmapMemory(stagingBuffer.memory());
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to copy memory: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        // create index buffer
        m_indexBuffer.createBuffer(vulkanDevice,
                                   bufferSize,
                                   vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                                   vk::MemoryPropertyFlagBits::eDeviceLocal);

        // fill index buffer by copying from staging
        m_indexBuffer.copyBufferFrom(stagingBuffer.buffer(), bufferSize, vulkanDevice, vulkanCommandBuffer);

        // destroy staging buffer
        vulkanDevice.logicalDevice().destroyBuffer(stagingBuffer.buffer());
        vulkanDevice.logicalDevice().freeMemory(stagingBuffer.memory());

        m_vertexLump.clear();

        GN_CORE_INFO("Vulkan vertex buffer created.");
    }
}  // namespace Genesis
