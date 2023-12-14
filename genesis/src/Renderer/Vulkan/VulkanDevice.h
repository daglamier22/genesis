#pragma once

#include "VulkanSupportTypes.h"

namespace Genesis {
    class VulkanDevice {
        public:
            VulkanDevice();
            ~VulkanDevice();

            VkPhysicalDevice getPhysicalDevice() const { return m_vkPhysicalDevice; }
            VkDevice getDevice() const { return m_vkDevice; }
            VkQueue getGraphicsQueue() const { return m_vkGraphicsQueue; }
            VkQueue getPresentQueue() const { return m_vkPresentQueue; }

            bool pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
            bool createLogicalDevice(VkSurfaceKHR surface);

            QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

            SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

        private:
            bool checkDeviceExtensionSupport(VkPhysicalDevice device);
            bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);

            VkPhysicalDevice m_vkPhysicalDevice = VK_NULL_HANDLE;
            VkDevice m_vkDevice;
            VkQueue m_vkGraphicsQueue;
            VkQueue m_vkPresentQueue;

            const std::vector<const char*> m_deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    };
}  // namespace Genesis
