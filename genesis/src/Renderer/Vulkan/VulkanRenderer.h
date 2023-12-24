#pragma once

#include "Core/EventSystem.h"
#include "Core/Logger.h"
#include "Core/Renderer.h"
#include "VulkanDevice.h"
#include "VulkanTypes.h"

namespace Genesis {
    struct Vertex {
            glm::vec3 pos;
            glm::vec3 color;
            glm::vec2 texCoord;

            static VkVertexInputBindingDescription getBindingDescription() {
                VkVertexInputBindingDescription bindingDescription{};
                bindingDescription.binding = 0;
                bindingDescription.stride = sizeof(Vertex);
                bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                return bindingDescription;
            }

            static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
                std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

                attributeDescriptions[0].binding = 0;
                attributeDescriptions[0].location = 0;
                attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
                attributeDescriptions[0].offset = offsetof(Vertex, pos);

                attributeDescriptions[1].binding = 0;
                attributeDescriptions[1].location = 1;
                attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
                attributeDescriptions[1].offset = offsetof(Vertex, color);

                attributeDescriptions[2].binding = 0;
                attributeDescriptions[2].location = 2;
                attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
                attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

                return attributeDescriptions;
            }

            bool operator==(const Vertex& other) const {
                return pos == other.pos && color == other.color && texCoord == other.texCoord;
            }
    };
}  // namespace Genesis

namespace std {
    template <>
    struct hash<Genesis::Vertex> {
            size_t operator()(Genesis::Vertex const& vertex) const {
                return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
            }
    };
};  // namespace std

namespace Genesis {
    const int MAX_FRAMES_IN_FLIGHT = 2;

    const std::string MODEL_PATH = "assets/models/viking_room.obj";
    const std::string TEXTURE_PATH = "assets/textures/viking_room.png";

    struct UniformBufferObject {
            alignas(16) glm::mat4 model;
            alignas(16) glm::mat4 view;
            alignas(16) glm::mat4 projection;
    };

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
            bool createSwapChain();
            bool createImageViews();
            bool createFramebuffers();
            vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
            vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
            VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
            void cleanupSwapChain();
            void recreateSwapChain();

            bool createDescriptorSetLayout();
            bool createGraphicsPipeline();
            bool createRenderPass();
            VkShaderModule createShaderModule(const std::vector<char>& code);
            static std::vector<char> readFile(const std::string& filename);

            bool createCommandPool();
            bool createColorResources();
            bool createDepthResources();
            VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
            VkFormat findDepthFormat();
            bool hasStencilComponent(VkFormat format);

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
            bool createBuffer(
                VkDeviceSize size,
                VkBufferUsageFlags usage,
                VkMemoryPropertyFlags properties,
                VkBuffer& buffer,
                VkDeviceMemory& bufferMemory);
            VkCommandBuffer beginSingleTimeCommands();
            void endSignleTimeCommands(VkCommandBuffer commandBuffer);
            void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
            uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

            vk::Instance m_vkInstance{nullptr};
            vk::SurfaceKHR m_vkSurface;

            VulkanDevice m_vulkanDevice;

            VkSwapchainKHR m_vkSwapchain;
            std::vector<VkImage> m_vkSwapchainImages;
            std::vector<VkImageView> m_vkSwapchainImageViews;
            std::vector<VkFramebuffer> m_vkSwapchainFramebuffers;
            VkFormat m_vkSwapchainImageFormat;
            VkExtent2D m_vkSwapchainExtent;

            VkRenderPass m_vkRenderPass;
            VkDescriptorSetLayout m_vkDescriptorSetLayout;
            std::vector<VkDescriptorSet> m_vkDescriptorSets;
            VkPipelineLayout m_vkPipelineLayout;
            VkPipeline m_vkGraphicsPipeline;

            VkCommandPool m_vkCommandPool;
            std::vector<VkCommandBuffer> m_vkCommandBuffers;

            VkImage m_vkColorImage;
            VkDeviceMemory m_vkColorImageMemory;
            VkImageView m_vkColorImageView;

            VkImage m_vkDepthImage;
            VkDeviceMemory m_vkDepthImageMemory;
            VkImageView m_vkDepthImageView;

            uint32_t m_vkMipLevels;
            VkImage m_vkTextureImage;
            VkDeviceMemory m_vkTextureImageMemory;
            VkImageView m_vkTextureImageView;
            VkSampler m_vkTextureSampler;

            std::vector<Vertex> m_vertices;
            std::vector<uint32_t> m_indices;
            VkBuffer m_vkVertexBuffer;
            VkDeviceMemory m_vkVertexBufferMemory;
            VkBuffer m_vkIndexBuffer;
            VkDeviceMemory m_vkIndexBufferMemory;

            std::vector<VkBuffer> m_vkUniformBuffers;
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
