/*
 *
 * Andrew Frost
 * examplevulkan.cpp
 * 2020
 *
 */

#define VMA_IMPLEMENTATION

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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
                         uint32_t                  graphicsQueueIdx,
                         uint32_t                  presentQueueIdx,
                         const vk::Extent2D&       size)
{
    m_allocator.init(device, physicalDevice, instance);
    m_device = device;
    m_physicalDevice = physicalDevice;
    m_graphicsQueueIdx = graphicsQueueIdx;
    m_presentQueueIdx = presentQueueIdx;
    m_size = size;
}

//--------------------------------------------------------------------------------------------------
// Destroy all Allocations
//
void ExampleVulkan::destroy()
{
    m_device.destroy(m_graphicsPipeline);
    m_device.destroy(m_pipelineLayout);
    m_device.destroy(m_descriptorPool);
    m_device.destroy(m_descriptorSetLayout);
    m_allocator.destroy(m_cameraMat);
    m_allocator.destroy(m_sceneDesc);

    for (auto& model : m_objModel)
    {
        m_allocator.destroy(model.vertexBuffer);
        m_allocator.destroy(model.indexBuffer);
        m_allocator.destroy(model.matColorBuffer);
    }

    for (auto& texture : m_textures)
    {
        m_allocator.destroy(texture);
    }

    // Post 
    m_device.destroy(m_postPipeline);
    m_device.destroy(m_postPipelineLayout);
    m_device.destroy(m_postDescriptorPool);
    m_device.destroy(m_postDescriptorSetLayout);
    m_allocator.destroy(m_offscreenColor);
    m_allocator.destroy(m_offscreenDepth);
    m_device.destroy(m_offscreenRenderPass);
    m_device.destroy(m_offscreenFramebuffer);
}

