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
    vk::PipelineShaderStageCreateInfo& addShader()
    {
    }

    //--------------------------------------------------------------------------------------------------
    //
    //
    template <typename T>
    vk::PipelineShaderStageCreateInfo& addShader()
    {

    }

    //--------------------------------------------------------------------------------------------------
    //
    //
    vk::Pipeline create(const vk::PipelineCache& cache)
    {

    }

    //--------------------------------------------------------------------------------------------------
    //
    //
    vk::Pipeline create()
    {

    }

    //--------------------------------------------------------------------------------------------------
    //
    //
    void destroyShaderModules()
    {

    }

    //--------------------------------------------------------------------------------------------------
    //
    //
    void update()
    {
    }

private:
    //--------------------------------------------------------------------------------------------------
    //
    //
    void init()
    {
    }

public:

    //--------------------------------------------------------------------------------------------------
    // Override Default pipeline associated structures
    //
    struct PipelineRasterizationStateCreateInfo
    {};
    struct PipelineInputAssemblyStateCreateInfo
    {};
    struct PipelineColorBlendAttachmentState
    {};
    struct PipelineColorBlendStateCreateInfo
    {};
    struct PipelineDynamicStateCreateInfo
    {};
    struct PipelineVertexInputStateCreateInfo
    {};
    struct PipelineViewportStateCreateInfo
    {};
    struct PipelineDepthStencilStateCreateInfo
    {};

    const vk::Device&                              device;
    vk::GraphicsPipelineCreateInfo                 pipelineCreateInfo;
    vk::PipelineCache                              pipelineCache;
    vk::RenderPass&                                renderPass{ pipelineCreateInfo.renderPass };
    vk::PipelineLayout&                            layout{ pipelineCreateInfo.layout };

}; // struct GraphicsPipelineGenerator


//--------------------------------------------------------------------------------------------------
//
//
template <typename T>
vk::PipelineShaderStageCreateInfo& app::GraphicsPipelineGenerator::addShader()
{

}

} // namespace app