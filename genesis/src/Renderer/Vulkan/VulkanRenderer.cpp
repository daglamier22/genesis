#include "VulkanRenderer.h"

// this code is to work around a GCC bug when also using FMT (which is included by quill logger)
#if defined(__GNUC__) && !defined(NDEBUG) && defined(__OPTIMIZE__)
    #undef __OPTIMIZE__
#endif
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <chrono>

#include "Core/Logger.h"
#include "Platform/GLFWWindow.h"
#include "VulkanShader.h"

namespace Genesis {
    VulkanRenderer::VulkanRenderer(std::shared_ptr<Window> window) : Renderer(window) {
        init();
    }

    VulkanRenderer::~VulkanRenderer() {
        shutdown();
    }

    void VulkanRenderer::onResizeEvent(Event& e) {
        GN_CORE_TRACE("Resized");
        m_framebufferResized = true;
    }

    void VulkanRenderer::init() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        m_vulkanDevice.pickPhysicalDevice(m_vkInstance, m_vkSurface);
        m_vulkanDevice.createLogicalDevice(m_vkSurface);
        m_vulkanSwapchain.createSwapChain(m_vulkanDevice, m_vkSurface, m_window);
        m_vulkanSwapchain.createImageViews(m_vulkanDevice);
        m_vulkanPipeline.createRenderPass(m_vulkanDevice, m_vulkanSwapchain);
        createDescriptorSetLayout(m_vulkanDevice);
        m_vulkanPipeline.createGraphicsPipeline(m_vulkanDevice, m_vulkanSwapchain, m_vkDescriptorSetLayout);
        m_vulkanRenderLoop.createCommandPool(m_vulkanDevice, m_vkSurface);
        m_vulkanSwapchain.createColorResources(m_vulkanDevice);
        m_vulkanSwapchain.createDepthResources(m_vulkanDevice, m_vulkanRenderLoop.commandPool());
        m_vulkanSwapchain.createFramebuffers(m_vulkanDevice, m_vulkanPipeline.renderPass());
        if (!createTextureImage()) {
            return;
        }
        createTextureImageView();
        if (!createTextureSampler()) {
            return;
        }
        if (!loadModel()) {
            return;
        }
        if (!createVertexBuffer()) {
            return;
        }
        if (!createIndexBuffer()) {
            return;
        }
        if (!createUniformBuffers()) {
            return;
        }
        if (!createDescriptorPool()) {
            return;
        }
        if (!createDescriptorSets()) {
            return;
        }
        if (!createCommandBuffers()) {
            return;
        }
        if (!createSyncObjects()) {
            return;
        }
        EventSystem::registerEvent(EventType::WindowResize, this, GN_BIND_EVENT_FN(VulkanRenderer::onResizeEvent));
    }

    bool VulkanRenderer::drawFrame() {
        vkWaitForFences(m_vulkanDevice.logicalDevice(), 1, &m_vkInFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(m_vulkanDevice.logicalDevice(), m_vulkanSwapchain.swapchain(), UINT64_MAX, m_vkImageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            m_vulkanSwapchain.recreateSwapChain(m_vulkanDevice, m_vkSurface, m_window, m_vulkanPipeline.renderPass(), m_vkCommandPool);
            return true;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            GN_CORE_ERROR("Failed to acquire swap chain image.");
            return false;
        }

        updateUniformBuffer(m_currentFrame);

        // Only reset the fence if we are submitting work
        vkResetFences(m_vulkanDevice.logicalDevice(), 1, &m_vkInFlightFences[m_currentFrame]);

        vkResetCommandBuffer(m_vkCommandBuffers[m_currentFrame], 0);
        recordCommandBuffer(m_vkCommandBuffers[m_currentFrame], imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {m_vkImageAvailableSemaphores[m_currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_vkCommandBuffers[m_currentFrame];

        VkSemaphore signalSemaphores[] = {m_vkRenderFinishedSemaphores[m_currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(m_vulkanDevice.graphicsQueue(), 1, &submitInfo, m_vkInFlightFences[m_currentFrame]) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to submit draw command buffer.");
            return false;
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {m_vulkanSwapchain.swapchain()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        presentInfo.pResults = nullptr;  // Optional

        result = vkQueuePresentKHR(m_vulkanDevice.presentQueue(), &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized) {
            m_framebufferResized = false;
            m_vulkanSwapchain.recreateSwapChain(m_vulkanDevice, m_vkSurface, m_window, m_vulkanPipeline.renderPass(), m_vkCommandPool);
        } else if (result != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to present swap chain image.");
            return false;
        }

        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        return true;
    }

    void VulkanRenderer::shutdown() {
        EventSystem::unregisterEvent(EventType::WindowResize, this, GN_BIND_EVENT_FN(VulkanRenderer::onResizeEvent));

        m_vulkanSwapchain.cleanupSwapChain(m_vulkanDevice);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(m_vulkanDevice.logicalDevice(), m_vkImageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(m_vulkanDevice.logicalDevice(), m_vkRenderFinishedSemaphores[i], nullptr);
            vkDestroyFence(m_vulkanDevice.logicalDevice(), m_vkInFlightFences[i], nullptr);
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(m_vulkanDevice.logicalDevice(), m_vkUniformBuffers[i], nullptr);
            vkFreeMemory(m_vulkanDevice.logicalDevice(), m_vkUniformBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorPool(m_vulkanDevice.logicalDevice(), m_vkDescriptorPool, nullptr);

        vkDestroySampler(m_vulkanDevice.logicalDevice(), m_vkTextureSampler, nullptr);
        m_textureImage.destroyImageView(m_vulkanDevice);
        m_textureImage.destroyImage(m_vulkanDevice);
        m_textureImage.freeImageMemory(m_vulkanDevice);

        m_vulkanDevice.logicalDevice().destroyDescriptorSetLayout(m_vkDescriptorSetLayout);

        vkDestroyBuffer(m_vulkanDevice.logicalDevice(), m_vkIndexBuffer, nullptr);
        vkFreeMemory(m_vulkanDevice.logicalDevice(), m_vkIndexBufferMemory, nullptr);

        vkDestroyBuffer(m_vulkanDevice.logicalDevice(), m_vkVertexBuffer, nullptr);
        vkFreeMemory(m_vulkanDevice.logicalDevice(), m_vkVertexBufferMemory, nullptr);

        vkDestroyCommandPool(m_vulkanDevice.logicalDevice(), m_vkCommandPool, nullptr);

        m_vulkanDevice.logicalDevice().destroyPipeline(m_vulkanPipeline.pipeline());
        m_vulkanDevice.logicalDevice().destroyPipelineLayout(m_vulkanPipeline.layout());
        m_vulkanDevice.logicalDevice().destroyRenderPass(m_vulkanPipeline.renderPass());

        m_vulkanDevice.logicalDevice().destroy();

        if (m_enableValidationLayers) {
            m_vkInstance.destroyDebugUtilsMessengerEXT(m_vkDebugMessenger, nullptr, m_vkDldi);
        }
        m_vkInstance.destroySurfaceKHR(m_vkSurface);
        m_vkInstance.destroy();
    }

    void VulkanRenderer::waitForIdle() {
        vkDeviceWaitIdle(m_vulkanDevice.logicalDevice());
    }

    void VulkanRenderer::createInstance() {
        std::shared_ptr<GLFWWindow> window = std::dynamic_pointer_cast<GLFWWindow>(m_window);
        vk::ApplicationInfo appInfo(window->getWindowTitle().c_str(),
                                    VK_MAKE_VERSION(1, 0, 0),
                                    "Genesis",
                                    VK_MAKE_VERSION(1, 0, 0),
                                    VK_API_VERSION_1_0);

        // confirm required extension support
        std::vector<const char*> extensions = getRequiredExtensions();
        if (!checkInstanceExtensionSupport(extensions)) {
            const std::string errMsg = "Instance extensions requested, but not available.";
            GN_CORE_ERROR("{}", errMsg);
            throw std::runtime_error(errMsg);
        }
#if defined(GN_PLATFORM_MACOS)
        extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

        // confirm required validation layer support
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        std::vector<const char*> validationLayers;
        if (m_enableValidationLayers) {
            validationLayers.push_back("VK_LAYER_KHRONOS_validation");
        }
        if (m_enableValidationLayers && !checkValidationLayerSupport(validationLayers)) {
            const std::string errMsg = "Validation layers requested, but not available.";
            GN_CORE_ERROR("{}", errMsg);
            throw std::runtime_error(errMsg);
        }

        vk::InstanceCreateInfo createInfo(vk::InstanceCreateFlags(),
                                          &appInfo,
                                          static_cast<uint32_t>(validationLayers.size()),
                                          validationLayers.data(),
                                          static_cast<uint32_t>(extensions.size()),
                                          extensions.data());

        try {
            m_vkInstance = vk::createInstance(createInfo);
        } catch (vk::SystemError err) {
            const std::string errMsg = "Unable to create Vulkan instance: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        GN_CORE_INFO("Vulkan instance created successfully.");
    }

    bool VulkanRenderer::checkInstanceExtensionSupport(std::vector<const char*>& extensions) {
        std::vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();
        std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());

        GN_CORE_TRACE("Available Vulkan instance extensions:");
        for (vk::ExtensionProperties availableExtension : availableExtensions) {
            GN_CORE_TRACE("\t{}", availableExtension.extensionName.data());
            requiredExtensions.erase(availableExtension.extensionName);
        }
        if (requiredExtensions.empty()) {
            GN_CORE_TRACE("Vulkan instance extensions found.");
        }
        return requiredExtensions.empty();
    }

    bool VulkanRenderer::checkValidationLayerSupport(std::vector<const char*>& layers) {
        std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();
        std::set<std::string> requiredLayers(layers.begin(), layers.end());

        GN_CORE_TRACE("Available Vulkan instance layers:");
        for (vk::LayerProperties availableLayer : availableLayers) {
            GN_CORE_TRACE("\t{}", availableLayer.layerName.data());
            requiredLayers.erase(availableLayer.layerName);
        }
        if (requiredLayers.empty()) {
            GN_CORE_TRACE("Vulkan instance layers found.");
        }
        return requiredLayers.empty();
    }

    std::vector<const char*> VulkanRenderer::getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;

        std::shared_ptr<GLFWWindow> window = std::dynamic_pointer_cast<GLFWWindow>(m_window);
        glfwExtensions = window->getRequiredVulkanInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (m_enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    void VulkanRenderer::createSurface() {
        std::shared_ptr<GLFWWindow> window = std::dynamic_pointer_cast<GLFWWindow>(m_window);
        // create a temp C style surface to retrieve from GLFW
        VkSurfaceKHR cStyleSurface;
        if (!window->createVulkanSurface(m_vkInstance, &cStyleSurface)) {
            std::string errMsg = "Failed to create window surface.";
            GN_CORE_ERROR("{}", errMsg);
            throw std::runtime_error(errMsg);
        }

        // then convert to the HPP style surface
        m_vkSurface = cStyleSurface;

        GN_CORE_INFO("Vulkan surface created successfully.");
    }

    void VulkanRenderer::createDescriptorSetLayout(VulkanDevice& vulkanDevice) {
        vk::DescriptorSetLayoutBinding uboLayoutBinding = {};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
        uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
        uboLayoutBinding.pImmutableSamplers = nullptr;  // Optional

        vk::DescriptorSetLayoutBinding samplerLayoutBinding = {};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
        samplerLayoutBinding.pImmutableSamplers = nullptr;

        std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
        vk::DescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.flags = vk::DescriptorSetLayoutCreateFlags();
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        try {
            m_vkDescriptorSetLayout = vulkanDevice.logicalDevice().createDescriptorSetLayout(layoutInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create descriptor set layout: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        GN_CORE_INFO("Vulkan descriptor set layour created successfully.");
    }

    void VulkanRenderer::createTextureImage() {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;
        m_vkMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        if (!pixels) {
            GN_CORE_ERROR("Failed to load texture image.");
            return false;
        }

        vk::Buffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(imageSize,
                     vk::BufferUsageFlagBits::eTransferSrc,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                     stagingBuffer,
                     stagingBufferMemory);

        void* data;
        vkMapMemory(m_vulkanDevice.logicalDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(m_vulkanDevice.logicalDevice(), stagingBufferMemory);

        stbi_image_free(pixels);

        m_textureImage.createImage(m_vulkanDevice,
                                   texWidth,
                                   texHeight,
                                   m_vkMipLevels,
                                   vk::SampleCountFlagBits::e1,
                                   vk::Format::eR8G8B8A8Srgb,
                                   vk::ImageTiling::eOptimal,
                                   vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                                   vk::MemoryPropertyFlagBits::eDeviceLocal);

        m_textureImage.transitionImageLayout(m_vulkanDevice,
                                             m_textureImage.image(),
                                             vk::Format::eR8G8B8A8Srgb,
                                             vk::ImageLayout::eUndefined,
                                             vk::ImageLayout::eTransferDstOptimal,
                                             m_vkMipLevels,
                                             m_vkCommandPool);
        copyBufferToImage(stagingBuffer, m_textureImage.image(), static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        // transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
        // transitionImageLayout(m_vkTextureImage,
        //                       VK_FORMAT_R8G8B8A8_SRGB,
        //                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        //                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        //                       m_vkMipLevels);

        vkDestroyBuffer(m_vulkanDevice.logicalDevice(), stagingBuffer, nullptr);
        vkFreeMemory(m_vulkanDevice.logicalDevice(), stagingBufferMemory, nullptr);

        if (!generateMipmaps(m_textureImage.image(), VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, m_vkMipLevels)) {
            return false;
        }

        GN_CORE_INFO("Texture successfully loaded: {}", TEXTURE_PATH.c_str());
        return true;
    }

    bool VulkanRenderer::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
        // Check if image format supports linear blitting
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(m_vulkanDevice.physicalDevice(), imageFormat, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            GN_CORE_ERROR("Texture image format does not support linear blitting.");
            return false;
        }

        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer,
                           image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &blit,
                           VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            if (mipWidth > 1) {
                mipWidth /= 2;
            }
            if (mipHeight > 1) {
                mipHeight /= 2;
            }
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

        endSingleTimeCommands(commandBuffer);

        GN_CORE_INFO("Vulkan mip maps generated successfully.");
        return true;
    }

    void VulkanRenderer::createTextureImageView() {
        m_textureImage.createImageView(m_vulkanDevice,
                                       m_textureImage.image(),
                                       vk::Format::eR8G8B8A8Srgb,
                                       vk::ImageAspectFlagBits::eColor,
                                       m_vkMipLevels);
    }

    bool VulkanRenderer::createTextureSampler() {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = m_vulkanDevice.physicalDeviceProperties().limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;  // Optional
        samplerInfo.minLod = 0.0f;      // Optional
        samplerInfo.maxLod = static_cast<float>(m_vkMipLevels);

        if (vkCreateSampler(m_vulkanDevice.logicalDevice(), &samplerInfo, nullptr, &m_vkTextureSampler) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to create texture sampler.");
            return false;
        }

        GN_CORE_INFO("Vulkan texture sampler created succesfully.");
        return true;
    }

    void VulkanRenderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            width,
            height,
            1};

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region);

        endSingleTimeCommands(commandBuffer);
    }

    bool VulkanRenderer::createCommandBuffers() {
        m_vkCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_vkCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)m_vkCommandBuffers.size();

        if (vkAllocateCommandBuffers(m_vulkanDevice.logicalDevice(), &allocInfo, m_vkCommandBuffers.data()) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to allocate command buffers.");
            return false;
        }

        GN_CORE_INFO("Vulkan command buffer created successfully.");
        return true;
    }

    bool VulkanRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        beginInfo.pInheritanceInfo = nullptr;  // Optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            GN_CORE_ERROR("Unable to begin recording command buffer.");
            return false;
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        VkRenderPass renderpass = m_vulkanPipeline.renderPass();
        renderPassInfo.renderPass = renderpass;  // m_vkRenderPass; TODO: fix
        renderPassInfo.framebuffer = m_vulkanSwapchain.framebuffers()[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_vulkanSwapchain.extent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanPipeline.pipeline());

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_vulkanSwapchain.extent().width);
        viewport.height = static_cast<float>(m_vulkanSwapchain.extent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = m_vulkanSwapchain.extent();
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkBuffer vertexBuffers[] = {m_vkVertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(commandBuffer, m_vkIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanPipeline.layout(), 0, 1, &m_vkDescriptorSets[m_currentFrame], 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to record command buffer.");
            return false;
        }

        return true;
    }

    bool VulkanRenderer::createSyncObjects() {
        m_vkImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_vkRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_vkInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(m_vulkanDevice.logicalDevice(), &semaphoreInfo, nullptr, &m_vkImageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_vulkanDevice.logicalDevice(), &semaphoreInfo, nullptr, &m_vkRenderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_vulkanDevice.logicalDevice(), &fenceInfo, nullptr, &m_vkInFlightFences[i]) != VK_SUCCESS) {
                GN_CORE_ERROR("Failed to create semaphores.");
                return false;
            }
        }

        return true;
    }

    bool VulkanRenderer::loadModel() {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
            GN_CORE_ERROR("{}{}", warn, err);
            return false;
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};

                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2],
                };

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
                };

                vertex.color = {1.0f, 1.0f, 1.0f};

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
                    m_vertices.push_back(vertex);
                }

                m_indices.push_back(uniqueVertices[vertex]);
            }
        }

        GN_CORE_INFO("Model loadded successfully.");
        return true;
    }

    bool VulkanRenderer::createVertexBuffer() {
        VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

        vk::Buffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize,
                     vk::BufferUsageFlagBits::eTransferSrc,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                     stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(m_vulkanDevice.logicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, m_vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(m_vulkanDevice.logicalDevice(), stagingBufferMemory);

        createBuffer(bufferSize,
                     vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                     vk::MemoryPropertyFlagBits::eDeviceLocal,
                     m_vkVertexBuffer,
                     m_vkVertexBufferMemory);

        copyBuffer(stagingBuffer, m_vkVertexBuffer, bufferSize);

        vkDestroyBuffer(m_vulkanDevice.logicalDevice(), stagingBuffer, nullptr);
        vkFreeMemory(m_vulkanDevice.logicalDevice(), stagingBufferMemory, nullptr);

        GN_CORE_INFO("Vulkan vertex buffer created.");
        return true;
    }

    bool VulkanRenderer::createIndexBuffer() {
        VkDeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

        vk::Buffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize,
                     vk::BufferUsageFlagBits::eTransferSrc,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                     stagingBuffer,
                     stagingBufferMemory);

        void* data;
        vkMapMemory(m_vulkanDevice.logicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, m_indices.data(), (size_t)bufferSize);
        vkUnmapMemory(m_vulkanDevice.logicalDevice(), stagingBufferMemory);

        createBuffer(bufferSize,
                     vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                     vk::MemoryPropertyFlagBits::eDeviceLocal,
                     m_vkIndexBuffer,
                     m_vkIndexBufferMemory);

        copyBuffer(stagingBuffer, m_vkIndexBuffer, bufferSize);

        vkDestroyBuffer(m_vulkanDevice.logicalDevice(), stagingBuffer, nullptr);
        vkFreeMemory(m_vulkanDevice.logicalDevice(), stagingBufferMemory, nullptr);

        return true;
    }

    bool VulkanRenderer::createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        m_vkUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        m_vkUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        m_vkUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(bufferSize,
                         vk::BufferUsageFlagBits::eUniformBuffer,
                         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                         m_vkUniformBuffers[i], m_vkUniformBuffersMemory[i]);
            vkMapMemory(m_vulkanDevice.logicalDevice(), m_vkUniformBuffersMemory[i], 0, bufferSize, 0, &m_vkUniformBuffersMapped[i]);
        }

        GN_CORE_INFO("Vulkan uniform buffers created successfully.");
        return true;
    }

    bool VulkanRenderer::createDescriptorPool() {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(m_vulkanDevice.logicalDevice(), &poolInfo, nullptr, &m_vkDescriptorPool) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to create descriptor pool.");
            return false;
        }

        GN_CORE_INFO("Vulkan descriptor pool created successfully.");
        return true;
    }

    bool VulkanRenderer::createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_vkDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_vkDescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        m_vkDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(m_vulkanDevice.logicalDevice(), &allocInfo, m_vkDescriptorSets.data()) != VK_SUCCESS) {
            GN_CORE_ERROR("Failed to allocate descriptor sets.");
            return false;
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = m_vkUniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkImageView imageview = m_textureImage.imageView();  // TODO: temp fix
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = imageview;  // m_textureImageView.imageView();
            imageInfo.sampler = m_vkTextureSampler;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = m_vkDescriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;
            descriptorWrites[0].pImageInfo = nullptr;        // Optional
            descriptorWrites[0].pTexelBufferView = nullptr;  // Optional

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = m_vkDescriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(m_vulkanDevice.logicalDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }

        GN_CORE_INFO("Vulkan descriptor sets created successfully.");
        return true;
    }

    void VulkanRenderer::updateUniformBuffer(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.projection = glm::perspective(glm::radians(45.0f), m_vulkanSwapchain.extent().width / (float)m_vulkanSwapchain.extent().height, 0.1f, 10.0f);
        ubo.projection[1][1] *= -1;

        memcpy(m_vkUniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

    bool VulkanRenderer::createBuffer(VkDeviceSize size,
                                      vk::BufferUsageFlags usage,
                                      vk::MemoryPropertyFlags properties,
                                      vk::Buffer& buffer,
                                      VkDeviceMemory& bufferMemory) {
        vk::BufferCreateInfo bufferInfo = {};
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        try {
            buffer = m_vulkanDevice.logicalDevice().createBuffer(bufferInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create vertex buffer: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        vk::MemoryRequirements memRequirements = m_vulkanDevice.logicalDevice().getBufferMemoryRequirements(buffer);

        vk::MemoryAllocateInfo allocInfo = {};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = m_vulkanDevice.findMemoryType(memRequirements.memoryTypeBits, properties);

        try {
            bufferMemory = m_vulkanDevice.logicalDevice().allocateMemory(allocInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to allocate vertex buffer memory: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        m_vulkanDevice.logicalDevice().bindBufferMemory(buffer, bufferMemory, 0);
        return true;
    }

    VkCommandBuffer VulkanRenderer::beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_vkCommandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(m_vulkanDevice.logicalDevice(), &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void VulkanRenderer::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(m_vulkanDevice.graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_vulkanDevice.graphicsQueue());

        vkFreeCommandBuffers(m_vulkanDevice.logicalDevice(), m_vkCommandPool, 1, &commandBuffer);
    }

    void VulkanRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;  // Optional
        copyRegion.dstOffset = 0;  // Optional
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderer::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                                 VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                                 const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                                 void* pUserData) {
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            GN_CORE_ERROR("Validation layer: {}", pCallbackData->pMessage);
        } else {
            GN_CORE_TRACE("Validation layer: {}", pCallbackData->pMessage);
        }

        return VK_FALSE;
    }

    void VulkanRenderer::setupDebugMessenger() {
        if (!m_enableValidationLayers) {
            return;
        }

        m_vkDldi = vk::DispatchLoaderDynamic(m_vkInstance, vkGetInstanceProcAddr);

        vk::DebugUtilsMessengerCreateInfoEXT createInfo = vk::DebugUtilsMessengerCreateInfoEXT(
            vk::DebugUtilsMessengerCreateFlagsEXT(),
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            VulkanRenderer::debugCallback,
            nullptr);

        try {
            m_vkDebugMessenger = m_vkInstance.createDebugUtilsMessengerEXT(createInfo, nullptr, m_vkDldi);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to setup debug messenger: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        GN_CORE_INFO("Vulkan debug messenger setup successfully.");
    }
}  // namespace Genesis
