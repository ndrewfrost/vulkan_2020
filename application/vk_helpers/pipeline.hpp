/*
 *
 * Andrew Frost
 * pipeline.hpp
 * 2020
 *
 */

#pragma once

#include <cassert>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace app {

///////////////////////////////////////////////////////////////////////////
// GraphicsPipelineState                                                 //
///////////////////////////////////////////////////////////////////////////

struct GraphicsPipelineState {
public:
    //-------------------------------------------------------------------------
    //
    //
    GraphicsPipelineState()
    {
        rasterizationState.flags                   = {};
        rasterizationState.depthClampEnable        = {};
        rasterizationState.rasterizerDiscardEnable = {};
        rasterizationState.polygonMode             = vk::PolygonMode::eFill;
        rasterizationState.cullMode                = vk::CullModeFlagBits::eBack;
        rasterizationState.frontFace               = vk::FrontFace::eCounterClockwise;
        rasterizationState.depthBiasEnable         = {};
        rasterizationState.depthBiasConstantFactor = {};
        rasterizationState.depthBiasClamp          = {};
        rasterizationState.depthBiasSlopeFactor    = {};
        rasterizationState.lineWidth               = 1.f;

        inputAssemblyState.flags                  = {};
        inputAssemblyState.topology               = vk::PrimitiveTopology::eTriangleList;
        inputAssemblyState.primitiveRestartEnable = {};

        colorBlendState.flags                 = {};
        colorBlendState.logicOpEnable         = {};
        colorBlendState.logicOp               = vk::LogicOp::eClear;
        colorBlendState.attachmentCount       = {};
        colorBlendState.pAttachments          = {};
        for (int i = 0; i < 4; i++) 
            colorBlendState.blendConstants[i] = 0.f;

        dynamicState.flags             = {};
        dynamicState.dynamicStateCount = {};
        dynamicState.pDynamicStates    = {};
        
        vertexInputState.flags                           = {};
        vertexInputState.vertexBindingDescriptionCount   = {};
        vertexInputState.pVertexBindingDescriptions      = {};
        vertexInputState.vertexAttributeDescriptionCount = {};
        vertexInputState.pVertexAttributeDescriptions    = {};
    
        viewportState.flags         = {};
        viewportState.viewportCount = {};
        viewportState.pViewports    = {};
        viewportState.scissorCount  = {};
        viewportState.pScissors     = {};
        
        depthStencilState.flags                 = {};
        depthStencilState.depthTestEnable       = VK_TRUE;
        depthStencilState.depthWriteEnable      = VK_TRUE;
        depthStencilState.depthCompareOp        = vk::CompareOp::eLessOrEqual;
        depthStencilState.depthBoundsTestEnable = {};
        depthStencilState.stencilTestEnable     = {};
        depthStencilState.front                 = vk::StencilOpState();
        depthStencilState.back                  = vk::StencilOpState();
        depthStencilState.minDepthBounds        = {};
        depthStencilState.maxDepthBounds        = {};

    }

    GraphicsPipelineState(const GraphicsPipelineState& src) = default;

    //-------------------------------------------------------------------------
    //
    //
    void update()
    {
        colorBlendState.attachmentCount = (uint32_t)blendAttachmentStates.size();
        colorBlendState.pAttachments    = blendAttachmentStates.data();

        dynamicState.dynamicStateCount = (uint32_t)dynamicStateEnables.size();
        dynamicState.pDynamicStates    = dynamicStateEnables.data();

        vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputState.vertexBindingDescriptionCount   = static_cast<uint32_t>(bindingDescriptions.size());
        vertexInputState.pVertexBindingDescriptions      = bindingDescriptions.data();
        vertexInputState.pVertexAttributeDescriptions    = attributeDescriptions.data();

        if (viewports.empty()) {
            viewportState.viewportCount = 1;
            viewportState.pViewports    = nullptr;
        }
        else {
            viewportState.viewportCount = (uint32_t)viewports.size();
            viewportState.pViewports    = viewports.data();
        }

        if (scissors.empty()) {
            viewportState.scissorCount = 1;
            viewportState.pScissors    = nullptr;
        }
        else {
            viewportState.scissorCount = (uint32_t)scissors.size();
            viewportState.pScissors    = scissors.data();
        }
    }

