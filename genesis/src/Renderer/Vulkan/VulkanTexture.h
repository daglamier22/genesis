#pragma once

#include <stb_image.h>

#include "VulkanImage.h"
#include "VulkanTypes.h"

namespace Genesis {
    class VulkanTexture {
        public:
            VulkanTexture(VulkanDevice& vulkanDevice,
                          std::string filename,
                          vk::CommandBuffer commandBuffer,
                          vk::Queue queue,
                          vk::DescriptorSetLayout layout,
                          vk::DescriptorPool descriptorPool);
            ~VulkanTexture();

            void use(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout);

        private:
            void load();
            void populate(VulkanDevice& vulkanDevice);
            void createTextureSampler(VulkanDevice& vulkanDevice);
            void makeDescriptorSet(VulkanDevice& vulkanDevice);
            // void generateMipmaps(vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

            vk::Device m_vkLogicalDevice;

            int m_width;
            int m_height;
            int m_channels;
            std::string m_filename;
            stbi_uc* m_pixels;

            uint32_t m_vkMipLevels = 1;
            VulkanImage m_textureImage;
            vk::Sampler m_vkSampler;

            vk::DescriptorSetLayout m_vkLayout;
            vk::DescriptorSet m_vkDescriptorSet;
            vk::DescriptorPool m_vkDescriptorPool;

            vk::CommandBuffer m_vkCommandBuffer;
            vk::Queue queue;
    };
}  // namespace Genesis
