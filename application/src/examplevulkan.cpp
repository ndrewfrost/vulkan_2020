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
    m_size = size;
    createOffscreenRender();
    updatePostDescriptorSet();
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

    ObjInstance instance = {};
    instance.objIndex    = static_cast<uint32_t>(m_objModel.size());
    instance.transform   = transform;
    instance.transformIT = glm::inverseTranspose(transform);
    instance.txtOffset   = static_cast<uint32_t>(m_textures.size());

    ObjModel model = {};
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
// Creating the uniform buffer holding the camera matrices
// - Buffer is host visible
//
void ExampleVulkan::createUniformBuffer()
{
    m_cameraMat = m_allocator.createBuffer(sizeof(CameraMatrices), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

//--------------------------------------------------------------------------------------------------
// Create a storage buffer containing the description of the scene elements
// - Which geometry is used by which instance
// - Transformation
// - Offset for texture
//
void ExampleVulkan::createSceneDescriptionBuffer()
{
    app::SingleCommandBuffer commandGen(m_device, m_graphicsQueueIdx);
    auto commandBuffer = commandGen.createCommandBuffer();
    m_sceneDesc = m_allocator.createBuffer(commandBuffer, m_objInstance, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    commandGen.flushCommandBuffer(commandBuffer);
}

//--------------------------------------------------------------------------------------------------
// Setting up the buffers in the descriptor set
//
void ExampleVulkan::updateDescriptorSet()
{
    std::vector<vk::WriteDescriptorSet> writes;

    // Camera Matrices
    vk::DescriptorBufferInfo cameraBufferInfo = {};
    cameraBufferInfo.buffer = m_cameraMat.buffer;
    cameraBufferInfo.offset = 0;
    cameraBufferInfo.range  = VK_WHOLE_SIZE;
    writes.emplace_back(app::util::createWrites(m_descriptorSet, m_descSetLayoutBind[0], &cameraBufferInfo));
    
    // Scene Description
    vk::DescriptorBufferInfo SceneBufferInfo = {};
    SceneBufferInfo.buffer = m_sceneDesc.buffer;
    SceneBufferInfo.offset = 0;
    SceneBufferInfo.range  = VK_WHOLE_SIZE;
    writes.emplace_back(app::util::createWrites(m_descriptorSet, m_descSetLayoutBind[2], &SceneBufferInfo));

    // All material buffers, 1 buffer per Obj
    std::vector<vk::DescriptorBufferInfo> materialBuffersInfo;
    for (size_t i = 0; i < m_objModel.size(); ++i) {
        materialBuffersInfo.push_back({ m_objModel[i].matColorBuffer.buffer, 0, VK_WHOLE_SIZE });
    }
    writes.emplace_back(app::util::createWrite(m_descriptorSet, m_descSetLayoutBind[1], materialBuffersInfo.data()));

    // All texture samplers
    std::vector<vk::DescriptorImageInfo> textureImageInfo;
    for (size_t i = 0; i < m_textures.size(); ++i) {
        textureImageInfo.push_back(m_textures[i].descriptor);
    }
    writes.emplace_back(app::util::createWrites(m_descriptorSet, m_descSetLayoutBind[3], textureImageInfo.data()));

    // writing the information
    m_device.updateDescriptorSets(static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

//--------------------------------------------------------------------------------------------------
// Called at each frame to update the camera matrix
//
void ExampleVulkan::updateUniformBuffer()
{
    const float aspectRatio = m_size.width / static_cast<float>(m_size.height);

    CameraMatrices ubo = {};
    ubo.view = CameraView.getMatrix();
    ubo.proj = glm::perspective(glm::radians(65.0f), aspectRatio, 0.1f, 1000.0f);
    ubo.proj[1][1] *= -1;  // Inverting Y for Vulkan
    ubo.viewInverse = glm::inverse(ubo.view);

    void* data;
    vmaMapMemory(m_allocator.getAllocator(), m_cameraMat.allocation, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vmaUnmapMemory(m_allocator.getAllocator(), m_cameraMat.allocation);
}

//--------------------------------------------------------------------------------------------------
// Drawing the scene in raster mode
//
void ExampleVulkan::rasterize(const vk::CommandBuffer& cmdBuffer)
{
    vk::DeviceSize offset{ 0 };

    // Dynamic Viewport
    vk::Viewport viewport = {};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (float)m_size.width;
    viewport.height   = (float)m_size.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor = {};
    scissor.offset = vk::Offset2D{ 0,0 };
    scissor.extent = m_size;

    cmdBuffer.setViewport(0, { viewport });
    cmdBuffer.setScissor(0, { scissor });

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

        cmdBuffer.bindVertexBuffers(0, 1, &vk::Buffer(model.vertexBuffer.buffer), &offset);
        cmdBuffer.bindIndexBuffer(model.indexBuffer.buffer, 0, vk::IndexType::eUint32);
        cmdBuffer.drawIndexed(model.nIndices, 1, 0, 0, 0);
    }
}

//////////////////////////////////////////////////////////////////////////
// Post-processing
//////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
// Creating an offscreen frame buffer and the associated render pass
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
// The pipeline is how things are rendered, which shaders, type of primitives, depth test and more
//
void ExampleVulkan::createPostPipeline(const vk::RenderPass& renderPass)
{
}

//--------------------------------------------------------------------------------------------------
// The descriptor layout is the description of the data that is passed to the vertex or the
// fragment program.
//
void ExampleVulkan::createPostDescriptor()
{
}

//--------------------------------------------------------------------------------------------------
// Update the output
//
void ExampleVulkan::updatePostDescriptorSet()
{
}

//--------------------------------------------------------------------------------------------------
// Draw a full screen quad with the attached image
//
void ExampleVulkan::drawPost(vk::CommandBuffer cmdBuf)
{
}
