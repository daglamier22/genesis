#pragma once

#include "Core/EventSystem.h"
#include "Core/Logger.h"
#include "Core/Renderer.h"
#include "VulkanDevice.h"
#include "VulkanPipeline.h"
#include "VulkanRenderLoop.h"
#include "VulkanSwapchain.h"
#include "VulkanTypes.h"

namespace Genesis {
    const std::string MODEL_PATH = "assets/models/viking_room.obj";
    const std::string TEXTURE_PATH = "assets/textures/viking_room.png";

    class VulkanRenderer : public Renderer {
        public:
            VulkanRenderer(std::shared_ptr<Window> window);
            ~VulkanRenderer();

            VulkanRenderer(const VulkanRenderer&) = delete;
            VulkanRenderer& operator=(const VulkanRenderer&) = delete;

            void init();
            bool drawFrame();
            void shutdown();

            void waitForIdle();

        private:
            void onResizeEvent(Event& e);

            void createInstance();
            bool checkInstanceExtensionSupport(std::vector<const char*>& extensions);
            bool checkValidationLayerSupport(std::vector<const char*>& layers);
            std::vector<const char*> getRequiredExtensions();

            void createSurface();

            void createDescriptorSetLayout(VulkanDevice& vulkanDevice);

            void createTextureImage();
            void generateMipmaps(vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
            void createTextureImageView();
            void createTextureSampler();
            void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);

            void drawFrameTemp(VulkanDevice& vulkanDevice, VulkanSwapchain& vulkanSwapchain);
            void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

            void loadModel();
            void createVertexBuffer();
            void createIndexBuffer();
            void createUniformBuffers();
            void createDescriptorPool();
            void createDescriptorSets();
            void updateUniformBuffer(uint32_t currentImage);
            void createBuffer(vk::DeviceSize size,
                              vk::BufferUsageFlags usage,
                              vk::MemoryPropertyFlags properties,
                              vk::Buffer& buffer,
                              vk::DeviceMemory& bufferMemory);
            vk::CommandBuffer beginSingleTimeCommands();
            void endSingleTimeCommands(vk::CommandBuffer commandBuffer);
            void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);

            vk::Instance m_vkInstance{nullptr};
            vk::SurfaceKHR m_vkSurface;

            VulkanDevice m_vulkanDevice;
            VulkanSwapchain m_vulkanSwapchain;
            VulkanPipeline m_vulkanPipeline;
            VulkanRenderLoop m_vulkanRenderLoop;

            vk::DescriptorSetLayout m_vkDescriptorSetLayout;
            std::vector<vk::DescriptorSet> m_vkDescriptorSets;

            uint32_t m_vkMipLevels;
            VulkanImage m_textureImage;
            vk::Sampler m_vkTextureSampler;

            std::vector<Vertex> m_vertices;
            std::vector<uint32_t> m_indices;
            vk::Buffer m_vkVertexBuffer;
            vk::DeviceMemory m_vkVertexBufferMemory;
            vk::Buffer m_vkIndexBuffer;
            vk::DeviceMemory m_vkIndexBufferMemory;

            std::vector<vk::Buffer> m_vkUniformBuffers;
            std::vector<vk::DeviceMemory> m_vkUniformBuffersMemory;
            std::vector<void*> m_vkUniformBuffersMapped;

            vk::DescriptorPool m_vkDescriptorPool;

            uint32_t m_maxFramesInFlight;
            uint32_t m_currentFrame = 0;
            bool m_framebufferResized = false;

            void setupDebugMessenger();
            static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
                VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void* pUserData);

            vk::DebugUtilsMessengerEXT m_vkDebugMessenger{nullptr};
            vk::DispatchLoaderDynamic m_vkDldi;
#ifdef NDEBUG
            const bool m_enableValidationLayers = false;
#else
            const bool m_enableValidationLayers = true;
#endif
    };
}  // namespace Genesis
