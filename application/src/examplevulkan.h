/*
 *
 * Andrew Frost
 * examplevulkan.h
 * 2020
 *
 */

#pragma once

#include <sstream>

#define VMA_IMPLEMENTATION
#include "..//external/vk_mem_alloc.h"
#include "vulkan/vulkan.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_inverse.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "..//external/obj_loader.h"
#include "commands.hpp"
#include "debugutil.hpp"



 ///////////////////////////////////////////////////////////////////////////
 // ExampleVulkan
 ///////////////////////////////////////////////////////////////////////////

class ExampleVulkan
{
public:

    void init(const vk::Device& device, 
              const vk::PhysicalDevice& physicalDevice,
              const vk::Instance& instance,
              uint32_t graphicsFamilyIdx,
              const vk::Extent2D& size);

    void destroy();

    void resize(const vk::Extent2D& size);

    void rasterize(const vk::CommandBuffer& cmdBuffer);

    void createDescriptorSetLayout();

    void createGraphicsPipeline(const vk::RenderPass& renderPass);

    void loadModel(const std::string& filename, glm::mat4 transform = glm::mat4(1));

    void updateDescriptorSet();

    void createUniformBuffer();

    void createSceneDescriptionBuffer();

    void createTextureImages(const vk::CommandBuffer& cmdBuffer,
                             const std::vector<std::string>& textures);

    void updateUniformBuffer();

    

    //---------------------------------------------------------------------------
    // Objects
    //
    struct BufferDedicated
    {
        VkBuffer         buffer;
        VmaAllocation    allocation;
    };

    struct ImageDedicated
    {
        VkImage          image;
        VmaAllocation    allocation;
    };

    struct TextureDedicated
    {
        vk::DescriptorImageInfo descriptor;

        TextureDedicated& operator=(const ImageDedicated& buffer)
        {
            static_cast<ImageDedicated&>(*this) = buffer;
            return *this;
        }
    };

    struct AccelerationDedicated
    {
        vk::AccelerationStructureNV accel;
        vk::DeviceMemory            allocation;
    };

    //---------------------------------------------------------------------------
    //
    //
    
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
        glm::vec3 pos;
        glm::vec3 nrm;
        glm::vec3 color;
        glm::vec2 texCoord;
        int       matID = 0;
    };

    // OBJ Model
    struct ObjModel
    {
        uint32_t  nbIndices{ 0 };
        uint32_t  nbVertices{ 0 };
        BufferDedicated vertexBuffer;   // Device buffer of all vertex
        BufferDedicated indexBuffer;    // Device buffer of all indices forming triangles
        BufferDedicated matColorBuffer; // Device buffer of array of 'wavefront material'
    };

    // OBJ Instance
    struct ObjInstance
    {
        uint32_t  objIndex{ 0 };    // Reference to m_objModel
        uint32_t  txtOffset{ 0 };   // offset in m_textures
        glm::mat4 transform{ 0 };   // Position of the instance
        glm::mat4 transformIT{ 0 }; // Inverse Transpose
    };

    // Information pushed at each draw call
    struct ObjPushConstant
    {
        glm::vec3 lightPosition{ 10.f, 15.f, 8.f };
        int       instanceId{ 0 }; // To retrieve the transformation matrix
        float     lightIntensity{ 100.f };
        int       lightType{ 0 };  // 0: point, 1: infinite
    };
    ObjPushConstant m_pushConstant;

    // Objects and instances in the scene
    std::vector<ObjModel>    m_objModel;
    std::vector<ObjInstance> m_objInstance;

    // Graphics Pipeline
    vk::PipelineLayout      m_pipelineLayout;
    vk::Pipeline            m_graphicsPipeline;

    std::vector<vk::DescriptorSetLayoutBinding> m_descSetLayoutBinding;
    vk::DescriptorPool                          m_descriptorPool;
    vk::DescriptorSetLayout                     m_descPoolLayout;
    vk::DescriptorSet                           m_descriptorSet;

    BufferDedicated               m_cameraMat;  // Device-Host of the camera matrices
    BufferDedicated               m_sceneDesc;  // Device buffer of the OBJ instances
    std::vector<TextureDedicated> m_textures;   // vector of all textures of the scene

    VmaAllocator            m_allocator; // VMA Allocator
    app::DebugUtil          m_debug;
    vk::Device              m_device;
    vk::PhysicalDevice      m_physicalDevice;
    vk::Instance            m_instance;
    vk::Extent2D            m_size;
    uint32_t                m_graphicsIdx{0};


    //////////////////////////////////////////////////////////////////////////
    // Post-processing
    //////////////////////////////////////////////////////////////////////////

    void createOffscreenRender();
    void createPostPipeline(const vk::RenderPass& renderPass);
    void createPostDescriptor();
    void createPostDescriptorSet();
    void updatePostDescriptorSet();
    void drawPost(vk::CommandBuffer commandBuffer);

    std::vector<vk::DescriptorSetLayoutBinding> m_postDescSetLayoutBinding;
    vk::DescriptorPool                          m_postDescriptorPool;
    vk::DescriptorSetLayout                     m_postDescPoolLayout;
    vk::DescriptorSet                           m_postDescriptorSet;

    vk::Pipeline                                m_postPipeline;
    vk::PipelineLayout                          m_postPipelineLayout;

    vk::RenderPass                              m_offscreenRenderPass;
    vk::Framebuffer                             m_offscreenFramebuffer;

}; // class ExampleVulkan