/*
 *
 * Andrew Frost
 * examplevulkan.h
 * 2020
 *
 */

#pragma once

#define VMA_IMPLEMENTATION
#include "..//external/vk_mem_alloc.h"
#include "vulkan/vulkan.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_inverse.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

    void createDescripotrSetLayout();

    void createGraphicsPipeline(const vk::RenderPass& renderPass);

    void loadModel(const std::string& filename, glm::mat4 transform = glm::mat4(1));

    void updateDescriptorSet();

    void createUniformBuffer();

    void createSceneDescriptionBuffer();

    void createTextureImages(const vk::CommandBuffer& cmdBuffer,
                             const std::vector<std::string>& textures);

    void updateUniformBuffer();


    // Graphics Pipeline
    vk::PipelineLayout      m_pipelineLayout;
    vk::Pipeline            m_graphicsPipeline;

    vk::DescriptorPool      m_descriptorPool;
    vk::DescriptorSetLayout m_descPoolLayout;
    vk::DescriptorSet       m_descriptorSet;

    VmaAllocator            m_allocator;
    vk::Device              m_device;
    vk::PhysicalDevice      m_physicalDevice;
    vk::Instance            m_instance;
    vk::Extent2D            m_size;
    uint32_t                m_graphicsIdx{0};

    // # Post

}; // class ExampleVulkan