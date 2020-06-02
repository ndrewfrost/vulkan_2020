/*
 *
 * Andrew Frost
 * descriptorsets.hpp
 * 2020
 *
 */

#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

namespace app {

///////////////////////////////////////////////////////////////////////////
// Descriptor Sets Helpers                                               //
///////////////////////////////////////////////////////////////////////////
namespace util {

inline vk::DescriptorPool createDescriptorPool(
    vk::Device device, size_t poolSizeCount, const vk::DescriptorPoolSize* poolSizes, uint32_t maxSets);

inline vk::DescriptorPool createDescriptorPool(
    vk::Device device, const std::vector<vk::DescriptorPoolSize>& poolSizes, uint32_t maxSets);


inline vk::DescriptorSet allocateDescriptorSet(
    vk::Device device, vk::DescriptorPool pool, vk::DescriptorSetLayout layout);

inline void allocateDescriptorSets(
    vk::Device device, vk::DescriptorPool pool, vk::DescriptorSetLayout layout, uint32_t count,
    std::vector<vk::DescriptorSet>& sets);

} // namespace util

///////////////////////////////////////////////////////////////////////////
// Descriptor Set Bindings                                               //
///////////////////////////////////////////////////////////////////////////

class DescriptorSetBindings
{
public:
    //-------------------------------------------------------------------------
    // Constructors
    //
    DescriptorSetBindings() = default;
    DescriptorSetBindings(const std::vector<vk::DescriptorSetLayoutBinding>& bindings)
        : m_bindings(bindings) {}

    //-------------------------------------------------------------------------
    // Clear Bindings
    //
    void clear()
    {
        m_bindings.clear();
        m_bindingFlags.clear();
    }

    //-------------------------------------------------------------------------
    // Add binding to descriptor set
    //
    void addBinding(
        uint32_t             slot,    // slot which the descriptor will be bound
        vk::DescriptorType   type,       // type of bound descriptor(s)
        uint32_t             count,      // number of descriptors
        vk::ShaderStageFlags stageFlags, // Shader stage, when the bound resources will be available
        const vk::Sampler* pImmutableSampler = nullptr) // sampler in case of textures
    {
        vk::DescriptorSetLayoutBinding binding = {};
        binding.binding            = slot;
        binding.descriptorType     = type;
        binding.descriptorCount    = count;
        binding.stageFlags         = stageFlags;
        binding.pImmutableSamplers = pImmutableSampler;

        m_bindings.push_back(binding);
    }

    void addBinding(const VkDescriptorSetLayoutBinding& layoutBinding) { m_bindings.emplace_back(layoutBinding); }

    void setBindings(const std::vector<vk::DescriptorSetLayoutBinding>& bindings) { m_bindings = bindings; }

    void setBindingFlags(uint32_t binding, vk::DescriptorBindingFlags bindingFlag);
        
    //-------------------------------------------------------------------------
    // Create Descriptor Layouts and Pools
    //
    vk::DescriptorSetLayout createLayout( vk::Device device, 
        vk::DescriptorSetLayoutCreateFlags flags = vk::DescriptorSetLayoutCreateFlags()) const;

    vk::DescriptorPool createPool(vk::Device device, uint32_t maxSets = 1) const;

    void addRequiredPoolSizes(std::vector<vk::DescriptorPoolSize>& poolSizes, uint32_t numSets) const;

    //-------------------------------------------------------------------------
    // Write Descriptor Sets 
    //
    vk::WriteDescriptorSet makeWrite(
        vk::DescriptorSet dstSet,
        uint32_t          dstBinding, 
        uint32_t          arrayElement = 0) const;
    
    vk::WriteDescriptorSet makeWrite(
        vk::DescriptorSet              dstSet,
        uint32_t                       dstBinding,
        const vk::DescriptorImageInfo* pImageInfo,
        uint32_t                       arrayElement = 0) const;

    vk::WriteDescriptorSet makeWrite(
        vk::DescriptorSet               dstSet,
        uint32_t                        dstBinding,
        const vk::DescriptorBufferInfo* pBufferInfo,
        uint32_t                        arrayElement = 0) const;

    vk::WriteDescriptorSet makeWrite(
        vk::DescriptorSet     dstSet,
        uint32_t              dstBinding,
        const vk::BufferView* pTexelBufferView,
        uint32_t              arrayElement = 0) const;

    vk::WriteDescriptorSet makeWrite(
        vk::DescriptorSet                                    dstSet,
        uint32_t                                             dstBinding,
        const vk::WriteDescriptorSetAccelerationStructureNV* pAccel,
        uint32_t                                             arrayElement = 0) const;

    vk::WriteDescriptorSet makeWrite(
        vk::DescriptorSet                                  dstSet,
        uint32_t                                           dstBinding,
        const vk::WriteDescriptorSetInlineUniformBlockEXT* pInlineUniform,
        uint32_t                                           arrayElement = 0) const;


    //-------------------------------------------------------------------------
    // Write Arrays 
    //
    vk::WriteDescriptorSet makeWriteArray(vk::DescriptorSet dstSet, uint32_t dstBinding) const;

    vk::WriteDescriptorSet makeWriteArray(vk::DescriptorSet dstSet, uint32_t dstBinding,
        const vk::DescriptorImageInfo* pImageInfo) const;

    vk::WriteDescriptorSet makeWriteArray(vk::DescriptorSet dstSet, uint32_t dstBinding,
        const vk::DescriptorBufferInfo* pBufferInfo) const;

    vk::WriteDescriptorSet makeWriteArray(vk::DescriptorSet dstSet, uint32_t dstBinding,
        const vk::BufferView* pTexelBufferView) const;

    vk::WriteDescriptorSet makeWriteArray(vk::DescriptorSet dstSet, uint32_t dstBinding,
        const vk::WriteDescriptorSetAccelerationStructureNV* pAccel) const;

    vk::WriteDescriptorSet makeWriteArray(vk::DescriptorSet dstSet, uint32_t dstBinding,
        const vk::WriteDescriptorSetInlineUniformBlockEXT* pInline) const;

    //-------------------------------------------------------------------------
    // Getters
    //
    bool                                  empty() const { return m_bindings.empty(); }
    size_t                                size()  const { return m_bindings.size(); }
    const vk::DescriptorSetLayoutBinding* data()  const { return m_bindings.data(); }

    vk::DescriptorType getType(uint32_t binding) const;
    uint32_t           getCount(uint32_t binding) const;

    

private:
    std::vector<vk::DescriptorSetLayoutBinding> m_bindings;
    std::vector<vk::DescriptorBindingFlags>     m_bindingFlags;
};

class DescriptorSetContainer
{

};

} // namespace app