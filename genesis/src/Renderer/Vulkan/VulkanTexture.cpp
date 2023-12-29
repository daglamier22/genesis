#include "VulkanTexture.h"

// this code is to work around a GCC bug when also using FMT (which is included by quill logger)
#if defined(__GNUC__) && !defined(NDEBUG) && defined(__OPTIMIZE__)
    #undef __OPTIMIZE__
#endif
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Core/Logger.h"
#include "VulkanBuffer.h"

namespace Genesis {
    VulkanTexture::VulkanTexture(VulkanDevice& vulkanDevice,
                                 std::string filename,
                                 vk::CommandBuffer commandBuffer,
                                 vk::Queue queue,
                                 vk::DescriptorSetLayout layout,
                                 vk::DescriptorPool descriptorPool) {
        m_vkLogicalDevice = vulkanDevice.logicalDevice();
        m_filename = filename;
        m_vkCommandBuffer = commandBuffer;
        m_vkDescriptorPool = descriptorPool;
        m_vkLayout = layout;

        load();

        m_textureImage.createImage(vulkanDevice,
                                   m_width,
                                   m_height,
                                   m_vkMipLevels,
                                   vk::SampleCountFlagBits::e1,
                                   vk::Format::eR8G8B8A8Srgb,
                                   vk::ImageTiling::eOptimal,
                                   vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                                   vk::MemoryPropertyFlagBits::eDeviceLocal);

        populate(vulkanDevice);

        stbi_image_free(m_pixels);

        m_textureImage.createImageView(vulkanDevice,
                                       m_textureImage.image(),
                                       vk::Format::eR8G8B8A8Srgb,  // NOTE: unorm here???
                                       vk::ImageAspectFlagBits::eColor,
                                       m_vkMipLevels);

        createTextureSampler(vulkanDevice);

        makeDescriptorSet(vulkanDevice);

        // // generateMipmaps(m_textureImage.image(), vk::Format::eR8G8B8A8Srgb, texWidth, texHeight, m_vkMipLevels);

        GN_CORE_INFO("Texture successfully loaded: {}", m_filename.c_str());
    }

    VulkanTexture::~VulkanTexture() {
        m_vkLogicalDevice.freeMemory(m_textureImage.imageMemory());
        m_vkLogicalDevice.destroyImage(m_textureImage.image());
        m_vkLogicalDevice.destroyImageView(m_textureImage.imageView());
        m_vkLogicalDevice.destroySampler(m_vkSampler);
    }

