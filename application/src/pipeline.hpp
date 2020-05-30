/*
 *
 * Andrew Frost
 * pipeline.hpp
 * 2020
 *
 */

#pragma once

#include <vulkan/vulkan.hpp>

namespace app {

///////////////////////////////////////////////////////////////////////////
// GraphicsPipelineGenerator
///////////////////////////////////////////////////////////////////////////

struct GraphicsPipelineGenerator
{
public:
    //--------------------------------------------------------------------------------------------------
    //
    //
    GraphicsPipelineGenerator(const vk::Device &         device,
                              const vk::PipelineLayout & layout,
                              const vk::RenderPass &     renderPass) : device(device)
    {
        pipelineCreateInfo.layout = layout;
        pipelineCreateInfo.renderPass = renderPass;
        init();
    }

    //--------------------------------------------------------------------------------------------------
    //
    //
    GraphicsPipelineGenerator(GraphicsPipelineGenerator& pipelineGen)
        : GraphicsPipelineGenerator(pipelineGen.device, pipelineGen.layout, pipelineGen.renderPass)
    {
    }

    //--------------------------------------------------------------------------------------------------
    //
    //
    GraphicsPipelineGenerator& operator=(const GraphicsPipelineGenerator& other) = delete;

    //--------------------------------------------------------------------------------------------------
    //
    //
    ~GraphicsPipelineGenerator() 
    { 
        destroyShaderModules(); 
    }

    //--------------------------------------------------------------------------------------------------
    //
    //
    vk::PipelineShaderStageCreateInfo& addShader(const std::string& code,
                                                 vk::ShaderStageFlagBits stage,
                                                 const char* entryPoint = "main")
    {
        std::vector<char> v;
        std::copy(code.begin(), code.end(), std::back_inserter(v));
        return addShader(v, stage, entryPoint);
    }

    //--------------------------------------------------------------------------------------------------
    //
    //
    template <typename T>
    vk::PipelineShaderStageCreateInfo& addShader(const std::vector<T>& code,
                                                 vk::ShaderStageFlagBits stage,
                                                 const char* entryPoint = "main");

    //--------------------------------------------------------------------------------------------------
    //
    //
    vk::Pipeline create(const vk::PipelineCache& cache)
    {
        update();
        return device.createGraphicsPipeline(cache, pipelineCreateInfo);
    }

    //--------------------------------------------------------------------------------------------------
    //
    //
    vk::Pipeline create()
    {
        return create(pipelineCache);
    }

    //--------------------------------------------------------------------------------------------------
    //
    //
    void destroyShaderModules()
    {
        for (const auto& shaderStage : shaderStages) {
            device.destroyShaderModule(shaderStage.module);
        }

        shaderStages.clear();
    }

    //--------------------------------------------------------------------------------------------------
    //
    //
    void update()
    {
        pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineCreateInfo.pStages    = shaderStages.data();

        dynamicState.update();
        colorBlendState.update();
        vertexInputState.update();
        viewportState.update();
    }

private:
    //--------------------------------------------------------------------------------------------------
    //
    //
    void init()
    {
        pipelineCreateInfo.pRasterizationState = &rasterizationState;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
        pipelineCreateInfo.pColorBlendState    = &colorBlendState;
        pipelineCreateInfo.pMultisampleState   = &multisampleState;
        pipelineCreateInfo.pViewportState      = &viewportState;
        pipelineCreateInfo.pDepthStencilState  = &depthStencilState;
        pipelineCreateInfo.pDynamicState       = &dynamicState;
        pipelineCreateInfo.pVertexInputState   = &vertexInputState;
    }

public:

    //--------------------------------------------------------------------------------------------------
    // Override Default pipeline associated structures
    //
    struct PipelineRasterizationStateCreateInfo : public vk::PipelineRasterizationStateCreateInfo
    {
        PipelineRasterizationStateCreateInfo()
        {
            lineWidth = 1.0f;
            cullMode  = vk::CullModeFlagBits::eBack;
        }
    };

    struct PipelineInputAssemblyStateCreateInfo : public vk::PipelineInputAssemblyStateCreateInfo
    {
        PipelineInputAssemblyStateCreateInfo()
        {
            topology = vk::PrimitiveTopology::eTriangleList;
        }
    };

    struct PipelineColorBlendAttachmentState : public vk::PipelineColorBlendAttachmentState
    {
        PipelineColorBlendAttachmentState()
        {
            blendEnable         = 0;
            srcColorBlendFactor = vk::BlendFactor::eZero;
            dstColorBlendFactor = vk::BlendFactor::eZero;
            colorBlendOp        = vk::BlendOp::eAdd;
            srcAlphaBlendFactor = vk::BlendFactor::eZero;
            dstAlphaBlendFactor = vk::BlendFactor::eZero;
            alphaBlendOp        = vk::BlendOp::eAdd;
            colorWriteMask      = vk::ColorComponentFlagBits::eR
                                | vk::ColorComponentFlagBits::eG
                                | vk::ColorComponentFlagBits::eB
                                | vk::ColorComponentFlagBits::eA;
        }
    };