    //-------------------------------------------------------------------------
    //
    //
    static inline vk::PipelineColorBlendAttachmentState makePipelineColorBlendAttachmentState(
        vk::ColorComponentFlags colorWriteMask     = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                                                   | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
        vk::Bool32             blendEnable         = 0,
        vk::BlendFactor        srcColorBlendFactor = vk::BlendFactor::eZero,
        vk::BlendFactor        dstColorBlendFactor = vk::BlendFactor::eZero,
        vk::BlendOp            colorBlendOp        = vk::BlendOp::eAdd,
        vk::BlendFactor        srcAlphaBlendFactor = vk::BlendFactor::eZero,
        vk::BlendFactor        dstAlphaBlendFactor = vk::BlendFactor::eZero,
        vk::BlendOp            alphaBlendOp        = vk::BlendOp::eAdd)
    {
        vk::PipelineColorBlendAttachmentState res;
        res.blendEnable         = blendEnable;
        res.srcColorBlendFactor = srcColorBlendFactor;
        res.dstColorBlendFactor = dstColorBlendFactor;
        res.colorBlendOp        = colorBlendOp;
        res.srcAlphaBlendFactor = srcAlphaBlendFactor;
        res.dstAlphaBlendFactor = dstAlphaBlendFactor;
        res.alphaBlendOp        = alphaBlendOp;
        res.colorWriteMask      = colorWriteMask;

        return res;
    }
    //-------------------------------------------------------------------------
    //
    //
    static inline vk::VertexInputBindingDescription makeVertexInputBinding(
        uint32_t binding, uint32_t stride, vk::VertexInputRate rate = vk::VertexInputRate::eVertex)
    {
        vk::VertexInputBindingDescription vertexBinding;
        vertexBinding.binding   = binding;
        vertexBinding.inputRate = rate;
        vertexBinding.stride    = stride;
        return vertexBinding;
    }

    //-------------------------------------------------------------------------
    //
    //
    static inline vk::VertexInputAttributeDescription makeVertexInputAttribute(
        uint32_t location, uint32_t binding, vk::Format format, uint32_t offset)
    {
        vk::VertexInputAttributeDescription attribute;
        attribute.binding  = binding;
        attribute.location = location;
        attribute.format   = format;
        attribute.offset   = offset;
        return attribute;
    }

    //-------------------------------------------------------------------------
    //
    //
    void clearBlendAttachmentStates() { blendAttachmentStates.clear(); }
   
    void setBlendAttachmentCount(uint32_t attachmentCount) { blendAttachmentStates.resize(attachmentCount); }
    
    void setBlendAttachmentState(uint32_t attachment, vk::PipelineColorBlendAttachmentState blendState)
    {
        assert(attachment < blendAttachmentStates.size());
        if (attachment <= blendAttachmentStates.size()) {
            blendAttachmentStates[attachment] = blendState;
        }
    }
    
    uint32_t addBlendAttachmentState(vk::PipelineColorBlendAttachmentState blendState)
    {
        blendAttachmentStates.push_back(blendState);
        return (uint32_t)(blendAttachmentStates.size() - 1);
    }

    //-------------------------------------------------------------------------
    //
    //
    void clearDynamicStateEnables() { dynamicStateEnables.clear(); }

    void setDynamicStateEnablesCount(uint32_t dynamicStateCount) { dynamicStateEnables.resize(dynamicStateCount); }

    void setDynamicStateEnable(uint32_t state, vk::DynamicState dynamicState)
    {
        assert(state < dynamicStateEnables.size());
        if (state <= dynamicStateEnables.size()) {
            dynamicStateEnables[state] = dynamicState;
        }
    }

    uint32_t addDynamicStateEnable(vk::DynamicState dynamicState)
    {
        dynamicStateEnables.push_back(dynamicState);
        return (uint32_t)(dynamicStateEnables.size() - 1);
    }

    //-------------------------------------------------------------------------
    //
    //
    void clearBindingDescriptions() { bindingDescriptions.clear(); }
    
    void setBindingDescriptionsCount(uint32_t bindingDescriptionCount)
    {
        bindingDescriptions.resize(bindingDescriptionCount);
    }
    
    void setBindingDescription(uint32_t binding, vk::VertexInputBindingDescription bindingDescription)
    {
        assert(binding < bindingDescriptions.size());
        if (binding <= bindingDescriptions.size()) {
            bindingDescriptions[binding] = bindingDescription;
        }
    }
    
    uint32_t addBindingDescription(vk::VertexInputBindingDescription bindingDescription)
    {
        bindingDescriptions.push_back(bindingDescription);
        return (uint32_t)(bindingDescriptions.size() - 1);
    }

    void addBindingDescriptions(const std::vector<vk::VertexInputBindingDescription>& bindingDescriptions_)
    {
        bindingDescriptions.insert(bindingDescriptions.end(), bindingDescriptions_.begin(), bindingDescriptions_.end());
    }
    
