#pragma once

#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"
#include "VulkanTypes.h"

namespace Genesis {
    class VulkanImage {
        public:
            VulkanImage();
            ~VulkanImage();

            vk::Image const& image() const { return m_vkImage; }
            vk::ImageView const& imageView() const { return m_vkImageView; }
            vk::DeviceMemory const& imageMemory() const { return m_vkImageMemory; }

            void setImage(vk::Image image);
            void createImage(VulkanDevice& vulkanDevice,
                             uint32_t width,
                             uint32_t height,
                             uint32_t mipLevels,
                             vk::SampleCountFlagBits numSamples,
                             vk::Format format,
                             vk::ImageTiling tiling,
                             vk::ImageUsageFlags usage,
                             vk::MemoryPropertyFlags properties);
            void createImageView(VulkanDevice& device,
                                 vk::Image image,
                                 vk::Format format,
                                 vk::ImageAspectFlags aspectFlags,
                                 uint32_t mipLevels);
            void transitionImageLayout(VulkanDevice& vulkanDevice,
                                       vk::Image image,
                                       vk::Format format,
                                       vk::ImageLayout oldLayout,
                                       vk::ImageLayout newLayout,
                                       uint32_t mipLevels,
                                       VulkanCommandBuffer& vulkanCommandBuffer);
            void copyBufferToImage(VulkanDevice& vulkanDevice,
                                   vk::Buffer buffer,
                                   uint32_t width,
                                   uint32_t height,
                                   VulkanCommandBuffer& vulkanCommandBuffer);

            void destroyImage(VulkanDevice& vulkanDevice);
            void destroyImageView(VulkanDevice& vulkanDevice);
            void freeImageMemory(VulkanDevice& vulkanDevice);

        private:
            bool hasStencilComponent(vk::Format format);

            vk::Image m_vkImage = {};
            vk::ImageView m_vkImageView;
            vk::DeviceMemory m_vkImageMemory;
    };
}  // namespace Genesis
