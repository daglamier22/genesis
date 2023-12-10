#pragma once

namespace Genesis {
    class VulkanDevice {
        public:
            VulkanDevice();
            ~VulkanDevice();

            VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
            VkDevice getDevice() const { return m_device; }
            VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
            VkQueue getPresentQueue() const { return m_presentQueue; }

            bool pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
            bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);

            bool createLogicalDevice(VkSurfaceKHR surface);

            void shutdown();

            struct QueueFamilyIndices {
                    std::optional<uint32_t> graphicsFamily;
                    std::optional<uint32_t> presentFamily;

                    bool isComplete() {
                        return graphicsFamily.has_value() && presentFamily.has_value();
                    }
            };
            QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

        private:
            VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
            VkDevice m_device;
            VkQueue m_graphicsQueue;
            VkQueue m_presentQueue;
    };
}  // namespace Genesis
