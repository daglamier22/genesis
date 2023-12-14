#pragma once

#include <vulkan/vulkan.h>

#include "Core/Logger.h"
#include "Core/Renderer.h"
#include "Platform/GLFWWindow.h"

namespace Genesis {
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

    class VulkanRenderer : public Renderer {
        public:
            VulkanRenderer(std::shared_ptr<Window> window);
            ~VulkanRenderer();

            void init(std::shared_ptr<Window> window);
            bool drawFrame();
            void shutdown();

            void waitForIdle();

        private:
            bool createInstance();
            bool checkValidationLayerSupport();
            std::vector<const char*> getRequiredExtensions();

            bool pickPhysicalDevice();
            bool createLogicalDevice();
            QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
            SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
            bool checkDeviceExtensionSupport(VkPhysicalDevice device);
            bool isDeviceSuitable(VkPhysicalDevice device);

            bool createSurface(std::shared_ptr<GLFWWindow> window);
            bool createSwapChain(std::shared_ptr<GLFWWindow> window);
            bool createImageViews();
            bool createFramebuffers();
            VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
            VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
            VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, std::shared_ptr<GLFWWindow> window);

            bool createGraphicsPipeline();
            bool createRenderPass();
            VkShaderModule createShaderModule(const std::vector<char>& code);
            static std::vector<char> readFile(const std::string& filename);

            bool createCommandPool();
            bool createCommandBuffer();
            bool recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
            bool createSyncObjects();

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
            VkCommandBuffer m_vkCommandBuffer;

            VkSemaphore m_vkImageAvailableSemaphore;
            VkSemaphore m_vkRenderFinishedSemaphore;
            VkFence m_vkInFlightFence;

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
