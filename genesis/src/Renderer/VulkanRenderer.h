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
            void createInstance();
            bool checkValidationLayerSupport();
            std::vector<const char*> getRequiredExtensions();

            // Setup Debug Messenger
            void setupDebugMessenger();
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

            VkInstance m_vkInstance;
            VkDebugUtilsMessengerEXT m_debugMessenger;

            const std::vector<const char*> m_validationLayers = {
                "VK_LAYER_KHRONOS_validation"};
#ifdef NDEBUG
            const bool m_enableValidationLayers = false;
#else
            const bool m_enableValidationLayers = true;
#endif
    };
}  // namespace Genesis
