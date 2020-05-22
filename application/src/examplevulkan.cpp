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
// called when resizing of the window
//
void ExampleVulkan::resize(const vk::Extent2D& size)
{
    m_size = size;
    createOffscreenRender();
    updatePostDescriptorSet();
}

//--------------------------------------------------------------------------------------------------
// Draw scene in raster mode
//
void ExampleVulkan::rasterize(const vk::CommandBuffer& cmdBuffer)
{
    vk::DeviceSize offset{ 0 };

    m_debug.beginLabel(cmdBuffer, "Rasterize");

    // Dynamic Viewport
    vk::Viewport viewport = {};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (float) m_size.width;
    viewport.height   = (float) m_size.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor = {};
    scissor.offset = vk::Offset2D{ 0,0 };
    scissor.extent = m_size;

    cmdBuffer.setViewport(0, {viewport});
    cmdBuffer.setScissor(0, {scissor});

    // Drawing all traingles
    cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline);
    cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 0, { m_descriptorSet }, {});

    for (int i = 0; i < m_objInstance.size(); ++i) {
        auto& instance = m_objInstance[i];
        auto& model = m_objModel[instance.objIndex];
        m_pushConstant.instanceId = i; // which instance to draw

        cmdBuffer.pushConstants<ObjPushConstant>(m_pipelineLayout, 
                                                 vk::ShaderStageFlagBits::eVertex 
                                                 | vk::ShaderStageFlagBits::eFragment,
                                                 0, m_pushConstant);

        cmdBuffer.bindVertexBuffers(0,1, &model.vertexBuffer.buffer, &offset);
        cmdBuffer.bindIndexBuffer(model.indexBuffer.buffer, 0, vk::IndexType::eUint32);
        cmdBuffer.drawIndexed(model.nbIndices, 1, 0, 0, 0);
    }

    m_debug.endLabel(cmdBuffer);
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
// TODO: FIX Buffer Staging (Maybe build another class)
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
    vmaDestroyBuffer(m_allocator, model.vertexBuffer.buffer,   model.vertexBuffer.allocation);
    vmaDestroyBuffer(m_allocator, model.indexBuffer.buffer,    model.indexBuffer.allocation);
    vmaDestroyBuffer(m_allocator, model.matColorBuffer.buffer, model.matColorBuffer.allocation);
    
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