    //-------------------------------------------------------------------------
    //
    //
    void clearAttributeDescriptions() { attributeDescriptions.clear(); }

    void setAttributeDescriptionsCount(uint32_t attributeDescriptionCount) {
        attributeDescriptions.resize(attributeDescriptionCount);
    }

    void setAttributeDescription(uint32_t attribute, vk::VertexInputAttributeDescription attributeDescription)
    {
        assert(attribute < attributeDescriptions.size());
        if (attribute <= attributeDescriptions.size()) {
            attributeDescriptions[attribute] = attributeDescription;
        }
    }
    
    uint32_t addAttributeDescription(vk::VertexInputAttributeDescription attributeDescription)
    {
        attributeDescriptions.push_back(attributeDescription);
        return (uint32_t)(attributeDescriptions.size() - 1);
    }

    void addAttributeDescriptions(const std::vector<vk::VertexInputAttributeDescription>& attributeDescriptions_)
    {
        attributeDescriptions.insert(attributeDescriptions.end(), attributeDescriptions_.begin(), attributeDescriptions_.end());
    }

    //-------------------------------------------------------------------------
    //
    //
    void clearViewports() { viewports.clear(); }

    void setViewportsCount(uint32_t viewportCount) { viewports.resize(viewportCount); }

    void setViewport(uint32_t attribute, vk::Viewport viewport)
    {
        assert(attribute < viewports.size());
        if (attribute <= viewports.size()){
            viewports[attribute] = viewport;
        }
    }

    uint32_t addViewport(vk::Viewport viewport)
    {
        viewports.push_back(viewport);
        return (uint32_t)(viewports.size() - 1);
    }

    //-------------------------------------------------------------------------
    //
    //
    void clearScissors() { scissors.clear(); }

    void setScissorsCount(uint32_t scissorCount) { scissors.resize(scissorCount); }

    void setScissor(uint32_t attribute, vk::Rect2D scissor)
    {
        assert(attribute < scissors.size());
        if (attribute <= scissors.size()) {
            scissors[attribute] = scissor;
        }
    }

    uint32_t addScissor(vk::Rect2D scissor) {
        scissors.push_back(scissor);
        return (uint32_t)(scissors.size() - 1);
    }

    //-------------------------------------------------------------------------

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState;
    vk::PipelineRasterizationStateCreateInfo rasterizationState;
    vk::PipelineMultisampleStateCreateInfo   multisampleState;
    vk::PipelineDepthStencilStateCreateInfo  depthStencilState;
    vk::PipelineViewportStateCreateInfo      viewportState;
    vk::PipelineDynamicStateCreateInfo       dynamicState;
    vk::PipelineColorBlendStateCreateInfo    colorBlendState;
    vk::PipelineVertexInputStateCreateInfo   vertexInputState;

private:
    std::vector<vk::PipelineColorBlendAttachmentState> blendAttachmentStates{ makePipelineColorBlendAttachmentState() };
    std::vector<vk::DynamicState> dynamicStateEnables = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

    std::vector<vk::VertexInputBindingDescription>   bindingDescriptions;
    std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;

    std::vector<vk::Viewport> viewports;
    std::vector<vk::Rect2D>   scissors;

}; // struct GraphicsPipelineState

///////////////////////////////////////////////////////////////////////////
// GraphicsPipelineGenerator                                             //
///////////////////////////////////////////////////////////////////////////

struct GraphicsPipelineGenerator
{
public:
    //-------------------------------------------------------------------------
    //
    //
    GraphicsPipelineGenerator(GraphicsPipelineState& state) : pipelineState(state)
    {
        init();
    }

    GraphicsPipelineGenerator(GraphicsPipelineGenerator& pipelineGen)
        : device(pipelineGen.device)
        , pipelineState(pipelineGen.pipelineState)
        , createInfo(pipelineGen.createInfo)
        , pipelineCache(pipelineGen.pipelineCache)
    {
        init();
    }

    GraphicsPipelineGenerator(
        vk::Device d,
        const vk::PipelineLayout& layout,
        const vk::RenderPass& renderPass,
        GraphicsPipelineState& state)
        : device(d)
        , pipelineState(state)
    {
        createInfo.layout = layout;
        createInfo.renderPass = renderPass;
        init();
    }

    const GraphicsPipelineGenerator& operator=(const GraphicsPipelineGenerator& other)
    {
        device = other.device;
        pipelineState = other.pipelineState;
        createInfo = other.createInfo;
        pipelineCache = other.pipelineCache;

        init();
        return *this;
    }

    ~GraphicsPipelineGenerator() { destroyShaderModules(); }