    struct PipelineColorBlendStateCreateInfo : public vk::PipelineColorBlendStateCreateInfo
    {
        std::vector<PipelineColorBlendAttachmentState> blendAttachmentStates{
            PipelineColorBlendAttachmentState() };

        void update()
        {
            this->attachmentCount = (uint32_t)blendAttachmentStates.size();
            this->pAttachments    = blendAttachmentStates.data();
        }
    };

    struct PipelineDynamicStateCreateInfo : public vk::PipelineDynamicStateCreateInfo
    {
        std::vector<vk::DynamicState> dynamicStateEnables;

        PipelineDynamicStateCreateInfo()
        {
            dynamicStateEnables = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        }

        void update()
        {
            this->dynamicStateCount = (uint32_t)dynamicStateEnables.size();
            this->pDynamicStates    = dynamicStateEnables.data();
        }
    };

    struct PipelineVertexInputStateCreateInfo : public vk::PipelineVertexInputStateCreateInfo
    {
        std::vector<vk::VertexInputBindingDescription>   bindingDescriptions;
        std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;

        void update()
        {
            vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
            vertexBindingDescriptionCount   = static_cast<uint32_t>(bindingDescriptions.size());
            pVertexBindingDescriptions      = bindingDescriptions.data();
            pVertexAttributeDescriptions    = attributeDescriptions.data();
        }
    };

    struct PipelineViewportStateCreateInfo : public vk::PipelineViewportStateCreateInfo
    {
        std::vector<vk::Viewport> viewports;
        std::vector<vk::Rect2D>   scissors;

        void update()
        {
            if (viewports.empty()) {
                viewportCount = 1;
                pViewports    = nullptr;
            }
            else {
                viewportCount = (uint32_t)viewports.size();
                pViewports    = viewports.data();
            }

            if (scissors.empty()) {
                scissorCount = 1;
                pScissors = nullptr;
            }
            else {
                scissorCount = (uint32_t)scissors.size();
                pScissors = scissors.data();
            }
        }
    };

    struct PipelineDepthStencilStateCreateInfo : public vk::PipelineDepthStencilStateCreateInfo
    {
        PipelineDepthStencilStateCreateInfo(bool depthEnable = true)
        {
            if (depthEnable) {
                depthTestEnable = VK_TRUE;
                depthWriteEnable = VK_TRUE;
                depthCompareOp = vk::CompareOp::eLessOrEqual;
            }
        }
    };

    struct PipelineMultisampleStateCreateInfo : public vk::PipelineMultisampleStateCreateInfo
    {
        PipelineMultisampleStateCreateInfo(vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1)
        {
            sampleShadingEnable  = VK_FALSE;
            rasterizationSamples = samples;
        }
    };

    const vk::Device&                              device;
    vk::GraphicsPipelineCreateInfo                 pipelineCreateInfo;
    vk::PipelineCache                              pipelineCache;
    vk::RenderPass&                                renderPass{ pipelineCreateInfo.renderPass };
    vk::PipelineLayout&                            layout{ pipelineCreateInfo.layout };
    uint32_t&                                      subpass{ pipelineCreateInfo.subpass };

    PipelineRasterizationStateCreateInfo           rasterizationState;
    PipelineInputAssemblyStateCreateInfo           inputAssemblyState;
    PipelineColorBlendStateCreateInfo              colorBlendState;
    PipelineDynamicStateCreateInfo                 dynamicState;
    PipelineVertexInputStateCreateInfo             vertexInputState;
    PipelineViewportStateCreateInfo                viewportState;
    PipelineDepthStencilStateCreateInfo            depthStencilState;
    PipelineMultisampleStateCreateInfo             multisampleState;
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

}; // struct GraphicsPipelineGenerator


//--------------------------------------------------------------------------------------------------
// Add Shader
//
template <typename T>
vk::PipelineShaderStageCreateInfo& app::GraphicsPipelineGenerator::addShader(
    const std::vector<T>& code,
    vk::ShaderStageFlagBits stage,
    const char* entryPoint)
{
    vk::ShaderModuleCreateInfo createInfo = {};
    createInfo.codeSize = sizeof(T) * code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    vk::ShaderModule shaderModule;

    try {
        shaderModule = device.createShaderModule(createInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create shader module!");
    }

    vk::PipelineShaderStageCreateInfo shaderStage = {};
    shaderStage.stage  = stage;
    shaderStage.module = shaderModule;
    shaderStage.pName  = entryPoint;

    shaderStages.push_back(shaderStage);
    return shaderStages.back();
}

} // namespace app