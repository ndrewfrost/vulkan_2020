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

    m_debug.setup(m_device);
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
    ObjLoader<Vertex> loader;
    loader.loadModel(filename);

    // convert srgb to linear
    for (auto& m : loader.m_materials) {
        m.ambient = glm::pow(m.ambient, glm::vec3(2.2f));
        m.diffuse = glm::pow(m.diffuse, glm::vec3(2.2f));
        m.specular = glm::pow(m.specular, glm::vec3(2.2f));
    }

    ObjInstance instance;
    instance.objIndex    = static_cast<uint32_t>(m_objModel.size());
    instance.transform   = transform;
    instance.transformIT = glm::inverseTranspose(transform);
    instance.txtOffset   = static_cast<uint32_t>(m_textures.size());

    ObjModel model;
    model.nbIndices = static_cast<uint32_t>(loader.m_indices.size());
    model.nbVertices = static_cast<uint32_t>(loader.m_vertices.size());

    // create buffers on device and copy vertices, indices and materials
    app::SingleCommandBuffer cmdBufferGet(m_device, m_graphicsIdx);
    vk::CommandBuffer commandBuffer = cmdBufferGet.createCommandBuffer();

    VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    VmaAllocationCreateInfo allocInfo = {};

    bufferInfo.size  = sizeof(loader.m_vertices) * loader.m_vertices.size();
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    allocInfo.usage  = VMA_MEMORY_USAGE_CPU_TO_GPU;
    vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &model.vertexBuffer.buffer, &model.vertexBuffer.allocation, nullptr);
    
    bufferInfo.size  = sizeof(loader.m_indices) * loader.m_indices.size();
    bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    allocInfo.usage  = VMA_MEMORY_USAGE_CPU_TO_GPU;
    vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &model.indexBuffer.buffer,    &model.indexBuffer.allocation, nullptr);
    
    bufferInfo.size  = sizeof(loader.m_materials) * loader.m_materials.size();
    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    allocInfo.usage  = VMA_MEMORY_USAGE_CPU_TO_GPU;
    vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &model.matColorBuffer.buffer, &model.matColorBuffer.allocation, nullptr);
    
    // creates all textures found
    createTextureImages(commandBuffer, loader.m_textures);
    cmdBufferGet.flushCommandBuffer(commandBuffer);
    
    std::string objNb = std::to_string(instance.objIndex);
    m_debug.setObjectName(model.vertexBuffer.buffer, (std::string("vertex_" + objNb).c_str()));
    m_debug.setObjectName(model.indexBuffer.buffer, (std::string("index_" + objNb).c_str()));
    m_debug.setObjectName(model.matColorBuffer.buffer, (std::string("mat_" + objNb).c_str()));

    m_objModel.emplace_back(model);
    m_objInstance.emplace_back(instance);
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

//////////////////////////////////////////////////////////////////////////
// Post-processing
//////////////////////////////////////////////////////////////////////////