#pragma once

#include <vulkan/vulkan.h>

#include "Core/Logger.h"
#include "Core/Renderer.h"

namespace Genesis {
    class VulkanRenderer : public Renderer {
        public:
            VulkanRenderer();
            ~VulkanRenderer();

            void init();
            void shutdown();

        private:
            bool createInstance();
            bool checkValidationLayerSupport();
            std::vector<const char*> getRequiredExtensions();

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

            // Select physical device and Queue families
            bool pickPhysicalDevice();
            bool isDeviceSuitable(VkPhysicalDevice device);

            struct QueueFamilyIndices {
                    std::optional<uint32_t> graphicsFamily;

                    bool isComplete() {
                        return graphicsFamily.has_value();
                    }
            };
            QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

            VkInstance m_vkInstance;
            VkDebugUtilsMessengerEXT m_debugMessenger;
            VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

            const std::vector<const char*> m_validationLayers = {"VK_LAYER_KHRONOS_validation"};
#ifdef NDEBUG
            const bool m_enableValidationLayers = false;
#else
            const bool m_enableValidationLayers = true;
#endif
    };
}  // namespace Genesis
