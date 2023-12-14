#pragma once

namespace Genesis {
    class VulkanDevice;
    class VulkanSwapchain;

    class VulkanPipeline {
        public:
            bool createGraphicsPipeline(VulkanDevice device, VulkanSwapchain swapchain);
            bool createRenderPass(VulkanDevice device, VulkanSwapchain swapchain);

            VkRenderPass getRenderPass() const { return m_vkRenderPass; }
            VkPipelineLayout getPipelineLayout() const { return m_vkPipelineLayout; }
            VkPipeline getGraphicsPipeline() const { return m_vkGraphicsPipeline; }

        private:
            VkRenderPass m_vkRenderPass;
            VkPipelineLayout m_vkPipelineLayout;
            VkPipeline m_vkGraphicsPipeline;

            VkShaderModule createShaderModule(VulkanDevice device, const std::vector<char>& code);
            static std::vector<char> readFile(const std::string& filename);
    };
}  // namespace Genesis
