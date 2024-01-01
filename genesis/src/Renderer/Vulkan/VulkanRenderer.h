#pragma once

#include "Core/EventSystem.h"
#include "Core/Logger.h"
#include "Core/Renderer.h"
#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"
#include "VulkanMesh.h"
#include "VulkanPipeline.h"
#include "VulkanSwapchain.h"
#include "VulkanTexture.h"
#include "VulkanTypes.h"
#include "VulkanVertexMenagerie.h"

namespace Genesis {
    // const std::string MODEL_PATH = "assets/models/viking_room.obj";
    // const std::string TEXTURE_PATH = "assets/textures/viking_room.png";

    class VulkanRenderer : public Renderer {
        public:
            VulkanRenderer(std::shared_ptr<Window> window);
            ~VulkanRenderer();

            VulkanRenderer(const VulkanRenderer&) = delete;
            VulkanRenderer& operator=(const VulkanRenderer&) = delete;

            void init();
            bool drawFrame(std::shared_ptr<Scene> scene);
            void shutdown();

            void waitForIdle();

        private:
            void onResizeEvent(Event& e);

            void createInstance();
            bool checkInstanceExtensionSupport(std::vector<const char*>& extensions);
            bool checkValidationLayerSupport(std::vector<const char*>& layers);
            std::vector<const char*> getRequiredExtensions();

            void createSurface();

            void createCommandPool();
            void createCommandBuffers();
            void renderFrame(std::shared_ptr<Scene> scene);
            void recordCommandBuffer(VulkanCommandBuffer& commandBuffer, uint32_t imageIndex, std::shared_ptr<Scene> scene);
            void renderObjects(VulkanCommandBuffer& commandBuffer, meshTypes objectType, uint32_t& startInstance, uint32_t instanceCount);

            // void loadModel();
            void createAssets();

            vk::Instance m_vkInstance{nullptr};
            vk::SurfaceKHR m_vkSurface;

            VulkanDevice m_vulkanDevice;
            VulkanSwapchain m_vulkanSwapchain;
            VulkanPipeline m_vulkanPipeline;

            vk::CommandPool m_vkCommandPool;
            VulkanCommandBuffer m_vulkanMainCommandBuffer;

            VulkanVertexMenagerie m_vulkanMeshes;
            std::unordered_map<meshTypes, VulkanTexture*> m_materials;

            uint32_t m_currentFrame = 0;
            bool m_framebufferResized = false;

            void setupDebugMessenger();
            static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
                VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void* pUserData);

            vk::DebugUtilsMessengerEXT m_vkDebugMessenger{nullptr};
            vk::DispatchLoaderDynamic m_vkDldi;
#ifdef NDEBUG
            const bool m_enableValidationLayers = false;
#else
            const bool m_enableValidationLayers = true;
#endif
    };
}  // namespace Genesis
