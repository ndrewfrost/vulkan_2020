/*
 *
 * Andrew Frost
 * examplevulkan.cpp
 * 2020
 *
 */

#include "examplevulkan.h"

 ///////////////////////////////////////////////////////////////////////////
 // ExampleVulkan
 ///////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
// Initialize vk variables to do all buffer and image allocations
//
void ExampleVulkan::init(const vk::Device& device,
                         const vk::PhysicalDevice& physicalDevice,
                         const vk::Instance& instance,
                         uint32_t graphicsFamilyIdx, 
                         const vk::Extent2D& size)
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device         = device;
    allocatorInfo.instance       = instance;

    vmaCreateAllocator(&allocatorInfo, &m_allocator);

    m_device         = device;
    m_physicalDevice = physicalDevice;
    m_instance       = instance;
    m_graphicsIdx    = graphicsFamilyIdx;
    m_size           = size;
}

//--------------------------------------------------------------------------------------------------
// Destroy all Allocations
//
void ExampleVulkan::destroy()
{
}

//--------------------------------------------------------------------------------------------------
// 
//
void ExampleVulkan::resize(const vk::Extent2D& size)
{
}

//--------------------------------------------------------------------------------------------------
// 
//
void ExampleVulkan::rasterize(const vk::CommandBuffer& cmdBuffer)
{
}

//--------------------------------------------------------------------------------------------------
// 
//
void ExampleVulkan::createDescripotrSetLayout()
{
}

//--------------------------------------------------------------------------------------------------
// 
//
void ExampleVulkan::createGraphicsPipeline(const vk::RenderPass& renderPass)
{
}

//--------------------------------------------------------------------------------------------------
// Loading the OBJ file and setting up all buffers
//
void ExampleVulkan::loadModel(const std::string& filename, glm::mat4 transform)
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
void ExampleVulkan::createTextureImages(const vk::CommandBuffer& cmdBuffer, const std::vector<std::string>& textures)
{
}

//--------------------------------------------------------------------------------------------------
// 
//
void ExampleVulkan::updateUniformBuffer()
{
}