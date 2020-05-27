/*
 *
 * Andrew Frost
 * examplevulkan.cpp
 * 2020
 *
 */

#include "examplevulkan.hpp"

///////////////////////////////////////////////////////////////////////////
// ExampleVulkan
///////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
// Initialize vk variables to do all buffer and image allocations
//
void ExampleVulkan::init(const vk::Device&         device, 
                         const vk::PhysicalDevice& physicalDevice, 
                         const vk::Instance&       instance, 
                         uint32_t                  graphicsFamilyIdx,
                         uint32_t                  presentFamilyIdx,
                         const vk::Extent2D&       size)
{
}

//--------------------------------------------------------------------------------------------------
// Destroy all Allocations
//
void ExampleVulkan::destroy()
{
}

//--------------------------------------------------------------------------------------------------
// called when resizing of the window
//
void ExampleVulkan::resize(const vk::Extent2D& size)
{
}

//--------------------------------------------------------------------------------------------------
//
//
void ExampleVulkan::loadModel(const std::string& filename, glm::mat4 transform)
{
}

//--------------------------------------------------------------------------------------------------
//
//
void ExampleVulkan::createTextureImages(const vk::CommandBuffer& cmdBuffer, 
                                        const std::vector<std::string>& textures)
{
}

//--------------------------------------------------------------------------------------------------
// Describing the layout pushed when rendering
//
void ExampleVulkan::createDescriptorSetLayout()
{
    uint32_t nTextures = static_cast<uint32_t>(m_textures.size());
    uint32_t nObjects  = static_cast<uint32_t>(m_objModel.size());

    // Camera matrices   (binding = 0)
    vk::DescriptorSetLayoutBinding binding = {};
    binding.binding         = 0;
    binding.descriptorType  = vk::DescriptorType::eUniformBuffer;
    binding.descriptorCount = 1;
    binding.stageFlags      = vk::ShaderStageFlagBits::eVertex;
    m_descSetLayoutBind.emplace_back(binding);

    // Materials        (binding = 1)
    binding.binding         = 1;
    binding.descriptorType  = vk::DescriptorType::eStorageBuffer;
    binding.descriptorCount = nObjects;
    binding.stageFlags      = vk::ShaderStageFlagBits::eVertex
                            | vk::ShaderStageFlagBits::eFragment;
    m_descSetLayoutBind.emplace_back(binding);

    // Scene Decription (binding = 2)
    binding.binding         = 2;
    binding.descriptorType  = vk::DescriptorType::eStorageBuffer;
    binding.descriptorCount = nTextures;
    binding.stageFlags      = vk::ShaderStageFlagBits::eVertex
                            | vk::ShaderStageFlagBits::eFragment;
    m_descSetLayoutBind.emplace_back(binding);

    // Textures         (binding = 3)
    binding.binding         = 3;
    binding.descriptorType  = vk::DescriptorType::eCombinedImageSampler;
    binding.descriptorCount = 1;
    binding.stageFlags      = vk::ShaderStageFlagBits::eFragment;
    m_descSetLayoutBind.emplace_back(binding);

    m_descriptorSetLayout = app::util::createDescriptorSetLayout(m_device, m_descSetLayoutBind);
    m_descriptorPool = app::util::createDescriptorPool(m_device, m_descSetLayoutBind, 1);
    m_descriptorSet = app::util::createDescriptorSet(m_device, m_descriptorPool, m_descriptorSetLayout);
}

//--------------------------------------------------------------------------------------------------
// Creating the pipeline layout
//
void ExampleVulkan::createGraphicsPipeline(const vk::RenderPass& renderPass)
{
    vk::PushConstantRange pushConstantRanges = { vk::ShaderStageFlagBits::eVertex
                                               | vk::ShaderStageFlagBits::eFragment,
                                                 0, sizeof(ObjPushConstant) };

    // Create Pipeline Layout
    vk::DescriptorSetLayout      descriptorSetLayout(m_descriptorSetLayout);
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRanges;
    
    try {
        m_pipelineLayout = m_device.createPipelineLayout(pipelineLayoutCreateInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    // Create the Pipeline
    app::GraphicsPipelineGenerator pipelineGenerator(m_device, m_pipelineLayout, m_offscreenRenderPass);
    pipelineGenerator.depthStencilState = { true };
    pipelineGenerator.addShader(app::util::readFile("shaders/vert_shader.vert.spv"), vk::ShaderStageFlagBits::eVertex);
    pipelineGenerator.addShader(app::util::readFile("shaders/frag_shader.vert.spv"), vk::ShaderStageFlagBits::eFragment);
    pipelineGenerator.vertexInputState.bindingDescriptions = { {0, sizeof(Vertex)} };
    pipelineGenerator.vertexInputState.attributeDescriptions = {
      {0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)},
      {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)},
      {2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)},
      {3, 0, vk::Format::eR32G32Sfloat,    offsetof(Vertex, texCoord)},
      {4, 0, vk::Format::eR32Sint,         offsetof(Vertex, matID)} };;

    m_graphicsPipeline = pipelineGenerator.create();
    m_debug.setObjectName(m_graphicsPipeline, "Graphics Pipeline");
}

//--------------------------------------------------------------------------------------------------
//
//
void ExampleVulkan::createUniformBuffer()
{
}

//--------------------------------------------------------------------------------------------------
//
//
void ExampleVulkan::createSceneDescriptionBuffer()
{
}

//--------------------------------------------------------------------------------------------------
//
//
void ExampleVulkan::updateDescriptorSet()
{
}

//--------------------------------------------------------------------------------------------------
//
//
void ExampleVulkan::updateUniformBuffer()
{
}

//--------------------------------------------------------------------------------------------------
//
//
void ExampleVulkan::rasterize(const vk::CommandBuffer& cmdBuffer)
{
}

//////////////////////////////////////////////////////////////////////////
// Post-processing
//////////////////////////////////////////////////////////////////////////