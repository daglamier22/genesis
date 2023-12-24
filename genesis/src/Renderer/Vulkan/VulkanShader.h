#pragma once

#include "VulkanDevice.h"
#include "VulkanTypes.h"

namespace Genesis {
    class VulkanShader {
        public:
            VulkanShader(VulkanDevice& vulkanDevice, std::string filename);
            ~VulkanShader();

            VulkanShader(const VulkanShader&) = delete;
            VulkanShader& operator=(const VulkanShader&) = delete;

            vk::ShaderModule const& shaderModule() const { return m_vkShaderModule; }

        private:
            void loadShader(VulkanDevice& vulkanDevice, std::string filename);
            vk::ShaderModule createShaderModule(VulkanDevice& vulkanDevice, const std::vector<char>& code);
            static std::vector<char> readFile(const std::string& filename);

            vk::ShaderModule m_vkShaderModule;
    };
}  // namespace Genesis
