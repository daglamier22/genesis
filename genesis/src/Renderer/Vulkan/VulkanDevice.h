#pragma once

#include "VulkanTypes.h"

namespace Genesis {
    class VulkanDevice {
        public:
            VulkanDevice();
            ~VulkanDevice();

            VulkanDevice(const VulkanDevice&) = delete;
            VulkanDevice& operator=(const VulkanDevice&) = delete;

            vk::PhysicalDevice const& physicalDevice() const { return m_vkPhysicalDevice; }
            vk::PhysicalDeviceProperties const& physicalDeviceProperties() const { return m_vkPhysicalDeviceProperties; }
            vk::Device const& logicalDevice() const { return m_vkDevice; }
            vk::Queue const& graphicsQueue() const { return m_vkGraphicsQueue; }
            vk::Queue const& presentQueue() const { return m_vkPresentQueue; }
            vk::SampleCountFlagBits const& msaaSamples() const { return m_msaaSamples; }

            void pickPhysicalDevice(const vk::Instance& instance, const vk::SurfaceKHR surface);
            void createLogicalDevice(const vk::SurfaceKHR surface);

            SwapChainSupportDetails querySwapChainSupport(const vk::PhysicalDevice& device, const vk::SurfaceKHR surface);
            QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice& device, const vk::SurfaceKHR surface);
            uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

        private:
            bool isDeviceSuitable(const vk::PhysicalDevice& device, const vk::SurfaceKHR surface);
            bool checkDeviceExtensionSupport(const vk::PhysicalDevice& device);
            vk::SampleCountFlagBits getMaxUsableSampleCount();

            vk::PhysicalDevice m_vkPhysicalDevice{nullptr};
            vk::PhysicalDeviceProperties m_vkPhysicalDeviceProperties;
            vk::Device m_vkDevice{nullptr};
            vk::Queue m_vkGraphicsQueue{nullptr};
            vk::Queue m_vkPresentQueue{nullptr};
            const std::vector<const char*> m_deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
            vk::SampleCountFlagBits m_msaaSamples = vk::SampleCountFlagBits::e1;
    };
}  // namespace Genesis
