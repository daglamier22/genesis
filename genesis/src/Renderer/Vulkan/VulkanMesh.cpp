#include "VulkanMesh.h"

#include "Core/Logger.h"

namespace Genesis {

    VulkanMesh::VulkanMesh() {
    }

    VulkanMesh::~VulkanMesh() {
    }

    vk::VertexInputBindingDescription VulkanMesh::getBindingDescription() {
        vk::VertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = 7 * sizeof(float);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;

        return bindingDescription;
    }

    std::vector<vk::VertexInputAttributeDescription> VulkanMesh::getAttributeDescriptions() {
        std::vector<vk::VertexInputAttributeDescription> attributeDescriptions{};
        // push dummy attribute to the list 3 times, so that we can still use absolute indexing below
        vk::VertexInputAttributeDescription dummy;
        attributeDescriptions.push_back(dummy);
        attributeDescriptions.push_back(dummy);
        attributeDescriptions.push_back(dummy);

        // Position
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[0].offset = 0;

        // Color
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[1].offset = 2 * sizeof(float);

        // TexCoord
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[2].offset = 5 * sizeof(float);

        return attributeDescriptions;
    }

    void VulkanMesh::createMesh(VulkanDevice& vulkanDevice, VulkanCommandBuffer& vulkanCommandBuffer, std::vector<float> vertices) {
        uint32_t bufferSize = sizeof(float) * vertices.size();

        VulkanBuffer stagingBuffer;
        stagingBuffer.createBuffer(vulkanDevice,
                                   bufferSize,
                                   vk::BufferUsageFlagBits::eTransferSrc,
                                   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        try {
            void* data = vulkanDevice.logicalDevice().mapMemory(stagingBuffer.memory(), vk::DeviceSize(0), bufferSize, vk::MemoryMapFlags());
            memcpy(data, vertices.data(), (size_t)bufferSize);
            vulkanDevice.logicalDevice().unmapMemory(stagingBuffer.memory());
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to copy memory: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        m_vertexBuffer.createBuffer(vulkanDevice,
                                    bufferSize,
                                    vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                                    vk::MemoryPropertyFlagBits::eDeviceLocal);

        m_vertexBuffer.copyBufferFrom(stagingBuffer.buffer(), bufferSize, vulkanDevice, vulkanCommandBuffer);

        vulkanDevice.logicalDevice().destroyBuffer(stagingBuffer.buffer());
        vulkanDevice.logicalDevice().freeMemory(stagingBuffer.memory());

        GN_CORE_INFO("Vulkan vertex buffer created.");
    }
}  // namespace Genesis
