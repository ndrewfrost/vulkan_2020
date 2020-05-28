/*
 *
 * Andrew Frost
 * examplevulkan.hpp
 * 2020
 *
 */

#pragma once

#include <sstream>
#include "vulkan/vulkan.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_inverse.hpp"

#include "..//external/obj_loader.h"
#include "commands.hpp"
#include "camera.h"
#include "pipeline.hpp"
#include "allocator.hpp"
#include "utilities.hpp"
#include "descriptorsets.hpp"
#include "renderpass.hpp"
//#include "debugutil.hpp"
#include "images.hpp"

///////////////////////////////////////////////////////////////////////////
// ExampleVulkan
///////////////////////////////////////////////////////////////////////////

class ExampleVulkan
{
public:

    void init(const vk::Device&         device,
              const vk::PhysicalDevice& physicalDevice,
              const vk::Instance&       instance,
              uint32_t                  graphicsFamilyIdx,
              uint32_t                  presentFamilyIdx,
              const vk::Extent2D&       size);

    void destroy();

    void resize(const vk::Extent2D& size);

    void loadModel(const std::string& filename, glm::mat4 transform = glm::mat4(1));

    void createTextureImages(const vk::CommandBuffer& cmdBuffer,
                             const std::vector<std::string>& textures);

    void createDescriptorSetLayout();

    void createGraphicsPipeline(const vk::RenderPass& renderPass);

    void createUniformBuffer();

    void createSceneDescriptionBuffer();

    void updateDescriptorSet();

    void updateUniformBuffer();

    void rasterize(const vk::CommandBuffer& cmdBuffer);

    // Holding the camera matrices
    struct CameraMatrices
    {
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 viewInverse;
    };

    // OBJ representation of a vertex
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec2 texCoord;
        int       matID = 0;
    };

    // The OBJ model
    struct ObjModel
    {
        uint32_t             nIndices{ 0 };
        uint32_t             nVertices{ 0 };
        app::BufferDedicated vertexBuffer;   // Device buffer of all vertex
        app::BufferDedicated indexBuffer;    // Device buffer of all indices forming triangles
        app::BufferDedicated matColorBuffer; // Device buffer of array of wavefront material
    };

    // Instance of the OBJ
    struct ObjInstance
    {
        uint32_t  objIndex{ 0 };    // Reference to the 'm_objModel'
        uint32_t  txtOffset{ 0 };   // Offset in 'm_textures'
        glm::mat4 transform{ 1 };   // Position of the instance
        glm::mat4 transformIT{ 1 }; // Inverse Transpose
    };

    // Information pushed at each draw call
    struct ObjPushConstant
    {
        glm::vec3 lightPosition{ 10.f, 15.f, 8.f };
        int       instanceId{ 0 };                  // To retrieve the transformation matrix
        float     lightIntensity{ 100.f };
        int       lightType{ 0 };                   // 0: point, 1: infinite
    };
    ObjPushConstant m_pushConstant;

    // Array of objects and instances in the scene
    std::vector<ObjModel>    m_objModel;
    std::vector<ObjInstance> m_objInstance;

    // Graphic pipeline
    vk::PipelineLayout                          m_pipelineLayout;
    vk::Pipeline                                m_graphicsPipeline;
    std::vector<vk::DescriptorSetLayoutBinding> m_descSetLayoutBind;
    vk::DescriptorPool                          m_descriptorPool;
    vk::DescriptorSetLayout                     m_descriptorSetLayout;
    vk::DescriptorSet                           m_descriptorSet;

    app::BufferDedicated               m_cameraMat;  // Device-Host of the camera matrices
    app::BufferDedicated               m_sceneDesc;  // Device buffer of the OBJ instances
    std::vector<app::TextureDedicated> m_textures;   // vector of all textures of the scene

    app::Allocator      m_allocator;
    vk::Device          m_device;  
    vk::PhysicalDevice  m_physicalDevice;
    uint32_t            m_graphicsQueueIdx{ 0 };
    uint32_t            m_presentQueueIdx{ 0 };
    vk::Extent2D        m_size;         

///////////////////////////////////////////////////////////////////////////
// Post-processing
///////////////////////////////////////////////////////////////////////////

    void createOffscreenRender();

    void createPostDescriptor();

    void createPostPipeline(const vk::RenderPass& renderPass);

    void updatePostDescriptorSet();

    void drawPost(vk::CommandBuffer cmdBuf);

    std::vector<vk::DescriptorSetLayoutBinding> m_postDescSetLayoutBind;
    vk::DescriptorPool      m_postDescriptorPool;
    vk::DescriptorSetLayout m_postDescriptorSetLayout;
    vk::DescriptorSet       m_postDescriptorSet;

    vk::Pipeline            m_postPipeline;
    vk::PipelineLayout      m_postPipelineLayout;
    vk::RenderPass          m_offscreenRenderPass;
    vk::Framebuffer         m_offscreenFramebuffer;

    app::TextureDedicated   m_offscreenColor;
    app::TextureDedicated   m_offscreenDepth;
    vk::Format              m_offscreenColorFormat{ vk::Format::eR32G32B32A32Sfloat };
    vk::Format              m_offscreenDepthFormat{ vk::Format::eD32Sfloat };
    
}; // class ExampleVulkan