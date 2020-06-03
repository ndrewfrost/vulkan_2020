/*
 *
 * Andrew Frost
 * examplevulkan.cpp
 * 2020
 *
 */

#define VMA_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "examplevulkan.hpp"

///////////////////////////////////////////////////////////////////////////
// ExampleVulkan                                                         //
///////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------
// Initialize vk variables to do all buffer and image allocations
//
void ExampleVulkan::setupVulkan(const app::ContextCreateInfo& info, GLFWwindow* window)
{
    VulkanBackend::setupVulkan(info, window);
    m_allocator.init(m_device, m_physicalDevice, m_instance);
#if _DEBUG
    m_debug.setup(m_device, m_instance);
#endif
}

//-------------------------------------------------------------------------
// Destroy all Allocations
//
void ExampleVulkan::destroyResources()
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

//-------------------------------------------------------------------------
// called when resizing of the window
//
void ExampleVulkan::onWindowResize(uint32_t width, uint32_t height)
{
    createOffscreenRender();
    updatePostDescriptorSet();
}

//-------------------------------------------------------------------------
// Loading the OBJ file and setting up all buffers
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
    app::CommandPool cmdBufferGet(m_device, m_graphicsQueueIdx);
    vk::CommandBuffer commandBuffer = cmdBufferGet.createBuffer();
    model.vertexBuffer   = m_allocator.createBuffer(commandBuffer, loader.m_vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    model.indexBuffer    = m_allocator.createBuffer(commandBuffer, loader.m_indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    model.matColorBuffer = m_allocator.createBuffer(commandBuffer, loader.m_materials, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    // creates all textures found
    createTextureImages(commandBuffer, loader.m_textures);
    cmdBufferGet.submitAndWait(commandBuffer);
    m_allocator.finalizeAndReleaseStaging();

#if _DEBUG
    std::string objNb = std::to_string(instance.objIndex);
    m_debug.setObjectName(model.vertexBuffer.buffer, (std::string("vertex_" + objNb).c_str()));
    m_debug.setObjectName(model.indexBuffer.buffer, (std::string("index_" + objNb).c_str()));
    m_debug.setObjectName(model.matColorBuffer.buffer, (std::string("mat_" + objNb).c_str()));
#endif

    m_objModel.emplace_back(model);
    m_objInstance.emplace_back(instance);
}

//-------------------------------------------------------------------------
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
        app::TextureVma texture;

        glm::u8vec4*   color      = new glm::u8vec4(255, 255, 255, 255);
        vk::DeviceSize bufferSize = sizeof(glm::u8vec4);
        vk::Extent2D   imgSize    = vk::Extent2D(1, 1);
        
        vk::ImageCreateInfo imageCreateInfo = app::image::create2DInfo(imgSize, format);

        // Creating the dummy texture
        app::ImageVma image = m_allocator.createImage(cmdBuffer, bufferSize, reinterpret_cast<stbi_uc*>(color), imageCreateInfo);
        vk::ImageViewCreateInfo imageViewCreateInfo = app::image::makeImageViewCreateInfo(image.image, imageCreateInfo);
        texture = m_allocator.createTexture(image, imageViewCreateInfo, samplerCreateInfo);

        // The image format must be in VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        app::image::cmdBarrierImageLayout(cmdBuffer, texture.image, vk::ImageLayout::eUndefined,
                                          vk::ImageLayout::eShaderReadOnlyOptimal);
        m_textures.push_back(texture);
    }
    else {
        // Uploading all images
        for (const auto& texture : textures) {
            std::stringstream ss;
            int texWidth, texHeight, texChannels;
            ss << "../media/textures/" << texture;

            stbi_uc* pixels =
                stbi_load(ss.str().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

            // Handle failure
            if (!pixels)
            {
                texWidth = texHeight = 1;
                texChannels = 4;
                glm::u8vec4* color = new glm::u8vec4(255, 0, 255, 255);
                pixels = reinterpret_cast<stbi_uc*>(color);
            }

            vk::DeviceSize bufferSize = static_cast<uint64_t>(texWidth) * texHeight * sizeof(glm::u8vec4);
            auto imageSize = vk::Extent2D(texWidth, texHeight);
            auto imageCreateInfo = app::image::create2DInfo(imageSize, format, vk::ImageUsageFlagBits::eSampled, true);

            app::ImageVma image = m_allocator.createImage(cmdBuffer, bufferSize, pixels, imageCreateInfo);
            app::image::generateMipmaps(cmdBuffer, image.image, format, imageSize, imageCreateInfo.mipLevels);
            
            vk::ImageViewCreateInfo imageViewCreateInfo = app::image::makeImageViewCreateInfo(image.image, imageCreateInfo);

            app::TextureVma texture = m_allocator.createTexture(image, imageViewCreateInfo, samplerCreateInfo);

            m_textures.push_back(texture);
        }
    }
}

