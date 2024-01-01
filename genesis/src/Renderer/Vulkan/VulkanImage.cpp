#include "VulkanImage.h"

#include "Core/Logger.h"

namespace Genesis {
    VulkanImage::VulkanImage() {
    }

    VulkanImage::~VulkanImage() {
    }

    void VulkanImage::setImage(vk::Image image) {
        m_vkImage = image;
    }

    void VulkanImage::createImage(VulkanDevice& vulkanDevice,
                                  uint32_t width,
                                  uint32_t height,
                                  uint32_t mipLevels,
                                  vk::SampleCountFlagBits numSamples,
                                  vk::Format format,
                                  vk::ImageTiling tiling,
                                  vk::ImageUsageFlags usage,
                                  vk::MemoryPropertyFlags properties) {
        vk::ImageCreateInfo imageInfo = {};
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = vk::ImageLayout::eUndefined;
        imageInfo.usage = usage;
        imageInfo.sharingMode = vk::SharingMode::eExclusive;
        imageInfo.samples = numSamples;

        try {
            m_vkImage = vulkanDevice.logicalDevice().createImage(imageInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create image: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        vk::MemoryRequirements memRequirements = vulkanDevice.logicalDevice().getImageMemoryRequirements(m_vkImage);

        vk::MemoryAllocateInfo allocInfo = {};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = vulkanDevice.findMemoryType(memRequirements.memoryTypeBits, properties);

        try {
            m_vkImageMemory = vulkanDevice.logicalDevice().allocateMemory(allocInfo);
            vulkanDevice.logicalDevice().bindImageMemory(m_vkImage, m_vkImageMemory, 0);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to allocate image memory: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        GN_CORE_INFO("Image loaded successfully.");
    }

    void VulkanImage::createImageView(VulkanDevice& vulkanDevice,
                                      vk::Image image,
                                      vk::Format format,
                                      vk::ImageAspectFlags aspectFlags,
                                      uint32_t mipLevels) {
        vk::ImageViewCreateInfo viewInfo = {};
        viewInfo.image = image;
        viewInfo.viewType = vk::ImageViewType::e2D;
        viewInfo.format = format;
        viewInfo.components.r = vk::ComponentSwizzle::eIdentity;
        viewInfo.components.g = vk::ComponentSwizzle::eIdentity;
        viewInfo.components.b = vk::ComponentSwizzle::eIdentity;
        viewInfo.components.a = vk::ComponentSwizzle::eIdentity;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        try {
            m_vkImageView = vulkanDevice.logicalDevice().createImageView(viewInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create image view: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        GN_CORE_INFO("Vulkan image view created successfully.");
    }

    void VulkanImage::transitionImageLayout(VulkanDevice& vulkanDevice,
                                            vk::Image image,
                                            vk::Format format,
                                            vk::ImageLayout oldLayout,
                                            vk::ImageLayout newLayout,
                                            uint32_t mipLevels,
                                            VulkanCommandBuffer& vulkanCommandBuffer) {
        vulkanCommandBuffer.beginSingleTimeCommands(vulkanDevice);

        vk::ImageMemoryBarrier barrier = {};
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;

        if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
            barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;

            if (hasStencilComponent(format)) {
                barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
            }
        } else {
            barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        }

        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        vk::PipelineStageFlags sourceStage;
        vk::PipelineStageFlags destinationStage;

        if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
            barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eTransfer;
        } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            sourceStage = vk::PipelineStageFlagBits::eTransfer;
            destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
        } else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
            barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
            barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
        } else {
            std::string errMsg = "Unsupported layout transition.";
            GN_CORE_ERROR("{}", errMsg);
            throw std::runtime_error(errMsg);
        }

        vulkanCommandBuffer.commandBuffer().pipelineBarrier(sourceStage,
                                                            destinationStage,
                                                            vk::DependencyFlags(),
                                                            nullptr,
                                                            nullptr,
                                                            barrier);

        vulkanCommandBuffer.endSingleTimeCommands(vulkanDevice);
    }

    void VulkanImage::copyBufferToImage(VulkanDevice& vulkanDevice,
                                        vk::Buffer buffer,
                                        uint32_t width,
                                        uint32_t height,
                                        VulkanCommandBuffer& vulkanCommandBuffer) {
        vulkanCommandBuffer.beginSingleTimeCommands(vulkanDevice);

        vk::BufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = vk::Offset3D(0, 0, 0);
        region.imageExtent = vk::Extent3D(width, height, 1);

        vulkanCommandBuffer.commandBuffer().copyBufferToImage(buffer,
                                                              m_vkImage,
                                                              vk::ImageLayout::eTransferDstOptimal,
                                                              region);

        vulkanCommandBuffer.endSingleTimeCommands(vulkanDevice);
    }

    bool VulkanImage::hasStencilComponent(vk::Format format) {
        return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
    }

    void VulkanImage::destroyImage(VulkanDevice& vulkanDevice) {
        vulkanDevice.logicalDevice().destroyImage(m_vkImage);
        GN_CORE_TRACE("Vulkan image destroyed successfully.");
    }

    void VulkanImage::destroyImageView(VulkanDevice& vulkanDevice) {
        vulkanDevice.logicalDevice().destroyImageView(m_vkImageView);
        GN_CORE_TRACE("Vulkan image view destroyed successfully.");
    }

    void VulkanImage::freeImageMemory(VulkanDevice& vulkanDevice) {
        vulkanDevice.logicalDevice().freeMemory(m_vkImageMemory);
        GN_CORE_TRACE("Vulkan image memory freed successfully.");
    }
}  // namespace Genesis