    void VulkanTexture::use(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout) {
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, m_vkDescriptorSet, nullptr);
    }

    void VulkanTexture::load() {
        m_pixels = stbi_load(m_filename.c_str(), &m_width, &m_height, &m_channels, STBI_rgb_alpha);

        if (!m_pixels) {
            std::string errMsg = "Failed to load texture image.";
            GN_CORE_ERROR("{}", errMsg);
            throw std::runtime_error(errMsg);
        }
        // m_vkMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    }

    void VulkanTexture::populate(VulkanDevice& vulkanDevice) {
        vk::DeviceSize imageSize = m_width * m_height * 4;
        VulkanBuffer stagingBuffer;
        stagingBuffer.createBuffer(vulkanDevice,
                                   imageSize,
                                   vk::BufferUsageFlagBits::eTransferSrc,
                                   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        void* data;
        try {
            data = vulkanDevice.logicalDevice().mapMemory(stagingBuffer.memory(), vk::DeviceSize(0), imageSize, vk::MemoryMapFlags());
            memcpy(data, m_pixels, static_cast<size_t>(imageSize));
            vulkanDevice.logicalDevice().unmapMemory(stagingBuffer.memory());
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to copy memory: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        m_textureImage.transitionImageLayout(vulkanDevice,
                                             m_textureImage.image(),
                                             vk::Format::eR8G8B8A8Srgb,  // ???
                                             vk::ImageLayout::eUndefined,
                                             vk::ImageLayout::eTransferDstOptimal,
                                             m_vkMipLevels,
                                             m_vkCommandBuffer);

        m_textureImage.copyBufferToImage(vulkanDevice,
                                         stagingBuffer.buffer(),
                                         static_cast<uint32_t>(m_width),
                                         static_cast<uint32_t>(m_height),
                                         m_vkCommandBuffer);

        m_textureImage.transitionImageLayout(vulkanDevice,
                                             m_textureImage.image(),
                                             vk::Format::eR8G8B8A8Srgb,  // ???
                                             vk::ImageLayout::eTransferDstOptimal,
                                             vk::ImageLayout::eShaderReadOnlyOptimal,
                                             m_vkMipLevels,
                                             m_vkCommandBuffer);

        vulkanDevice.logicalDevice().destroyBuffer(stagingBuffer.buffer());
        vulkanDevice.logicalDevice().freeMemory(stagingBuffer.memory());
    }

    void VulkanTexture::createTextureSampler(VulkanDevice& vulkanDevice) {
        vk::SamplerCreateInfo samplerInfo = {};
        samplerInfo.flags = vk::SamplerCreateFlags();
        samplerInfo.minFilter = vk::Filter::eNearest;
        samplerInfo.magFilter = vk::Filter::eLinear;
        samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
        samplerInfo.anisotropyEnable = true;
        samplerInfo.maxAnisotropy = vulkanDevice.physicalDeviceProperties().limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
        samplerInfo.unnormalizedCoordinates = false;
        samplerInfo.compareEnable = false;
        samplerInfo.compareOp = vk::CompareOp::eAlways;
        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;  // static_cast<float>(m_vkMipLevels);

        try {
            m_vkSampler = vulkanDevice.logicalDevice().createSampler(samplerInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create texture sampler: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        GN_CORE_INFO("Vulkan texture sampler created succesfully.");
    }

    void VulkanTexture::makeDescriptorSet(VulkanDevice& vulkanDevice) {
        std::vector<vk::DescriptorSetLayout> layouts = {m_vkLayout};
        vk::DescriptorSetAllocateInfo allocInfo = {};
        allocInfo.descriptorPool = m_vkDescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = layouts.data();

        std::vector<vk::DescriptorSet> descriptorSets;
        try {
            m_vkDescriptorSet = vulkanDevice.logicalDevice().allocateDescriptorSets(allocInfo)[0];
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to allocate descriptor sets: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        vk::DescriptorImageInfo imageDescriptor;
        imageDescriptor.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageDescriptor.imageView = m_textureImage.imageView();
        imageDescriptor.sampler = m_vkSampler;

        vk::WriteDescriptorSet descriptorWrite;
        descriptorWrite.dstSet = m_vkDescriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageDescriptor;

        vulkanDevice.logicalDevice().updateDescriptorSets(descriptorWrite, nullptr);

        GN_CORE_INFO("Vulkan texture descriptor sets created successfully.");
    }

    // void VulkanTexture::generateMipmaps(vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
    //     // Check if image format supports linear blitting
    //     vk::FormatProperties formatProperties = m_vulkanDevice.physicalDevice().getFormatProperties(imageFormat);

    //     if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
    //         std::string errMsg = "Texture image format does not support linear blitting.";
    //         GN_CORE_ERROR("{}", errMsg);
    //         throw std::runtime_error(errMsg);
    //     }

    //     vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

    //     vk::ImageMemoryBarrier barrier = {};
    //     barrier.image = image;
    //     barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    //     barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    //     barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    //     barrier.subresourceRange.baseArrayLayer = 0;
    //     barrier.subresourceRange.layerCount = 1;
    //     barrier.subresourceRange.levelCount = 1;

    //     int32_t mipWidth = texWidth;
    //     int32_t mipHeight = texHeight;

    //     for (uint32_t i = 1; i < mipLevels; i++) {
    //         barrier.subresourceRange.baseMipLevel = i - 1;
    //         barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    //         barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
    //         barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    //         barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

    //         commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
    //                                       vk::PipelineStageFlagBits::eTransfer,
    //                                       vk::DependencyFlags(),
    //                                       0, nullptr,
    //                                       0, nullptr,
    //                                       1, &barrier);

    //         vk::ImageBlit blit = {};
    //         blit.srcOffsets[0] = vk::Offset3D(0, 0, 0);
    //         blit.srcOffsets[1] = vk::Offset3D(mipWidth, mipHeight, 1);
    //         blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    //         blit.srcSubresource.mipLevel = i - 1;
    //         blit.srcSubresource.baseArrayLayer = 0;
    //         blit.srcSubresource.layerCount = 1;
    //         blit.dstOffsets[0] = vk::Offset3D(0, 0, 0);
    //         blit.dstOffsets[1] = vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1);
    //         blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    //         blit.dstSubresource.mipLevel = i;
    //         blit.dstSubresource.baseArrayLayer = 0;
    //         blit.dstSubresource.layerCount = 1;

    //         commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal,
    //                                 image, vk::ImageLayout::eTransferDstOptimal,
    //                                 1, &blit,
    //                                 vk::Filter::eLinear);

    //         barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
    //         barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    //         barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
    //         barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    //         commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
    //                                       vk::PipelineStageFlagBits::eFragmentShader,
    //                                       vk::DependencyFlags(),
    //                                       0, nullptr,
    //                                       0, nullptr,
    //                                       1, &barrier);

    //         if (mipWidth > 1) {
    //             mipWidth /= 2;
    //         }
    //         if (mipHeight > 1) {
    //             mipHeight /= 2;
    //         }
    //     }

    //     barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    //     barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    //     barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    //     barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    //     barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    //     commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
    //                                   vk::PipelineStageFlagBits::eFragmentShader,
    //                                   vk::DependencyFlags(),
    //                                   0, nullptr,
    //                                   0, nullptr,
    //                                   1, &barrier);

    //     endSingleTimeCommands(commandBuffer);

    //     GN_CORE_INFO("Vulkan mip maps generated successfully.");
    // }
}  // namespace Genesis
