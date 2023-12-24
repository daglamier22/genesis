#include "VulkanShader.h"

#include "Core/Logger.h"

namespace Genesis {
    VulkanShader::VulkanShader(VulkanDevice& vulkanDevice, std::string filename) {
        loadShader(vulkanDevice, filename);
    }

    VulkanShader::~VulkanShader() {
    }

    void VulkanShader::loadShader(VulkanDevice& vulkanDevice, std::string filename) {
        auto shaderCode = readFile(filename.c_str());
        if (shaderCode.size() == 0) {
            std::string errMsg = "Failed to read shader file: ";
            GN_CORE_ERROR("{}{}", errMsg, filename);
            throw std::runtime_error(errMsg + filename);
        }

        m_vkShaderModule = createShaderModule(vulkanDevice, shaderCode);
    }

    vk::ShaderModule VulkanShader::createShaderModule(VulkanDevice& vulkanDevice, const std::vector<char>& code) {
        vk::ShaderModuleCreateInfo createInfo = {};
        createInfo.flags = vk::ShaderModuleCreateFlags();
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        vk::ShaderModule shaderModule;
        try {
            shaderModule = vulkanDevice.logicalDevice().createShaderModule(createInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create shader module: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        return shaderModule;
    }

    std::vector<char> VulkanShader::readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            GN_CORE_ERROR("Failed to open file {}.", filename);
            std::vector<char> emptyContents(0);
            return emptyContents;
        }

        size_t fileSize(static_cast<size_t>(file.tellg()));
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }
}  // namespace Genesis
