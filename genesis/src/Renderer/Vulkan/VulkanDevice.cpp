#include "VulkanDevice.h"

#include "Core/Logger.h"

namespace Genesis {
    VulkanDevice::VulkanDevice() {
    }

    VulkanDevice::~VulkanDevice() {
    }

    void VulkanDevice::pickPhysicalDevice(const vk::Instance& instance, const vk::SurfaceKHR surface) {
        uint32_t deviceCount = 0;
        std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
        deviceCount = static_cast<uint32_t>(devices.size());
        if (deviceCount == 0) {
            std::string errMsg = "Failed to find GPUs with Vulkan support.";
            GN_CORE_ERROR("{}", errMsg);
            throw std::runtime_error(errMsg);
        }
        GN_CORE_TRACE("Discovered {} devices with Vulkan support.", deviceCount);

        for (const auto& device : devices) {
            if (isDeviceSuitable(device, surface)) {
                m_vkPhysicalDevice = device;
                break;
            }
        }

        if (!m_vkPhysicalDevice) {
            std::string errMsg = "Failed to find a suitable GPU.";
            GN_CORE_ERROR("{}", errMsg);
            throw std::runtime_error(errMsg);
        }

        m_vkPhysicalDeviceProperties = m_vkPhysicalDevice.getProperties();
        m_msaaSamples = getMaxUsableSampleCount();

        std::string deviceType;
        switch (m_vkPhysicalDeviceProperties.deviceType) {
            default:
            case vk::PhysicalDeviceType::eOther:
                deviceType = "Unknown";
                break;
            case vk::PhysicalDeviceType::eIntegratedGpu:
                deviceType = "Integrated";
            case vk::PhysicalDeviceType::eDiscreteGpu:
                deviceType = "Discrete";
            case vk::PhysicalDeviceType::eVirtualGpu:
                deviceType = "Virtual";
            case vk::PhysicalDeviceType::eCpu:
                deviceType = "CPU";
        }

        GN_CORE_INFO("Vulkan physical device selected.");
        GN_CORE_TRACE("\tDevice selected: {}", m_vkPhysicalDeviceProperties.deviceName.data());
        GN_CORE_TRACE("\tDevice type: {}", deviceType);
        GN_CORE_TRACE("\tAPI version: {}.{}.{}",
                      VK_API_VERSION_MAJOR(m_vkPhysicalDeviceProperties.apiVersion),
                      VK_API_VERSION_MINOR(m_vkPhysicalDeviceProperties.apiVersion),
                      VK_API_VERSION_PATCH(m_vkPhysicalDeviceProperties.apiVersion));
    }

    bool VulkanDevice::isDeviceSuitable(const vk::PhysicalDevice& device, const vk::SurfaceKHR surface) {
        QueueFamilyIndices indices = findQueueFamilies(device, surface);

        bool extensionSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        vk::PhysicalDeviceFeatures supportedFeatures = device.getFeatures();

        return indices.isComplete() && extensionSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
    }

    bool VulkanDevice::checkDeviceExtensionSupport(const vk::PhysicalDevice& device) {
        std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();

        std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());

        GN_CORE_TRACE("Available Vulkan extensions:");
        for (const auto& extension : availableExtensions) {
            GN_CORE_TRACE("\t{}", extension.extensionName.data());
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    SwapChainSupportDetails VulkanDevice::querySwapChainSupport(const vk::PhysicalDevice& device, const vk::SurfaceKHR surface) {
        SwapChainSupportDetails details;

        details.capabilities = device.getSurfaceCapabilitiesKHR(surface);

        details.formats = device.getSurfaceFormatsKHR(surface);

        details.presentModes = device.getSurfacePresentModesKHR(surface);

        return details;
    }

    QueueFamilyIndices VulkanDevice::findQueueFamilies(const vk::PhysicalDevice& device, const vk::SurfaceKHR surface) {
        QueueFamilyIndices indices;

        std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            presentSupport = device.getSurfaceSupportKHR(i, surface);
            if (presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    void VulkanDevice::createLogicalDevice(const vk::SurfaceKHR surface) {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice(), surface);

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {
            indices.graphicsFamily.value(),
            indices.presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            vk::DeviceQueueCreateInfo queueCreateInfo(vk::DeviceQueueCreateFlags(),
                                                      queueFamily,
                                                      1,
                                                      &queuePriority);
            queueCreateInfos.push_back(queueCreateInfo);
        }

        vk::PhysicalDeviceFeatures deviceFeatures = vk::PhysicalDeviceFeatures();
        deviceFeatures.samplerAnisotropy = true;
        // deviceFeatures.sampleRateShading = VK_TRUE;  // NOTE: expensive! enable sample shading feature for the device

        vk::DeviceCreateInfo createInfo = vk::DeviceCreateInfo(vk::DeviceCreateFlags(),
                                                               static_cast<uint32_t>(queueCreateInfos.size()),
                                                               queueCreateInfos.data(),
                                                               0, nullptr,
                                                               static_cast<uint32_t>(m_deviceExtensions.size()),
                                                               m_deviceExtensions.data(),
                                                               &deviceFeatures);

        try {
            m_vkDevice = m_vkPhysicalDevice.createDevice(createInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create logical device: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        m_vkGraphicsQueue = m_vkDevice.getQueue(indices.graphicsFamily.value(), 0);
        m_vkPresentQueue = m_vkDevice.getQueue(indices.presentFamily.value(), 0);

        GN_CORE_INFO("Vulkan logical device created.");
    }

    vk::SampleCountFlagBits VulkanDevice::getMaxUsableSampleCount() {
        vk::PhysicalDeviceProperties props = physicalDeviceProperties();
        vk::SampleCountFlags counts = props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts;
        if (counts & vk::SampleCountFlagBits::e64) {
            return vk::SampleCountFlagBits::e64;
        }
        if (counts & vk::SampleCountFlagBits::e32) {
            return vk::SampleCountFlagBits::e32;
        }
        if (counts & vk::SampleCountFlagBits::e16) {
            return vk::SampleCountFlagBits::e16;
        }
        if (counts & vk::SampleCountFlagBits::e8) {
            return vk::SampleCountFlagBits::e8;
        }
        if (counts & vk::SampleCountFlagBits::e4) {
            return vk::SampleCountFlagBits::e4;
        }
        if (counts & vk::SampleCountFlagBits::e2) {
            return vk::SampleCountFlagBits::e2;
        }

        return vk::SampleCountFlagBits::e1;
    }
}  // namespace Genesis
