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

#include "../external/obj_loader.h"

#include "../vk_helpers/utilities.hpp"
#include "../vk_helpers/renderpass.hpp"
#include "../vk_helpers/images.hpp"

#include "../vk_helpers/pipeline.hpp"
#include "../general_helpers/manipulator.h"
#include "../vk_helpers/commands.hpp"

#include "../vk_helpers/vulkanbackend.hpp"
#include "../vk_helpers/debug.hpp"
#include "../vk_helpers/descriptorsets.hpp"
#include "../vk_helpers/allocator.hpp"

 ///////////////////////////////////////////////////////////////////////////
 // Example Vulkan                                                        //
 ///////////////////////////////////////////////////////////////////////////
 // Simple Rasterizer of OBJ objects                                      //
 // - Each Obj loaded are stored in an 'ObjModel' and refrenced by        //
 //   'ObjInstance'                                                       //
 // - Rendering is done in an offscreen framebuffer                       //
 // - The image of framebuffer is displayed in post-process in a          //
 //   fullscreen quad                                                     //
 ///////////////////////////////////////////////////////////////////////////

class ExampleVulkan : public app::VulkanBackend
{
public:
    void setupVulkan(const app::ContextCreateInfo& info, GLFWwindow* window) override;

    void destroyResources();

    void onResize(int /*w*/, int /*h*/) override;

    void loadModel(const std::string& filename, glm::mat4 transform = glm::mat4(1));

    void createTextureImages(const vk::CommandBuffer& cmdBuffer,
                             const std::vector<std::string>& textures);

    void createDescriptorSetLayout();

    void createGraphicsPipeline();

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
        uint32_t       nIndices{ 0 };
        uint32_t       nVertices{ 0 };
        app::BufferVma vertexBuffer;   // Device buffer of all vertex
        app::BufferVma indexBuffer;    // Device buffer of all indices forming triangles
        app::BufferVma matColorBuffer; // Device buffer of array of wavefront material
        app::BufferVma matIndexBuffer; // Device buffer of array of Wavefront material
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
    std::vector<ObjModel>        m_objModel;
    std::vector<ObjInstance>     m_objInstance;

    // Graphic pipeline
    vk::PipelineLayout           m_pipelineLayout;
    vk::Pipeline                 m_graphicsPipeline;
    app::DescriptorSetBindings   m_descSetLayoutBind;
    vk::DescriptorPool           m_descriptorPool;
    vk::DescriptorSetLayout      m_descriptorSetLayout;
    vk::DescriptorSet            m_descriptorSet;

    app::BufferVma               m_cameraMat;  // Device-Host of the camera matrices
    app::BufferVma               m_sceneDesc;  // Device buffer of the OBJ instances
    std::vector<app::TextureVma> m_textures;   // vector of all textures of the scene
    
    app::Allocator               m_allocator;
    app::debug::DebugUtil        m_debug;

///////////////////////////////////////////////////////////////////////////
// Post-processing                                                       //
///////////////////////////////////////////////////////////////////////////

    void createOffscreenRender();

    void createPostDescriptor();

    void createPostPipeline();

    void updatePostDescriptorSet();

    void drawPost(vk::CommandBuffer cmdBuffer);

    app::DescriptorSetBindings m_postDescSetLayoutBind;
    vk::DescriptorPool         m_postDescriptorPool;
    vk::DescriptorSetLayout    m_postDescriptorSetLayout;
    vk::DescriptorSet          m_postDescriptorSet;

    vk::Pipeline               m_postPipeline;
    vk::PipelineLayout         m_postPipelineLayout;
    vk::RenderPass             m_offscreenRenderPass;
    vk::Framebuffer            m_offscreenFramebuffer;

    app::TextureVma            m_offscreenColor;
    app::TextureVma            m_offscreenDepth;
    app::TextureVma            m_offscreenResolve;
    vk::Format                 m_offscreenColorFormat  { vk::Format::eR32G32B32A32Sfloat };
    vk::Format                 m_offscreenDepthFormat  { vk::Format::eD32Sfloat };
    vk::Format                 m_offscreenResolveFormat{ vk::Format::eR32G32B32A32Sfloat };
    
}; // class ExampleVulkan