#include "VulkanPipeline.h"

#include "Core/Logger.h"
#include "VulkanMesh.h"
#include "VulkanShader.h"

namespace Genesis {
    VulkanPipeline::VulkanPipeline() {
    }

    VulkanPipeline::~VulkanPipeline() {
    }

    void VulkanPipeline::createGraphicsPipeline(VulkanDevice& vulkanDevice, VulkanSwapchain& vulkanSwapchain) {
        VulkanShader vertShader(vulkanDevice, "assets/shaders/shader.vert.spv");
        VulkanShader fragShader(vulkanDevice, "assets/shaders/shader.frag.spv");

        vk::PipelineShaderStageCreateInfo vertShaderStageInfo = {};
        vertShaderStageInfo.flags = vk::PipelineShaderStageCreateFlags();
        vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
        vertShaderStageInfo.module = vertShader.shaderModule();
        vertShaderStageInfo.pName = "main";

        vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.flags = vk::PipelineShaderStageCreateFlags();
        fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
        fragShaderStageInfo.module = fragShader.shaderModule();
        fragShaderStageInfo.pName = "main";

        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

        auto bindingDescription = VulkanMesh::getBindingDescription();
        auto attributeDescriptions = VulkanMesh::getAttributeDescriptions();
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.flags = vk::PipelineVertexInputStateCreateFlags();
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.flags = vk::PipelineInputAssemblyStateCreateFlags();
        inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        vk::Viewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)vulkanSwapchain.extent().width;
        viewport.height = (float)vulkanSwapchain.extent().height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vk::Rect2D scissor = {};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent = vulkanSwapchain.extent();

        vk::PipelineViewportStateCreateInfo viewportState = {};
        viewportState.flags = vk::PipelineViewportStateCreateFlags();
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        vk::PipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.flags = vk::PipelineRasterizationStateCreateFlags();
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = vk::PolygonMode::eFill;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = vk::CullModeFlagBits::eBack;
        rasterizer.frontFace = vk::FrontFace::eClockwise;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f;  // Optional
        rasterizer.depthBiasClamp = 0.0f;           // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f;     // Optional

        vk::PipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.flags = vk::PipelineMultisampleStateCreateFlags();
        multisampling.sampleShadingEnable = VK_FALSE;  // NOTE: expensive! enable sample shading in the pipeline
        multisampling.minSampleShading = .2f;          // NOTE: expensive! min fraction for sample shading; closer to one is smoother
        multisampling.rasterizationSamples = vulkanDevice.msaaSamples();
        multisampling.minSampleShading = 1.0f;           // Optional
        multisampling.pSampleMask = nullptr;             // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
        multisampling.alphaToOneEnable = VK_FALSE;       // Optional

        vk::PipelineDepthStencilStateCreateInfo depthStencil = {};
        depthStencil.flags = vk::PipelineDepthStencilStateCreateFlags();
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = vk::CompareOp::eLess;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f;  // Optional
        depthStencil.maxDepthBounds = 1.0f;  // Optional
        depthStencil.stencilTestEnable = VK_FALSE;

        vk::PipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;   // Optional
        colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero;  // Optional
        colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;              // Optional
        colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;   // Optional
        colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;  // Optional
        colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;              // Optional

        vk::PipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.flags = vk::PipelineColorBlendStateCreateFlags();
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = vk::LogicOp::eCopy;  // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;  // Optional
        colorBlending.blendConstants[1] = 0.0f;  // Optional
        colorBlending.blendConstants[2] = 0.0f;  // Optional
        colorBlending.blendConstants[3] = 0.0f;  // Optional

        // std::vector<vk::DynamicState> dynamicStates = {
        //     vk::DynamicState::eViewport,
        //     vk::DynamicState::eScissor};
        // vk::PipelineDynamicStateCreateInfo dynamicState = {};
        // dynamicState.flags = vk::PipelineDynamicStateCreateFlags();
        // dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        // dynamicState.pDynamicStates = dynamicStates.data();

        std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = {vulkanSwapchain.frameDescriptorSetLayout(), vulkanSwapchain.meshDescriptorSetLayout()};
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.flags = vk::PipelineLayoutCreateFlags();
        pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        try {
            m_vkPipelineLayout = vulkanDevice.logicalDevice().createPipelineLayout(pipelineLayoutInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create pipeline layout: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        vk::GraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.flags = vk::PipelineCreateFlags();
        pipelineInfo.stageCount = shaderStages.size();
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        // pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = m_vkPipelineLayout;
        pipelineInfo.renderPass = m_vkRenderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = nullptr;  // Optional
        pipelineInfo.basePipelineIndex = -1;        // Optional

        try {
            m_vkGraphicsPipeline = vulkanDevice.logicalDevice().createGraphicsPipeline(nullptr, pipelineInfo).value;
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create graphics pipeline: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        vulkanDevice.logicalDevice().destroyShaderModule(vertShader.shaderModule());
        vulkanDevice.logicalDevice().destroyShaderModule(fragShader.shaderModule());

        GN_CORE_INFO("Vulkan pipeline created successfully.");
    }

    void VulkanPipeline::createRenderPass(VulkanDevice& vulkanDevice, VulkanSwapchain& vulkanSwapchain) {
        vk::AttachmentDescription colorAttachment = {};
        colorAttachment.flags = vk::AttachmentDescriptionFlags();
        colorAttachment.format = vulkanSwapchain.format();
        colorAttachment.samples = vulkanDevice.msaaSamples();
        colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
        colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
        colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::AttachmentDescription depthAttachment = {};
        depthAttachment.format = vulkanSwapchain.findDepthFormat(vulkanDevice);
        depthAttachment.samples = vulkanDevice.msaaSamples();
        depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
        depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
        depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::AttachmentDescription colorAttachmentResolve = {};
        colorAttachmentResolve.format = vulkanSwapchain.format();
        colorAttachmentResolve.samples = vk::SampleCountFlagBits::e1;
        colorAttachmentResolve.loadOp = vk::AttachmentLoadOp::eDontCare;
        colorAttachmentResolve.storeOp = vk::AttachmentStoreOp::eStore;
        colorAttachmentResolve.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        colorAttachmentResolve.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        colorAttachmentResolve.initialLayout = vk::ImageLayout::eUndefined;
        colorAttachmentResolve.finalLayout = vk::ImageLayout::ePresentSrcKHR;

        vk::AttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::AttachmentReference depthAttachmentRef = {};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::AttachmentReference colorAttachmentResolveRef = {};
        colorAttachmentResolveRef.attachment = 2;
        colorAttachmentResolveRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::SubpassDescription subpass = {};
        subpass.flags = vk::SubpassDescriptionFlags();
        subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        subpass.pResolveAttachments = &colorAttachmentResolveRef;

        vk::SubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
        dependency.srcAccessMask = vk::AccessFlagBits::eNone;
        dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
        dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

        std::array<vk::AttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
        vk::RenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.flags = vk::RenderPassCreateFlags();
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        try {
            m_vkRenderPass = vulkanDevice.logicalDevice().createRenderPass(renderPassInfo);
        } catch (vk::SystemError err) {
            std::string errMsg = "Failed to create render pass: ";
            GN_CORE_ERROR("{}{}", errMsg, err.what());
            throw std::runtime_error(errMsg + err.what());
        }

        GN_CORE_INFO("Vulkan renderpass created successfully.");
    }
}  // namespace Genesis
