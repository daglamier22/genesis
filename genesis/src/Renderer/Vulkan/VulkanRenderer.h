#pragma once

#include <vulkan/vulkan.h>

#include <glm.hpp>

#include "Core/EventSystem.h"
#include "Core/Logger.h"
#include "Core/Renderer.h"
#include "Platform/GLFWWindow.h"

namespace Genesis {
    const int MAX_FRAMES_IN_FLIGHT = 2;

    struct QueueFamilyIndices {
            std::optional<uint32_t> graphicsFamily;
            std::optional<uint32_t> presentFamily;

            bool isComplete() {
                return graphicsFamily.has_value() && presentFamily.has_value();
            }
    };

    struct SwapChainSupportDetails {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
    };

    struct Vertex {
            glm::vec2 pos;
            glm::vec3 color;

            static VkVertexInputBindingDescription getBindingDescription() {
                VkVertexInputBindingDescription bindingDescription{};
                bindingDescription.binding = 0;
                bindingDescription.stride = sizeof(Vertex);
                bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                return bindingDescription;
            }

            static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
                std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

                attributeDescriptions[0].binding = 0;
                attributeDescriptions[0].location = 0;
                attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
                attributeDescriptions[0].offset = offsetof(Vertex, pos);

                attributeDescriptions[1].binding = 0;
                attributeDescriptions[1].location = 1;
                attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
                attributeDescriptions[1].offset = offsetof(Vertex, color);

                return attributeDescriptions;
            }
    };

    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},
    };

    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0};

    class VulkanRenderer : public Renderer {
        public:
            VulkanRenderer(std::shared_ptr<Window> window);
            ~VulkanRenderer();

            void init();
            bool drawFrame();
            void shutdown();

            void waitForIdle();

        private:
            void onResizeEvent(Event& e);

            bool createInstance();
            bool checkValidationLayerSupport();
            std::vector<const char*> getRequiredExtensions();

            bool pickPhysicalDevice();
            bool createLogicalDevice();
            QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
            SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
            bool checkDeviceExtensionSupport(VkPhysicalDevice device);
            bool isDeviceSuitable(VkPhysicalDevice device);

            bool createSurface();
            bool createSwapChain();
            bool createImageViews();
            bool createFramebuffers();
            VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
            VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
            VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
            void cleanupSwapChain();
            void recreateSwapChain();

            bool createGraphicsPipeline();
            bool createRenderPass();
            VkShaderModule createShaderModule(const std::vector<char>& code);
            static std::vector<char> readFile(const std::string& filename);

            bool createCommandPool();
            bool createCommandBuffers();
            bool recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
            bool createSyncObjects();

            bool createVertexBuffer();
            bool createIndexBuffer();
            bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
            void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
            uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

            VkInstance m_vkInstance;

            VkPhysicalDevice m_vkPhysicalDevice = VK_NULL_HANDLE;
            VkDevice m_vkDevice;
            VkQueue m_vkGraphicsQueue;
            VkQueue m_vkPresentQueue;
            const std::vector<const char*> m_deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

            VkSurfaceKHR m_vkSurface;
            VkSwapchainKHR m_vkSwapchain;
            std::vector<VkImage> m_vkSwapchainImages;
            std::vector<VkImageView> m_vkSwapchainImageViews;
            std::vector<VkFramebuffer> m_vkSwapchainFramebuffers;
            VkFormat m_vkSwapchainImageFormat;
            VkExtent2D m_vkSwapchainExtent;

            VkRenderPass m_vkRenderPass;
            VkPipelineLayout m_vkPipelineLayout;
            VkPipeline m_vkGraphicsPipeline;

            VkCommandPool m_vkCommandPool;
            std::vector<VkCommandBuffer> m_vkCommandBuffers;

            VkBuffer m_vkVertexBuffer;
            VkDeviceMemory m_vkVertexBufferMemory;
            VkBuffer m_vkIndexBuffer;
            VkDeviceMemory m_vkIndexBufferMemory;

            std::vector<VkSemaphore> m_vkImageAvailableSemaphores;
            std::vector<VkSemaphore> m_vkRenderFinishedSemaphores;
            std::vector<VkFence> m_vkInFlightFences;

            uint32_t m_currentFrame = 0;
            bool m_framebufferResized = false;

            bool setupDebugMessenger();
            void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
            static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
                VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void* pUserData);
            static VkResult createDebugUtilsMessengerEXT(
                VkInstance instance,
                const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                const VkAllocationCallbacks* pAllocator,
                VkDebugUtilsMessengerEXT* pDebugMessenger);
            static void destroyDebugUtilsMessengerEXT(
                VkInstance instance,
                VkDebugUtilsMessengerEXT debugMessenger,
                const VkAllocationCallbacks* pAllocator);

            VkDebugUtilsMessengerEXT m_debugMessenger;

            const std::vector<const char*> m_validationLayers = {"VK_LAYER_KHRONOS_validation"};
#ifdef NDEBUG
            const bool m_enableValidationLayers = false;
#else
            const bool m_enableValidationLayers = true;
#endif
    };
}  // namespace Genesis
