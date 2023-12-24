#pragma once

#include "Core/EventSystem.h"
#include "Core/Logger.h"
#include "Core/Renderer.h"
#include "VulkanDevice.h"
#include "VulkanPipeline.h"
#include "VulkanSwapchain.h"
#include "VulkanTypes.h"

namespace Genesis {
    const int MAX_FRAMES_IN_FLIGHT = 2;

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
            bool createCommandPool();

            bool createTextureImage();
            bool generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
            void createTextureImageView();
            bool createTextureSampler();
            void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

            bool createCommandBuffers();
            bool recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
            bool createSyncObjects();

            bool loadModel();
            bool createVertexBuffer();
            bool createIndexBuffer();
            bool createUniformBuffers();
            bool createDescriptorPool();
            bool createDescriptorSets();
            void updateUniformBuffer(uint32_t currentImage);
            bool createBuffer(VkDeviceSize size,
                              vk::BufferUsageFlags usage,
                              vk::MemoryPropertyFlags properties,
                              vk::Buffer& buffer,
                              VkDeviceMemory& bufferMemory);
            VkCommandBuffer beginSingleTimeCommands();
            void endSingleTimeCommands(VkCommandBuffer commandBuffer);
            void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

            vk::Instance m_vkInstance{nullptr};
            vk::SurfaceKHR m_vkSurface;

            VulkanDevice m_vulkanDevice;
            VulkanSwapchain m_vulkanSwapchain;
            VulkanPipeline m_vulkanPipeline;

            vk::DescriptorSetLayout m_vkDescriptorSetLayout;
            std::vector<VkDescriptorSet> m_vkDescriptorSets;

            VkCommandPool m_vkCommandPool;
            std::vector<VkCommandBuffer> m_vkCommandBuffers;

            uint32_t m_vkMipLevels;
            VulkanImage m_textureImage;
            VkSampler m_vkTextureSampler;

            std::vector<Vertex> m_vertices;
            std::vector<uint32_t> m_indices;
            vk::Buffer m_vkVertexBuffer;
            VkDeviceMemory m_vkVertexBufferMemory;
            vk::Buffer m_vkIndexBuffer;
            VkDeviceMemory m_vkIndexBufferMemory;

            std::vector<vk::Buffer> m_vkUniformBuffers;
            std::vector<VkDeviceMemory> m_vkUniformBuffersMemory;
            std::vector<void*> m_vkUniformBuffersMapped;

            VkDescriptorPool m_vkDescriptorPool;

            std::vector<VkSemaphore> m_vkImageAvailableSemaphores;
            std::vector<VkSemaphore> m_vkRenderFinishedSemaphores;
            std::vector<VkFence> m_vkInFlightFences;

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
