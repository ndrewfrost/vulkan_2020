/*
 *
 * Andrew Frost
 * examplevulkan.h
 * 2020
 *
 */

#pragma once

class ExampleVulkan
{
public:

    void init(const vk::Device& device, 
              const vk::PhysicalDevice& physicalDevice,
              uint32_t graphicsFamilyIdx,
              uint32_t presentFamilyIdx,
              const vk::Extent2D& size);

    void destroy();

    void resize();

    void rasterize();

    void createDescripotrSetLayout();

    void createGraphicsPipeline(const vk::RenderPass& renderPass);

    void loadModel();

    void updateDescriptorSet();

    void createUniformBuffer();

    void createSceneDescriptionBuffer();

    void createTextureImages();

    void updateUniformBuffer();


    // Graphics Pipeline
    vk::PipelineLayout      m_pipelineLayout;
    vk::Pipeline            m_graphicsPipeline;

    vk::DescriptorPool      m_descriptorPool;
    vk::DescriptorSetLayout m_descPoolLayout;
    vk::DescriptorSet       m_descriptorSet;

    //app::Allocator          m_alloc;
    vk::Device              m_device;
    vk::PhysicalDevice      m_physicalDevice;
    vk::Extent2D            m_size;
    uint32_t                m_graphicsIdx{0};

    // # Post

}; // class ExampleVulkan