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
//
//
void ExampleVulkan::createGraphicsPipeline(const vk::RenderPass& renderPass)
{
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