//--------------------------------------------------------------------------------------------------
// called when resizing of the window
//
void ExampleVulkan::resize(const vk::Extent2D& size)
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
        m.ambient  = glm::pow(m.ambient, glm::vec3(2.2f));
        m.diffuse  = glm::pow(m.diffuse, glm::vec3(2.2f));
        m.specular = glm::pow(m.specular, glm::vec3(2.2f));
    }

    ObjInstance instance;
    instance.objIndex    = static_cast<uint32_t>(m_objModel.size());
    instance.transform   = transform;
    instance.transformIT = glm::inverseTranspose(transform);
    instance.txtOffset   = static_cast<uint32_t>(m_textures.size());

    ObjModel model;
    model.nIndices  = static_cast<uint32_t>(loader.m_indices.size());
    model.nVertices = static_cast<uint32_t>(loader.m_vertices.size());

    // create buffers on device and copy vertices, indices and materials
    app::SingleCommandBuffer cmdBufferGet(m_device, m_graphicsQueueIdx);
    vk::CommandBuffer commandBuffer = cmdBufferGet.createCommandBuffer();
    model.vertexBuffer   = m_allocator.createBuffer(loader.m_vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    model.indexBuffer    = m_allocator.createBuffer(loader.m_indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    model.matColorBuffer = m_allocator.createBuffer(loader.m_materials, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    // creates all textures found
    createTextureImages(commandBuffer, loader.m_textures);
    cmdBufferGet.flushCommandBuffer(commandBuffer);
    m_allocator.flushStaging();

    //std::string objNb = std::to_string(instance.objIndex);
    //m_debug.setObjectName(model.vertexBuffer.buffer, (std::string("vertex_" + objNb).c_str()));
    //m_debug.setObjectName(model.indexBuffer.buffer, (std::string("index_" + objNb).c_str()));
    //m_debug.setObjectName(model.matColorBuffer.buffer, (std::string("mat_" + objNb).c_str()));

    m_objModel.emplace_back(model);
    m_objInstance.emplace_back(instance);
}

//--------------------------------------------------------------------------------------------------
// Create textures and samplers
//
void ExampleVulkan::createTextureImages(const vk::CommandBuffer& cmdBuffer, 
                                        const std::vector<std::string>& textures)
{
    vk::SamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.magFilter  = vk::Filter::eLinear;
    samplerCreateInfo.minFilter  = vk::Filter::eLinear;
    samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerCreateInfo.maxLod     = FLT_MAX;

    vk::Format format = vk::Format::eR8G8B8A8Srgb;

    // if no textures are present, create a dummy one to accomodate the pipeline layout
    if (textures.empty() && m_textures.empty()) {
        app::TextureDedicated texture;

        glm::u8vec4* color = new glm::u8vec4(255, 255, 255, 255);
        vk::DeviceSize bufferSize = sizeof(glm::u8vec4);
        vk::Extent2D   imgSize = vk::Extent2D(1, 1);
        auto           imageCreateInfo = app::image::create2DInfo(imgSize, format);

        // Creating the vkImage
        texture = m_allocator.createImage(cmdBuffer, bufferSize, color, imageCreateInfo);

        // Setting up the descriptor used by the shader
        texture.descriptor = app::image::create2DDescriptor(m_device, texture.image, samplerCreateInfo, format);

        // The image format must be in VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        m_textures.push_back(texture);
    }
    else {
        // Uploading all images
        for (const auto& texture : textures) {

        }
    }
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
    m_descriptorPool      = app::util::createDescriptorPool(m_device, m_descSetLayoutBind, 1);
    m_descriptorSet       = app::util::createDescriptorSet(m_device, m_descriptorPool, m_descriptorSetLayout);
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

//--------------------------------------------------------------------------------------------------
// 
//
void ExampleVulkan::createOffscreenRender()
{
    m_allocator.destroy(m_offscreenColor);
    m_allocator.destroy(m_offscreenDepth);

    // creating the color image
    {
        vk::ImageCreateInfo colorCreateInfo = app::image::create2DInfo(m_size, m_offscreenColorFormat,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage);

        try {
            m_offscreenColor = m_allocator.createImage(colorCreateInfo);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to create image!");
        }

        m_offscreenColor.descriptor = app::image::create2DDescriptor(m_device, m_offscreenColor.image,
            vk::SamplerCreateInfo{}, m_offscreenColorFormat, vk::ImageLayout::eGeneral);
    }

    // creating the depth buffer
    {
        vk::ImageCreateInfo depthCreateInfo = app::image::create2DInfo(m_size, m_offscreenDepthFormat,
            vk::ImageUsageFlagBits::eDepthStencilAttachment);

        try {
            m_offscreenDepth = m_allocator.createImage(depthCreateInfo);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to create image!");
        }

        vk::ImageViewCreateInfo depthStencilView = {};
        depthStencilView.viewType         = vk::ImageViewType::e2D;
        depthStencilView.format           = m_offscreenDepthFormat;
        depthStencilView.subresourceRange = { vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 };
        depthStencilView.image            = m_offscreenDepth.image;

        try {
            m_offscreenDepth.descriptor.imageView = m_device.createImageView(depthStencilView);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to create image!");
        }
    }

    // setting the image layout for both color and depth
    {
        app::SingleCommandBuffer commandBufferGen(m_device, m_graphicsQueueIdx);
        vk::CommandBuffer commandBuffer = commandBufferGen.createCommandBuffer();

        app::image::setImageLayout(
            commandBuffer, m_offscreenColor.image,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);

        app::image::setImageLayout(
            commandBuffer, m_offscreenDepth.image,
            vk::ImageAspectFlagBits::eDepth, vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthStencilAttachmentOptimal);

        commandBufferGen.flushCommandBuffer(commandBuffer);
    }

    // creating a render pass for the offscreen
    if (!m_offscreenRenderPass) {
        m_offscreenRenderPass = app::util::createRenderPass(
            m_device, { m_offscreenColorFormat }, m_offscreenDepthFormat,
            1, true, true, vk::ImageLayout::eGeneral, vk::ImageLayout::eGeneral);
    }

    // creating the frambuffer for offscreen
    {
        std::vector<vk::ImageView> attachments = { m_offscreenColor.descriptor.imageView,
                                                   m_offscreenDepth.descriptor.imageView };

        m_device.destroy(m_offscreenFramebuffer);

        vk::FramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.renderPass      = m_offscreenRenderPass;
        framebufferInfo.attachmentCount = 2;
        framebufferInfo.pAttachments    = attachments.data();
        framebufferInfo.width           = m_size.width;
        framebufferInfo.height          = m_size.height;
        framebufferInfo.layers          = 1;

        try {
            m_offscreenFramebuffer = m_device.createFramebuffer(framebufferInfo);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to create offscreen framebuffer!");
        }
    }
}

//--------------------------------------------------------------------------------------------------
// 
//
void ExampleVulkan::createPostPipeline(const vk::RenderPass& renderPass)
{
}

//--------------------------------------------------------------------------------------------------
// 
//
void ExampleVulkan::createPostDescriptor()
{
}

//--------------------------------------------------------------------------------------------------
// 
//
void ExampleVulkan::updatePostDescriptorSet()
{
}

//--------------------------------------------------------------------------------------------------
//
//
void ExampleVulkan::drawPost(vk::CommandBuffer cmdBuf)
{
}
