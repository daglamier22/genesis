#pragma once

namespace Genesis {
    class VulkanDevice {
        public:
            VulkanDevice();
            ~VulkanDevice();

            VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
            VkDevice getDevice() const { return m_device; }
            VkQueue getGraphicsQueue() const { return m_graphicsQueue; }

            bool pickPhysicalDevice(VkInstance instance);
            bool isDeviceSuitable(VkPhysicalDevice device);

            bool createLogicalDevice();

            void shutdown();

            struct QueueFamilyIndices {
                    std::optional<uint32_t> graphicsFamily;

                    bool isComplete() {
                        return graphicsFamily.has_value();
                    }
            };
            QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

        private:
            VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
            VkDevice m_device;
            VkQueue m_graphicsQueue;
    };
}  // namespace Genesis