//-------------------------------------------------------------------------
// Describing the layout pushed when rendering
//
void ExampleVulkan::createDescriptorSetLayout()
{
    uint32_t nTextures = static_cast<uint32_t>(m_textures.size());
    uint32_t nObjects  = static_cast<uint32_t>(m_objModel.size());

    // Camera matrices (binding = 0)
    vk::DescriptorSetLayoutBinding bindingCamera = {};
    bindingCamera.binding         = 0;
    bindingCamera.descriptorType  = vk::DescriptorType::eUniformBuffer;
    bindingCamera.descriptorCount = 1;
    bindingCamera.stageFlags      = vk::ShaderStageFlagBits::eVertex;
    m_descSetLayoutBind.addBinding(bindingCamera);

    // Materials (binding = 1)
    vk::DescriptorSetLayoutBinding bindingMat = {};
    bindingMat.binding         = 1;
    bindingMat.descriptorType  = vk::DescriptorType::eStorageBuffer;
    bindingMat.descriptorCount = nObjects;
    bindingMat.stageFlags      = vk::ShaderStageFlagBits::eVertex
                               | vk::ShaderStageFlagBits::eFragment;
    m_descSetLayoutBind.addBinding(bindingMat);

    // Scene Decription (binding = 2)
    vk::DescriptorSetLayoutBinding bindingScene = {};
    bindingScene.binding         = 2;
    bindingScene.descriptorType  = vk::DescriptorType::eStorageBuffer;
    bindingScene.descriptorCount = 1;
    bindingScene.stageFlags      = vk::ShaderStageFlagBits::eVertex
                                 | vk::ShaderStageFlagBits::eFragment;
    m_descSetLayoutBind.addBinding(bindingScene);

    // Textures (binding = 3)
    vk::DescriptorSetLayoutBinding bindingTextures = {};
    bindingTextures.binding         = 3;
    bindingTextures.descriptorType  = vk::DescriptorType::eCombinedImageSampler;
    bindingTextures.descriptorCount = nTextures;
    bindingTextures.stageFlags      = vk::ShaderStageFlagBits::eFragment;
    m_descSetLayoutBind.addBinding(bindingTextures);

    // Materials (binding = 4)
    vk::DescriptorSetLayoutBinding bindingMat = {};
    bindingMat.binding         = 4;
    bindingMat.descriptorType  = vk::DescriptorType::eStorageBuffer;
    bindingMat.descriptorCount = nObjects;
    bindingMat.stageFlags      = vk::ShaderStageFlagBits::eFragment;
    m_descSetLayoutBind.addBinding(bindingMat);

    m_descriptorSetLayout = m_descSetLayoutBind.createLayout(m_device);
    m_descriptorPool      = m_descSetLayoutBind.createPool(m_device, 1);
    m_descriptorSet       = app::util::allocateDescriptorSet(m_device, m_descriptorPool, m_descriptorSetLayout);
}

//-------------------------------------------------------------------------
// Creating the pipeline layout
//
void ExampleVulkan::createGraphicsPipeline()
{
    vk::PushConstantRange pushConstantRanges = { vk::ShaderStageFlagBits::eVertex
                                               | vk::ShaderStageFlagBits::eFragment,
                                                 0, sizeof(ObjPushConstant) };

    // Create Pipeline Layout
    vk::DescriptorSetLayout descriptorSetLayout(m_descriptorSetLayout);
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
    pipelineGenerator.addShader(app::util::readFile("shaders/frag_shader.frag.spv"), vk::ShaderStageFlagBits::eFragment);
    pipelineGenerator.multisampleState.rasterizationSamples  = vk::SampleCountFlagBits::e1;
    pipelineGenerator.vertexInputState.bindingDescriptions   = { {0, sizeof(Vertex)} };
    pipelineGenerator.vertexInputState.attributeDescriptions = {
      {0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)},
      {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)},
      {2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)},
      {3, 0, vk::Format::eR32G32Sfloat,    offsetof(Vertex, texCoord)}};

    m_graphicsPipeline = pipelineGenerator.create();
#if _DEBUG
    m_debug.setObjectName(m_graphicsPipeline, "graphicsPipeline");
#endif
}