    //-------------------------------------------------------------------------
    //
    //
    void setDevice(vk::Device d) { device = d; }

    void setRenderPass(vk::RenderPass renderPass) { createInfo.renderPass = renderPass; }

    void setLayout(vk::PipelineLayout layout) { createInfo.layout = layout; }

    //-------------------------------------------------------------------------
    //
    //
    vk::PipelineShaderStageCreateInfo& addShader(
        const std::string&      code,
        vk::ShaderStageFlagBits stage,
        const char*             entryPoint = "main")
    {
        std::vector<char> v;
        std::copy(code.begin(), code.end(), std::back_inserter(v));
        return addShader(v, stage, entryPoint);
    }

    template <typename T>
    vk::PipelineShaderStageCreateInfo& addShader(
        const std::vector<T>&   code,
        vk::ShaderStageFlagBits stage,
        const char*             entryPoint = "main")
    {
        vk::ShaderModuleCreateInfo createInfo = {};
        createInfo.codeSize = sizeof(T) * code.size();
        createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

        vk::ShaderModule shaderModule;
        try{
            shaderModule = device.createShaderModule(createInfo);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to create shader module!");
        }
        temporaryModules.push_back(shaderModule);

        return addShader(shaderModule, stage, entryPoint);
    }

    vk::PipelineShaderStageCreateInfo& addShader(
        vk::ShaderModule        shaderModule,
        vk::ShaderStageFlagBits stage,
        const char*             entryPoint = "main")
    {
        vk::PipelineShaderStageCreateInfo shaderStage = {};
        shaderStage.stage  = stage;
        shaderStage.module = shaderModule;
        shaderStage.pName  = entryPoint;

        shaderStages.push_back(shaderStage);
        return shaderStages.back();
    }

    //-------------------------------------------------------------------------
    //
    //
    void clearShaders()
    {
        shaderStages.clear();
        destroyShaderModules();
    }

    //-------------------------------------------------------------------------
    //
    //
    vk::ShaderModule getShaderModule(size_t index) const
    {
        if (index < shaderStages.size())
            return shaderStages[index].module;
        return nullptr;
    }

    //-------------------------------------------------------------------------
    //
    //
    vk::Pipeline createPipeline(const vk::PipelineCache cache)
    {
        update();
        try{
            return device.createGraphicsPipeline(cache, createInfo);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to create graphics Pipeline!");
        }
    }

    vk::Pipeline createPipeline() { return createPipeline(pipelineCache); }

    //-------------------------------------------------------------------------
    //
    //
    void destroyShaderModules()
    {
        for (const auto& shaderModule : temporaryModules) {
            device.destroyShaderModule(shaderModule);
        }
        temporaryModules.clear();
    }

    //-------------------------------------------------------------------------
    //
    //
    void update()
    {
        createInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        createInfo.pStages    = shaderStages.data();

        pipelineState.update();
    }

private:
    //-------------------------------------------------------------------------
    //
    //
    void init()
    {
        pipelineState.multisampleState.rasterizationSamples = vk::SampleCountFlagBits::e1;
        createInfo.pRasterizationState = &pipelineState.rasterizationState;
        createInfo.pInputAssemblyState = &pipelineState.inputAssemblyState;
        createInfo.pColorBlendState    = &pipelineState.colorBlendState;
        createInfo.pMultisampleState   = &pipelineState.multisampleState;
        createInfo.pViewportState      = &pipelineState.viewportState;
        createInfo.pDepthStencilState  = &pipelineState.depthStencilState;
        createInfo.pDynamicState       = &pipelineState.dynamicState;
        createInfo.pVertexInputState   = &pipelineState.vertexInputState;
    }

    vk::Device                                     device;
    vk::PipelineCache                              pipelineCache;
    vk::GraphicsPipelineCreateInfo                 createInfo;

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
    std::vector<vk::ShaderModule>                  temporaryModules;

    GraphicsPipelineState&                         pipelineState;

}; // struct GraphicsPipelineGenerator

///////////////////////////////////////////////////////////////////////////
// GraphicsPipelineGeneratorCombined                                     //
///////////////////////////////////////////////////////////////////////////

struct GraphicsPipelineGeneratorCombined 
    : public GraphicsPipelineState, public GraphicsPipelineGenerator
{
    GraphicsPipelineGeneratorCombined(vk::Device deviceInput, const vk::PipelineLayout& layout, const vk::RenderPass& renderPass)
        : GraphicsPipelineState()
        , GraphicsPipelineGenerator(deviceInput, layout, renderPass, *this) {}
};

} // namespace app