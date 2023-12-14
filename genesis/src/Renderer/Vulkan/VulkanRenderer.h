#pragma once

#include <vulkan/vulkan.h>

#include "Core/Logger.h"
#include "Core/Renderer.h"
#include "Platform/GLFWWindow.h"
#include "VulkanDevice.h"
#include "VulkanPipeline.h"
#include "VulkanSupportTypes.h"
#include "VulkanSwapchain.h"

namespace Genesis {
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
            bool createCommandPool();
            bool createCommandBuffer();

            bool recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

            bool createSyncObjects();

            VkInstance m_vkInstance;
            VulkanDevice m_vkDevice;
            VulkanSwapchain m_vkSwapChain;
            VulkanPipeline m_vkPipeline;
            VkCommandPool m_vkCommandPool;
            VkCommandBuffer m_vkCommandBuffer;

            VkSemaphore m_vkImageAvailableSemaphore;
            VkSemaphore m_vkRenderFinishedSemaphore;
            VkFence m_vkInFlightFence;

            // Setup Debug Messenger
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
