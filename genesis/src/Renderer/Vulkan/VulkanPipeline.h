#pragma once

#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanTypes.h"

namespace Genesis {
    class VulkanPipeline {
        public:
            VulkanPipeline();
            ~VulkanPipeline();

            VulkanPipeline(const VulkanPipeline&) = delete;
            VulkanPipeline& operator=(const VulkanPipeline&) = delete;

            vk::Pipeline const& pipeline() const { return m_vkGraphicsPipeline; }
            vk::PipelineLayout const& layout() const { return m_vkPipelineLayout; }
            vk::RenderPass const& renderPass() const { return m_vkRenderPass; }

            void createGraphicsPipeline(VulkanDevice& vulkanDevice, VulkanSwapchain& vulkanSwapchain);
            void createRenderPass(VulkanDevice& vulkanDevice, VulkanSwapchain& vulkanSwapchain);

        private:
            vk::PipelineLayout m_vkPipelineLayout;
            vk::Pipeline m_vkGraphicsPipeline;
            vk::RenderPass m_vkRenderPass;
    };
}  // namespace Genesis