//-------------------------------------------------------------------------
// Creating the uniform buffer holding the camera matrices
// - Buffer is host visible
//
void ExampleVulkan::createUniformBuffer()
{
    m_cameraMat = m_allocator.createBuffer(sizeof(CameraMatrices), vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
#if _DEBUG
    m_debug.setObjectName(m_cameraMat.buffer, "cameraMatBuffer");
#endif
}

//-------------------------------------------------------------------------
// Create a storage buffer containing the description of the scene elements
// - Which geometry is used by which instance
// - Transformation
// - Offset for texture
//
void ExampleVulkan::createSceneDescriptionBuffer()
{
    app::CommandPool commandGen(m_device, m_graphicsQueueIdx);
    auto commandBuffer = commandGen.createBuffer();

    m_sceneDesc = m_allocator.createBuffer(commandBuffer, m_objInstance, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    commandGen.submitAndWait(commandBuffer);
    m_allocator.finalizeAndReleaseStaging();

#if _DEBUG
    m_debug.setObjectName(m_sceneDesc.buffer, "sceneDescBuffer");
#endif
}

//-------------------------------------------------------------------------
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
    writes.emplace_back(m_descSetLayoutBind.makeWrite(m_descriptorSet, 0, &cameraBufferInfo));
    
    // Scene Description
    vk::DescriptorBufferInfo SceneBufferInfo = {};
    SceneBufferInfo.buffer = m_sceneDesc.buffer;
    SceneBufferInfo.offset = 0;
    SceneBufferInfo.range  = VK_WHOLE_SIZE;
    writes.emplace_back(m_descSetLayoutBind.makeWrite(m_descriptorSet, 2, &SceneBufferInfo));

    // All material buffers, 1 buffer per Obj
    std::vector<vk::DescriptorBufferInfo> materialBuffersInfo;
    std::vector<vk::DescriptorBufferInfo> materialBuffersIdxInfo;

    for (size_t i = 0; i < m_objModel.size(); ++i) {
        materialBuffersInfo.push_back({ m_objModel[i].matColorBuffer.buffer, 0, VK_WHOLE_SIZE });
        materialBuffersIdxInfo.push_back({ m_objModel[i].matIndexBuffer.buffer, 0, VK_WHOLE_SIZE });
    }
    writes.emplace_back(m_descSetLayoutBind.makeWriteArray(m_descriptorSet, 1, materialBuffersInfo.data()));
    writes.emplace_back(m_descSetLayoutBind.makeWriteArray(m_descriptorSet, 4, materialBuffersIdxInfo.data()));

    // All texture samplers
    std::vector<vk::DescriptorImageInfo> textureImageInfo;
    for (size_t i = 0; i < m_textures.size(); ++i) {
        textureImageInfo.push_back(m_textures[i].descriptor);
    }
    writes.emplace_back(m_descSetLayoutBind.makeWriteArray(m_descriptorSet, 3, textureImageInfo.data()));

    // writing the information
    m_device.updateDescriptorSets(static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

//-------------------------------------------------------------------------
// Called at each frame to update the camera matrix
//
void ExampleVulkan::updateUniformBuffer()
{
    const float aspectRatio = m_size.width / static_cast<float>(m_size.height);

    CameraMatrices ubo = {};
    ubo.view = CameraManipulator.getMatrix();
    ubo.proj = glm::perspective(glm::radians(65.0f), aspectRatio, 0.1f, 1000.0f);
    ubo.proj[1][1] *= -1;  // Inverting Y for Vulkan
    ubo.viewInverse = glm::inverse(ubo.view);

    void* data;
    vmaMapMemory(m_allocator.getAllocator(), m_cameraMat.allocation, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vmaUnmapMemory(m_allocator.getAllocator(), m_cameraMat.allocation);
}

//-------------------------------------------------------------------------
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

///////////////////////////////////////////////////////////////////////////
// Post-processing                                                       //
///////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------
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

        app::ImageVma image = {};

        try {
            image = m_allocator.createImage(colorCreateInfo);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to create image!");
        }
        
        vk::ImageViewCreateInfo imageViewCreateInfo = app::image::makeImageViewCreateInfo(image.image, colorCreateInfo);
        m_offscreenColor = m_allocator.createTexture(image, imageViewCreateInfo, vk::SamplerCreateInfo());
        m_offscreenColor.descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    }

    // creating the depth buffer
    {
        vk::ImageCreateInfo depthCreateInfo = 
            app::image::create2DInfo(m_size, m_offscreenDepthFormat,
                                     vk::ImageUsageFlagBits::eDepthStencilAttachment);

        app::ImageVma image = {};

        try {
            app::ImageVma image = m_allocator.createImage(depthCreateInfo);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to create image!");
        }

        vk::ImageViewCreateInfo depthStencilView = {};
        depthStencilView.viewType         = vk::ImageViewType::e2D;
        depthStencilView.format           = m_offscreenDepthFormat;
        depthStencilView.subresourceRange = { vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 };
        depthStencilView.image            = image.image;

        try {
            m_offscreenDepth = m_allocator.createTexture(image, depthStencilView);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to create image!");
        }
    }

    // setting the image layout for both color and depth
    {
        app::CommandPool commandBufferGen(m_device, m_graphicsQueueIdx);
        vk::CommandBuffer commandBuffer = commandBufferGen.createBuffer();

        app::image::cmdBarrierImageLayout(
            commandBuffer, m_offscreenColor.image,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);

        app::image::cmdBarrierImageLayout(
            commandBuffer, m_offscreenDepth.image, vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthStencilAttachmentOptimal,
            vk::ImageAspectFlagBits::eDepth);

        commandBufferGen.submitAndWait(commandBuffer);
    }

    // creating a render pass for the offscreen
    if (!m_offscreenRenderPass) {
        m_offscreenRenderPass = app::util::createRenderPass(
            m_device, { m_offscreenColorFormat }, m_offscreenDepthFormat,
            1, true, true, vk::ImageLayout::eGeneral, vk::ImageLayout::eGeneral);
#if _DEBUG
        m_debug.setObjectName(m_offscreenRenderPass, "offscreenRenderPass");
#endif
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

//-------------------------------------------------------------------------
// The descriptor layout is the description of the data that is passed to 
// the vertex or the fragment program.
//
void ExampleVulkan::createPostDescriptor()
{
    vk::DescriptorSetLayoutBinding postBinding = {};
    postBinding.binding         = 0;
    postBinding.descriptorType  = vk::DescriptorType::eCombinedImageSampler;
    postBinding.descriptorCount = 1;
    postBinding.stageFlags      = vk::ShaderStageFlagBits::eFragment;
    m_postDescSetLayoutBind.addBinding(postBinding);
    
    m_postDescriptorSetLayout = m_descSetLayoutBind.createLayout(m_device);
    m_postDescriptorPool      = m_descSetLayoutBind.createPool(m_device);
    m_postDescriptorSet       = app::util::allocateDescriptorSet(m_device, m_postDescriptorPool, m_postDescriptorSetLayout);
}

//-------------------------------------------------------------------------
// create Post Pipeline
//
void ExampleVulkan::createPostPipeline()
{
    vk::PushConstantRange pushConstantRanges = {vk::ShaderStageFlagBits::eFragment,
                                                 0, sizeof(float) };

    // Create Pipeline Layout
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.setLayoutCount         = 1;
    pipelineLayoutCreateInfo.pSetLayouts            = &m_postDescriptorSetLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges    = &pushConstantRanges;

    try {
        m_postPipelineLayout = m_device.createPipelineLayout(pipelineLayoutCreateInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    // Create the Pipeline
    app::GraphicsPipelineGenerator pipelineGenerator(m_device, m_postPipelineLayout, m_renderPass);

    pipelineGenerator.addShader(app::util::readFile("shaders/passthrough.vert.spv"), vk::ShaderStageFlagBits::eVertex);
    pipelineGenerator.addShader(app::util::readFile("shaders/post.frag.spv"), vk::ShaderStageFlagBits::eFragment);
    pipelineGenerator.multisampleState.setRasterizationSamples(m_sampleCount);
    pipelineGenerator.rasterizationState.setCullMode(vk::CullModeFlagBits::eNone);
    m_postPipeline = pipelineGenerator.create();
#if _DEBUG
    m_debug.setObjectName(m_postPipeline, "postPipeline");
#endif
}

//-------------------------------------------------------------------------
// Update the output
//
void ExampleVulkan::updatePostDescriptorSet()
{
    vk::WriteDescriptorSet writeDescSet =
        m_postDescSetLayoutBind.makeWrite(m_postDescriptorSet, 0, reinterpret_cast<vk::DescriptorImageInfo*>(&m_offscreenColor.descriptor));
    m_device.updateDescriptorSets(writeDescSet, nullptr);
}

//-------------------------------------------------------------------------
// Draw a full screen quad with the attached image
//
void ExampleVulkan::drawPost(vk::CommandBuffer cmdBuffer)
{
    vk::Viewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_size.width;
    viewport.height = (float)m_size.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor = {};
    scissor.offset = vk::Offset2D{ 0,0 };
    scissor.extent = m_size;

    cmdBuffer.setViewport(0, { viewport });
    cmdBuffer.setScissor(0, { scissor });

    const float aspectRatio = m_size.width / static_cast<float>(m_size.height);

    cmdBuffer.pushConstants<float>(m_postPipelineLayout, vk::ShaderStageFlagBits::eFragment, 
                                   0, aspectRatio);

    cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_postPipeline);

    cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_postPipelineLayout, 
                                 0, m_postDescriptorSet, {});

    cmdBuffer.draw(3, 1, 0, 0);
}