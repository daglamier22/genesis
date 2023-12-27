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
        m_maxFramesInFlight = static_cast<uint32_t>(m_vulkanSwapchain.swapchainImages().size());
        m_vulkanPipeline.createRenderPass(m_vulkanDevice, m_vulkanSwapchain);
        createDescriptorSetLayout(m_vulkanDevice);
        m_vulkanPipeline.createGraphicsPipeline(m_vulkanDevice, m_vulkanSwapchain, m_vkDescriptorSetLayout);
        m_vulkanRenderLoop.createCommandPool(m_vulkanDevice, m_vkSurface);
        m_vulkanSwapchain.createColorResources(m_vulkanDevice);
        m_vulkanSwapchain.createDepthResources(m_vulkanDevice, m_vulkanRenderLoop.commandPool());
        m_vulkanSwapchain.createFramebuffers(m_vulkanDevice, m_vulkanPipeline.renderPass());
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        loadModel();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        m_vulkanRenderLoop.createCommandBuffers(m_vulkanDevice, m_vulkanSwapchain);
        createSyncObjects(m_vulkanDevice);
        EventSystem::registerEvent(EventType::WindowResize, this, GN_BIND_EVENT_FN(VulkanRenderer::onResizeEvent));
    }

    bool VulkanRenderer::drawFrame() {
        drawFrameTemp(m_vulkanDevice);
        return true;
    }

    void VulkanRenderer::drawFrameTemp(VulkanDevice& vulkanDevice) {
        vk::Result result;
        try {
            result = vulkanDevice.logicalDevice().waitForFences(1, &m_vkInFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to wait for fence: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        uint32_t imageIndex;
        try {
            auto result = vulkanDevice.logicalDevice().acquireNextImageKHR(m_vulkanSwapchain.swapchain(), UINT64_MAX, m_vkImageAvailableSemaphores[m_currentFrame], nullptr);
            if (result.result == vk::Result::eErrorOutOfDateKHR) {
                m_vulkanSwapchain.recreateSwapChain(m_vulkanDevice, m_vkSurface, m_window, m_vulkanPipeline.renderPass(), m_vulkanRenderLoop.commandPool());
                return;
            }
            imageIndex = result.value;
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to aquire swap chain image: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }
        // VkResult result = vkAcquireNextImageKHR(m_vulkanDevice.logicalDevice(), m_vulkanSwapchain.swapchain(), UINT64_MAX, m_vkImageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
        // if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        //     return true;
        // } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        //     GN_CORE_ERROR("Failed to acquire swap chain image.");
        //     return false;
        // }

        updateUniformBuffer(m_currentFrame);

        // Only reset the fence if we are submitting work
        try {
            result = vulkanDevice.logicalDevice().resetFences(1, &m_vkInFlightFences[m_currentFrame]);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to reset fence: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        m_vulkanSwapchain.commandbuffers()[m_currentFrame].reset();
        // vkResetCommandBuffer(m_vulkanSwapchain.commandbuffers()[m_currentFrame], 0);
        recordCommandBuffer(m_vulkanSwapchain.commandbuffers()[m_currentFrame], imageIndex);

        vk::SubmitInfo submitInfo = {};

        vk::Semaphore waitSemaphores[] = {m_vkImageAvailableSemaphores[m_currentFrame]};
        vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_vulkanSwapchain.commandbuffers()[m_currentFrame];

        vk::Semaphore signalSemaphores[] = {m_vkRenderFinishedSemaphores[m_currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        try {
            vulkanDevice.graphicsQueue().submit(submitInfo, m_vkInFlightFences[m_currentFrame]);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to submit draw command buffer: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        vk::PresentInfoKHR presentInfo = {};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        vk::SwapchainKHR swapChains[] = {m_vulkanSwapchain.swapchain()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        try {
            auto result = vulkanDevice.presentQueue().presentKHR(presentInfo);
            if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_framebufferResized) {
                m_framebufferResized = false;
                m_vulkanSwapchain.recreateSwapChain(m_vulkanDevice, m_vkSurface, m_window, m_vulkanPipeline.renderPass(), m_vulkanRenderLoop.commandPool());
            }
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to present swap chain image: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }
        // result = vkQueuePresentKHR(m_vulkanDevice.presentQueue(), &presentInfo);
        // if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized) {
        //     m_framebufferResized = false;
        // } else if (result != VK_SUCCESS) {
        //     GN_CORE_ERROR("Failed to present swap chain image.");
        //     return false;
        // }

        m_currentFrame = (m_currentFrame + 1) % m_maxFramesInFlight;
    }

    void VulkanRenderer::shutdown() {
        EventSystem::unregisterEvent(EventType::WindowResize, this, GN_BIND_EVENT_FN(VulkanRenderer::onResizeEvent));

        m_vulkanSwapchain.cleanupSwapChain(m_vulkanDevice);

        for (size_t i = 0; i < m_maxFramesInFlight; i++) {
            m_vulkanDevice.logicalDevice().destroySemaphore(m_vkImageAvailableSemaphores[i]);
            m_vulkanDevice.logicalDevice().destroySemaphore(m_vkRenderFinishedSemaphores[i]);
            m_vulkanDevice.logicalDevice().destroyFence(m_vkInFlightFences[i]);
        }

        for (size_t i = 0; i < m_maxFramesInFlight; i++) {
            m_vulkanDevice.logicalDevice().destroyBuffer(m_vkUniformBuffers[i]);
            m_vulkanDevice.logicalDevice().freeMemory(m_vkUniformBuffersMemory[i]);
        }

        m_vulkanDevice.logicalDevice().destroyDescriptorPool(m_vkDescriptorPool);

        m_vulkanDevice.logicalDevice().destroySampler(m_vkTextureSampler);
        m_textureImage.destroyImageView(m_vulkanDevice);
        m_textureImage.destroyImage(m_vulkanDevice);
        m_textureImage.freeImageMemory(m_vulkanDevice);

        m_vulkanDevice.logicalDevice().destroyDescriptorSetLayout(m_vkDescriptorSetLayout);

        m_vulkanDevice.logicalDevice().destroyBuffer(m_vkIndexBuffer);
        m_vulkanDevice.logicalDevice().freeMemory(m_vkIndexBufferMemory);

        m_vulkanDevice.logicalDevice().destroyBuffer(m_vkVertexBuffer);
        m_vulkanDevice.logicalDevice().freeMemory(m_vkVertexBufferMemory);

        m_vulkanDevice.logicalDevice().destroyCommandPool(m_vulkanRenderLoop.commandPool());

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
        m_vulkanDevice.logicalDevice().waitIdle();
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

#if defined(GN_PLATFORM_MACOS)
        extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        createInfo.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

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
        vk::DeviceSize imageSize = texWidth * texHeight * 4;
        m_vkMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        if (!pixels) {
            std::string errMsg = "Failed to load texture image.";
            GN_CORE_ERROR("{}", errMsg);
            throw std::runtime_error(errMsg);
        }

        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingBufferMemory;
        createBuffer(imageSize,
                     vk::BufferUsageFlagBits::eTransferSrc,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                     stagingBuffer,
                     stagingBufferMemory);

        void* data;
        try {
            data = m_vulkanDevice.logicalDevice().mapMemory(stagingBufferMemory, vk::DeviceSize(0), imageSize, vk::MemoryMapFlags());
            memcpy(data, pixels, static_cast<size_t>(imageSize));
            m_vulkanDevice.logicalDevice().unmapMemory(stagingBufferMemory);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to copy memory: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

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
                                             m_vulkanRenderLoop.commandPool());
        copyBufferToImage(stagingBuffer, m_textureImage.image(), static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        // transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
        // transitionImageLayout(m_vkTextureImage,
        //                       VK_FORMAT_R8G8B8A8_SRGB,
        //                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        //                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        //                       m_vkMipLevels);

        m_vulkanDevice.logicalDevice().destroyBuffer(stagingBuffer);
        m_vulkanDevice.logicalDevice().freeMemory(stagingBufferMemory);

        generateMipmaps(m_textureImage.image(), vk::Format::eR8G8B8A8Srgb, texWidth, texHeight, m_vkMipLevels);

        GN_CORE_INFO("Texture successfully loaded: {}", TEXTURE_PATH.c_str());
    }

    void VulkanRenderer::generateMipmaps(vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
        // Check if image format supports linear blitting
        vk::FormatProperties formatProperties = m_vulkanDevice.physicalDevice().getFormatProperties(imageFormat);

        if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
            std::string errMsg = "Texture image format does not support linear blitting.";
            GN_CORE_ERROR("{}", errMsg);
            throw std::runtime_error(errMsg);
        }

        vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

        vk::ImageMemoryBarrier barrier = {};
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

            commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                          vk::PipelineStageFlagBits::eTransfer,
                                          vk::DependencyFlags(),
                                          0, nullptr,
                                          0, nullptr,
                                          1, &barrier);

            vk::ImageBlit blit = {};
            blit.srcOffsets[0] = vk::Offset3D(0, 0, 0);
            blit.srcOffsets[1] = vk::Offset3D(mipWidth, mipHeight, 1);
            blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = vk::Offset3D(0, 0, 0);
            blit.dstOffsets[1] = vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1);
            blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal,
                                    image, vk::ImageLayout::eTransferDstOptimal,
                                    1, &blit,
                                    vk::Filter::eLinear);

            barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                          vk::PipelineStageFlagBits::eFragmentShader,
                                          vk::DependencyFlags(),
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
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                      vk::PipelineStageFlagBits::eFragmentShader,
                                      vk::DependencyFlags(),
                                      0, nullptr,
                                      0, nullptr,
                                      1, &barrier);

        endSingleTimeCommands(commandBuffer);

        GN_CORE_INFO("Vulkan mip maps generated successfully.");
    }

    void VulkanRenderer::createTextureImageView() {
        m_textureImage.createImageView(m_vulkanDevice,
                                       m_textureImage.image(),
                                       vk::Format::eR8G8B8A8Srgb,
                                       vk::ImageAspectFlagBits::eColor,
                                       m_vkMipLevels);
    }

    void VulkanRenderer::createTextureSampler() {
        vk::SamplerCreateInfo samplerInfo = {};
        samplerInfo.flags = vk::SamplerCreateFlags();
        samplerInfo.magFilter = vk::Filter::eLinear;
        samplerInfo.minFilter = vk::Filter::eLinear;
        samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = m_vulkanDevice.physicalDeviceProperties().limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = vk::CompareOp::eAlways;
        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
        samplerInfo.mipLodBias = 0.0f;  // Optional
        samplerInfo.minLod = 0.0f;      // Optional
        samplerInfo.maxLod = static_cast<float>(m_vkMipLevels);

        try {
            m_vkTextureSampler = m_vulkanDevice.logicalDevice().createSampler(samplerInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create texture sampler: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        GN_CORE_INFO("Vulkan texture sampler created succesfully.");
    }

    void VulkanRenderer::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height) {
        vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

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

        commandBuffer.copyBufferToImage(buffer,
                                        image,
                                        vk::ImageLayout::eTransferDstOptimal,
                                        1,
                                        &region);

        endSingleTimeCommands(commandBuffer);
    }

    void VulkanRenderer::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex) {
        vk::CommandBufferBeginInfo beginInfo = {};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        try {
            commandBuffer.begin(beginInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Unable to being recording command buffer: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        vk::RenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.renderPass = m_vulkanPipeline.renderPass();
        renderPassInfo.framebuffer = m_vulkanSwapchain.framebuffers()[imageIndex];
        renderPassInfo.renderArea.offset.x = 0;
        renderPassInfo.renderArea.offset.y = 0;
        renderPassInfo.renderArea.extent = m_vulkanSwapchain.extent();

        std::array<vk::ClearValue, 2> clearValues{};
        clearValues[0].color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
        clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_vulkanPipeline.pipeline());

        vk::Viewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_vulkanSwapchain.extent().width);
        viewport.height = static_cast<float>(m_vulkanSwapchain.extent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        commandBuffer.setViewport(0, 1, &viewport);

        vk::Rect2D scissor = {};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent = m_vulkanSwapchain.extent();
        commandBuffer.setScissor(0, 1, &scissor);

        vk::Buffer vertexBuffers[] = {m_vkVertexBuffer};
        vk::DeviceSize offsets[] = {0};
        commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);

        commandBuffer.bindIndexBuffer(m_vkIndexBuffer, 0, vk::IndexType::eUint32);

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_vulkanPipeline.layout(), 0, 1, &m_vkDescriptorSets[m_currentFrame], 0, nullptr);

        commandBuffer.drawIndexed(static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);

        commandBuffer.endRenderPass();

        try {
            commandBuffer.end();
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to record command buffer: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }
    }

    void VulkanRenderer::createSyncObjects(VulkanDevice& vulkanDevice) {
        m_vkImageAvailableSemaphores.resize(m_maxFramesInFlight);
        m_vkRenderFinishedSemaphores.resize(m_maxFramesInFlight);
        m_vkInFlightFences.resize(m_maxFramesInFlight);

        for (size_t i = 0; i < m_maxFramesInFlight; i++) {
            m_vkImageAvailableSemaphores[i] = createSemaphore(vulkanDevice);
            m_vkRenderFinishedSemaphores[i] = createSemaphore(vulkanDevice);
            m_vkInFlightFences[i] = createFence(vulkanDevice);
        }

        GN_CORE_INFO("Vulkan semaphores and fences created successfully.");
    }

    vk::Semaphore VulkanRenderer::createSemaphore(VulkanDevice& vulkanDevice) {
        vk::SemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.flags = vk::SemaphoreCreateFlags();

        try {
            return vulkanDevice.logicalDevice().createSemaphore(semaphoreInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create semaphore: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }
    }

    vk::Fence VulkanRenderer::createFence(VulkanDevice& vulkanDevice) {
        vk::FenceCreateInfo fenceInfo = {};
        fenceInfo.flags = vk::FenceCreateFlags() | vk::FenceCreateFlagBits::eSignaled;

        try {
            return vulkanDevice.logicalDevice().createFence(fenceInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create fence: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }
    }

    void VulkanRenderer::loadModel() {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
            GN_CORE_ERROR("{}{}", warn, err);
            throw std::runtime_error(warn + err);
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
    }

    void VulkanRenderer::createVertexBuffer() {
        vk::DeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingBufferMemory;
        createBuffer(bufferSize,
                     vk::BufferUsageFlagBits::eTransferSrc,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                     stagingBuffer, stagingBufferMemory);

        try {
            void* data;
            auto result = m_vulkanDevice.logicalDevice().mapMemory(stagingBufferMemory, vk::DeviceSize(0), bufferSize, vk::MemoryMapFlags(), &data);
            memcpy(data, m_vertices.data(), (size_t)bufferSize);
            m_vulkanDevice.logicalDevice().unmapMemory(stagingBufferMemory);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to copy memory: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        createBuffer(bufferSize,
                     vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                     vk::MemoryPropertyFlagBits::eDeviceLocal,
                     m_vkVertexBuffer,
                     m_vkVertexBufferMemory);

        copyBuffer(stagingBuffer, m_vkVertexBuffer, bufferSize);

        m_vulkanDevice.logicalDevice().destroyBuffer(stagingBuffer);
        m_vulkanDevice.logicalDevice().freeMemory(stagingBufferMemory);

        GN_CORE_INFO("Vulkan vertex buffer created.");
    }

    void VulkanRenderer::createIndexBuffer() {
        vk::DeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingBufferMemory;
        createBuffer(bufferSize,
                     vk::BufferUsageFlagBits::eTransferSrc,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                     stagingBuffer,
                     stagingBufferMemory);

        try {
            void* data;
            auto result = m_vulkanDevice.logicalDevice().mapMemory(stagingBufferMemory, vk::DeviceSize(0), bufferSize, vk::MemoryMapFlags(), &data);
            memcpy(data, m_indices.data(), (size_t)bufferSize);
            m_vulkanDevice.logicalDevice().unmapMemory(stagingBufferMemory);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to copy memory: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        createBuffer(bufferSize,
                     vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                     vk::MemoryPropertyFlagBits::eDeviceLocal,
                     m_vkIndexBuffer,
                     m_vkIndexBufferMemory);

        copyBuffer(stagingBuffer, m_vkIndexBuffer, bufferSize);

        m_vulkanDevice.logicalDevice().destroyBuffer(stagingBuffer);
        m_vulkanDevice.logicalDevice().freeMemory(stagingBufferMemory);

        GN_CORE_INFO("Vulkan index buffer created.");
    }

    void VulkanRenderer::createUniformBuffers() {
        vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

        m_vkUniformBuffers.resize(m_maxFramesInFlight);
        m_vkUniformBuffersMemory.resize(m_maxFramesInFlight);
        m_vkUniformBuffersMapped.resize(m_maxFramesInFlight);

        for (size_t i = 0; i < m_maxFramesInFlight; i++) {
            createBuffer(bufferSize,
                         vk::BufferUsageFlagBits::eUniformBuffer,
                         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                         m_vkUniformBuffers[i], m_vkUniformBuffersMemory[i]);
            auto result = m_vulkanDevice.logicalDevice().mapMemory(m_vkUniformBuffersMemory[i],
                                                                   vk::DeviceSize(0),
                                                                   bufferSize,
                                                                   vk::MemoryMapFlags(),
                                                                   &m_vkUniformBuffersMapped[i]);
        }

        GN_CORE_INFO("Vulkan uniform buffers created successfully.");
    }

    void VulkanRenderer::createDescriptorPool() {
        std::array<vk::DescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(m_maxFramesInFlight);
        poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(m_maxFramesInFlight);

        vk::DescriptorPoolCreateInfo poolInfo = {};
        poolInfo.flags = vk::DescriptorPoolCreateFlags();
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(m_maxFramesInFlight);

        try {
            m_vkDescriptorPool = m_vulkanDevice.logicalDevice().createDescriptorPool(poolInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create descriptor pool: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        GN_CORE_INFO("Vulkan descriptor pool created successfully.");
    }

    void VulkanRenderer::createDescriptorSets() {
        std::vector<vk::DescriptorSetLayout> layouts(m_maxFramesInFlight, m_vkDescriptorSetLayout);
        vk::DescriptorSetAllocateInfo allocInfo = {};
        ;
        allocInfo.descriptorPool = m_vkDescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(m_maxFramesInFlight);
        allocInfo.pSetLayouts = layouts.data();

        m_vkDescriptorSets.resize(m_maxFramesInFlight);
        try {
            m_vkDescriptorSets = m_vulkanDevice.logicalDevice().allocateDescriptorSets(allocInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to allocate descriptor sets: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        for (size_t i = 0; i < m_maxFramesInFlight; i++) {
            vk::DescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = m_vkUniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            vk::DescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            imageInfo.imageView = m_textureImage.imageView();
            imageInfo.sampler = m_vkTextureSampler;

            std::array<vk::WriteDescriptorSet, 2> descriptorWrites{};
            descriptorWrites[0].dstSet = m_vkDescriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;
            descriptorWrites[0].pImageInfo = nullptr;        // Optional
            descriptorWrites[0].pTexelBufferView = nullptr;  // Optional

            descriptorWrites[1].dstSet = m_vkDescriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            try {
                m_vulkanDevice.logicalDevice().updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
            } catch (vk::SystemError err) {
                std::string errMsg = "Failed to update descriptor sets: ";
                GN_CORE_ERROR("{}{}", errMsg, err.what());
                throw std::runtime_error(errMsg + err.what());
            }
        }

        GN_CORE_INFO("Vulkan descriptor sets created successfully.");
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

    void VulkanRenderer::createBuffer(vk::DeviceSize size,
                                      vk::BufferUsageFlags usage,
                                      vk::MemoryPropertyFlags properties,
                                      vk::Buffer& buffer,
                                      vk::DeviceMemory& bufferMemory) {
        vk::BufferCreateInfo bufferInfo = {};
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        try {
            buffer = m_vulkanDevice.logicalDevice().createBuffer(bufferInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create buffer: ";
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
            std::string errMsg = "Failed to allocate buffer memory: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        m_vulkanDevice.logicalDevice().bindBufferMemory(buffer, bufferMemory, 0);
    }

    vk::CommandBuffer VulkanRenderer::beginSingleTimeCommands() {
        vk::CommandBufferAllocateInfo allocInfo = {};
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandPool = m_vulkanRenderLoop.commandPool();
        allocInfo.commandBufferCount = 1;

        vk::CommandBuffer commandBuffer = m_vulkanDevice.logicalDevice().allocateCommandBuffers(allocInfo)[0];

        vk::CommandBufferBeginInfo beginInfo = {};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        try {
            commandBuffer.begin(beginInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Unable to being recording single use command buffer: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        return commandBuffer;
    }

    void VulkanRenderer::endSingleTimeCommands(vk::CommandBuffer commandBuffer) {
        try {
            commandBuffer.end();
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to record single use command buffer: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        vk::SubmitInfo submitInfo = {};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        try {
            auto result = m_vulkanDevice.graphicsQueue().submit(1, &submitInfo, nullptr);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to submit sungle use command buffer: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        m_vulkanDevice.graphicsQueue().waitIdle();

        m_vulkanDevice.logicalDevice().freeCommandBuffers(m_vulkanRenderLoop.commandPool(), 1, &commandBuffer);
    }

    void VulkanRenderer::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) {
        vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

        vk::BufferCopy copyRegion = {};
        copyRegion.srcOffset = 0;  // Optional
        copyRegion.dstOffset = 0;  // Optional
        copyRegion.size = size;
        try {
            commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to copy buffer: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

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
