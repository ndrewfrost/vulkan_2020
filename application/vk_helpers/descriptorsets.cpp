/*
 *
 * Andrew Frost
 * descriptorsets.cpp
 * 2020
 *
 */

#include "descriptorsets.hpp"

namespace app {

///////////////////////////////////////////////////////////////////////////
// Descriptor Set Helpers                                                //
///////////////////////////////////////////////////////////////////////////

namespace util {
//-------------------------------------------------------------------------
//
//
vk::DescriptorPool createDescriptorPool(vk::Device device, size_t poolSizeCount,
    const vk::DescriptorPoolSize* poolSizes, uint32_t maxSets)
{
    vk::DescriptorPoolCreateInfo poolInfo = {};
    poolInfo.maxSets = maxSets;
    poolInfo.poolSizeCount = uint32_t(poolSizeCount);
    poolInfo.pPoolSizes = poolSizes;

    try {
        return device.createDescriptorPool(poolInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

//-------------------------------------------------------------------------
//
//
vk::DescriptorPool createDescriptorPool(vk::Device device,
    const std::vector<vk::DescriptorPoolSize>& poolSizes, uint32_t maxSets)
{
    return createDescriptorPool(device, poolSizes.size(), poolSizes.data(), maxSets);
}

//-------------------------------------------------------------------------
//
//
vk::DescriptorSet allocateDescriptorSet(vk::Device device, vk::DescriptorPool pool,
    vk::DescriptorSetLayout layout)
{
    vk::DescriptorSetAllocateInfo allocInfo(pool, 1, &layout);

    try {
        vk::DescriptorSet set = device.allocateDescriptorSets(allocInfo)[0];
        return set;
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to allocate descriptor set!");
    }
}

//-------------------------------------------------------------------------
//
//
void allocateDescriptorSets(vk::Device device, vk::DescriptorPool pool,
    vk::DescriptorSetLayout layout, uint32_t count, std::vector<vk::DescriptorSet>& sets)
{
    sets.resize(count);
    std::vector<vk::DescriptorSetLayout> layouts(count, layout);

    vk::DescriptorSetAllocateInfo allocInfo = {};
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = count;
    allocInfo.pSetLayouts = layouts.data();

    try {
        sets = device.allocateDescriptorSets(allocInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }
}

} // namespace util

///////////////////////////////////////////////////////////////////////////
// Descriptor Set Bindings                                               //
///////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------
// requires use of SUPPORT_INDEXING_EXT/SUPPORT_INDEXING_V1_2 on createLayout
//
void DescriptorSetBindings::setBindingFlags(uint32_t binding, vk::DescriptorBindingFlags bindingFlag)
{
    for (size_t i = 0; i < m_bindings.size(); i++) {

        if (m_bindings[i].binding == binding) {

            if (m_bindingFlags.size() <= i) {
                m_bindingFlags.resize(i + 1, vk::DescriptorBindingFlags());
            }

            m_bindingFlags[i] = bindingFlag;
            return;
        }
    }
    assert(0 && "binding not found");
}

//-------------------------------------------------------------------------
// get descriptorType of binding at location
//
vk::DescriptorType DescriptorSetBindings::getType(uint32_t binding) const
{
    for (size_t i = 0; i < m_bindings.size(); i++) {
        if (m_bindings[i].binding == binding) {
            return m_bindings[i].descriptorType;
        }
    }

    assert(0 && "binding not found");
    return;
}

//-------------------------------------------------------------------------
// get descriptorCount of binding at location
//
uint32_t DescriptorSetBindings::getCount(uint32_t binding) const
{
    for (size_t i = 0; i < m_bindings.size(); i++) {
        if (m_bindings[i].binding == binding) {
            return m_bindings[i].descriptorCount;
        }
    }

    assert(0 && "binding not found");
    return ~0;
}

//-------------------------------------------------------------------------
// generates the descriptor layout corresponding to the bound resources.
//
vk::DescriptorSetLayout DescriptorSetBindings::createLayout(vk::Device device, vk::DescriptorSetLayoutCreateFlags flags) const
{
    vk::DescriptorSetLayoutBindingFlagsCreateInfo extendedInfo{};
    extendedInfo.pNext         = nullptr;
    extendedInfo.bindingCount  = static_cast<uint32_t>(m_bindingFlags.size());
    extendedInfo.pBindingFlags = m_bindingFlags.data();

    vk::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(m_bindings.size());
    layoutCreateInfo.pBindings    = m_bindings.data();
    layoutCreateInfo.flags        = flags;
    layoutCreateInfo.pNext        = m_bindingFlags.empty() ? nullptr : &extendedInfo;

    try {
        vk::DescriptorSetLayout layout = device.createDescriptorSetLayout(layoutCreateInfo);
        return layout;
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

//-------------------------------------------------------------------------
// generates the descriptor pool with enough space to handle all the 
// bound resources and allocate up to maxSets descriptor sets
//
vk::DescriptorPool DescriptorSetBindings::createPool(vk::Device device, uint32_t maxSets) const
{
    // Aggregate the bindings to obtain the required size of the descriptors using that layout
    std::vector<vk::DescriptorPoolSize> poolSizes;
    addRequiredPoolSizes(poolSizes, maxSets);

    for (const auto& b : m_bindings)
        poolSizes.emplace_back(b.descriptorType, b.descriptorCount);

    // Create the pool information descriptor, contains the number of descriptors of each type
    vk::DescriptorPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolCreateInfo.pPoolSizes    = poolSizes.data();
    poolCreateInfo.maxSets       = maxSets;

    try {
        vk::DescriptorPool pool = device.createDescriptorPool(poolCreateInfo);
        return pool;
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

//-------------------------------------------------------------------------
// appends the required poolsizes for N sets
//
void DescriptorSetBindings::addRequiredPoolSizes(std::vector<vk::DescriptorPoolSize>& poolSizes, uint32_t numSets) const
{
    for (auto it = m_bindings.cbegin(); it != m_bindings.cend(); ++it) {
        bool found = false;

        for (auto itpool = poolSizes.begin(); itpool != poolSizes.end(); ++itpool) {
            
            if (itpool->type == it->descriptorType) {
                itpool->descriptorCount += it->descriptorCount * numSets;
                found = true;
                break;
            }

        }
        if (!found) {
            vk::DescriptorPoolSize poolSize;
            poolSize.type = it->descriptorType;
            poolSize.descriptorCount = it->descriptorCount * numSets;
            poolSizes.push_back(poolSize);
        }
    }
}

//-------------------------------------------------------------------------
// Write Descriptor Sets 
//
vk::WriteDescriptorSet DescriptorSetBindings::makeWrite(vk::DescriptorSet dstSet, uint32_t dstBinding, 
    uint32_t arrayElement = 0) const
{
    for (size_t i = 0; i < m_bindings.size(); i++) {
        if (m_bindings[i].binding == dstBinding)
            return { dstSet, dstBinding, arrayElement, 1, m_bindings[i].descriptorType };
    }
    assert(0 && "binding not found");
    return {};
}

vk::WriteDescriptorSet DescriptorSetBindings::makeWrite(vk::DescriptorSet dstSet, uint32_t dstBinding, 
    const vk::DescriptorImageInfo* pImageInfo, uint32_t arrayElement = 0) const
{
    vk::WriteDescriptorSet writeSet = makeWrite(dstSet, dstBinding, arrayElement);
    assert(writeSet.descriptorType == vk::DescriptorType::eSampler ||
           writeSet.descriptorType == vk::DescriptorType::eCombinedImageSampler ||
           writeSet.descriptorType == vk::DescriptorType::eSampledImage ||
           writeSet.descriptorType == vk::DescriptorType::eStorageImage ||
           writeSet.descriptorType == vk::DescriptorType::eInputAttachment);

    writeSet.pImageInfo = pImageInfo;
    return writeSet;
}

vk::WriteDescriptorSet DescriptorSetBindings::makeWrite(vk::DescriptorSet dstSet, uint32_t dstBinding, 
    const vk::DescriptorBufferInfo* pBufferInfo, uint32_t arrayElement = 0) const
{
    vk::WriteDescriptorSet writeSet = makeWrite(dstSet, dstBinding, arrayElement);
    assert(writeSet.descriptorType == vk::DescriptorType::eStorageBuffer ||
           writeSet.descriptorType == vk::DescriptorType::eStorageBufferDynamic ||
           writeSet.descriptorType == vk::DescriptorType::eUniformBuffer ||
           writeSet.descriptorType == vk::DescriptorType::eUniformBufferDynamic);

    writeSet.pBufferInfo = pBufferInfo;
    return writeSet;
}

vk::WriteDescriptorSet DescriptorSetBindings::makeWrite(vk::DescriptorSet dstSet, uint32_t dstBinding, 
    const vk::BufferView* pTexelBufferView, uint32_t arrayElement = 0) const
{
    vk::WriteDescriptorSet writeSet = makeWrite(dstSet, dstBinding, arrayElement);
    assert(writeSet.descriptorType == vk::DescriptorType::eUniformTexelBuffer ||
           writeSet.descriptorType == vk::DescriptorType::eStorageTexelBuffer);

    writeSet.pTexelBufferView = pTexelBufferView;
    return writeSet;
}

vk::WriteDescriptorSet DescriptorSetBindings::makeWrite(vk::DescriptorSet dstSet, uint32_t dstBinding, 
    const vk::WriteDescriptorSetAccelerationStructureNV* pAccel, uint32_t arrayElement = 0) const
{
    vk::WriteDescriptorSet writeSet = makeWrite(dstSet, dstBinding);
    assert(writeSet.descriptorType == vk::DescriptorType::eAccelerationStructureNV);

    writeSet.pNext = pAccel;
    return writeSet;
}

vk::WriteDescriptorSet DescriptorSetBindings::makeWrite(vk::DescriptorSet dstSet, uint32_t dstBinding, 
    const vk::WriteDescriptorSetInlineUniformBlockEXT* pInlineUniform, uint32_t arrayElement = 0) const
{
    vk::WriteDescriptorSet writeSet = makeWrite(dstSet, dstBinding);
    assert(writeSet.descriptorType == vk::DescriptorType::eInlineUniformBlockEXT);

    writeSet.pNext = pInlineUniform;
    return writeSet;
}

//-------------------------------------------------------------------------
// Write Arrays
//
vk::WriteDescriptorSet DescriptorSetBindings::makeWriteArray(vk::DescriptorSet dstSet, 
    uint32_t dstBinding) const
{
    for (size_t i = 0; i < m_bindings.size(); i++) {
        if (m_bindings[i].binding == dstBinding) 
            return { dstSet, dstBinding, 0, m_bindings[i].descriptorCount, m_bindings[i].descriptorType };
    }
    assert(0 && "binding not found");
    return {};
}

vk::WriteDescriptorSet DescriptorSetBindings::makeWriteArray(vk::DescriptorSet dstSet, 
    uint32_t dstBinding, const vk::DescriptorImageInfo* pImageInfo) const
{
    vk::WriteDescriptorSet writeSet = makeWriteArray(dstSet, dstBinding);
    assert(writeSet.descriptorType == vk::DescriptorType::eSampler ||
           writeSet.descriptorType == vk::DescriptorType::eCombinedImageSampler ||
           writeSet.descriptorType == vk::DescriptorType::eSampledImage ||
           writeSet.descriptorType == vk::DescriptorType::eStorageImage ||
           writeSet.descriptorType == vk::DescriptorType::eInputAttachment);

    writeSet.pImageInfo = pImageInfo;
    return writeSet;
}

vk::WriteDescriptorSet DescriptorSetBindings::makeWriteArray(vk::DescriptorSet dstSet, 
    uint32_t dstBinding, const vk::DescriptorBufferInfo* pBufferInfo) const
{
    vk::WriteDescriptorSet writeSet = makeWriteArray(dstSet, dstBinding);
    assert(writeSet.descriptorType == vk::DescriptorType::eStorageBuffer ||
           writeSet.descriptorType == vk::DescriptorType::eStorageBufferDynamic ||
           writeSet.descriptorType == vk::DescriptorType::eUniformBuffer ||
           writeSet.descriptorType == vk::DescriptorType::eUniformBufferDynamic);

    writeSet.pBufferInfo = pBufferInfo;
    return writeSet;
}

vk::WriteDescriptorSet DescriptorSetBindings::makeWriteArray(vk::DescriptorSet dstSet, 
    uint32_t dstBinding, const vk::BufferView* pTexelBufferView) const
{
    vk::WriteDescriptorSet writeSet = makeWriteArray(dstSet, dstBinding);
    assert(writeSet.descriptorType == vk::DescriptorType::eUniformTexelBuffer);

    writeSet.pTexelBufferView = pTexelBufferView;
    return writeSet;
}

vk::WriteDescriptorSet DescriptorSetBindings::makeWriteArray(vk::DescriptorSet dstSet, 
    uint32_t dstBinding, const vk::WriteDescriptorSetAccelerationStructureNV* pAccel) const
{
    vk::WriteDescriptorSet writeSet = makeWriteArray(dstSet, dstBinding);
    assert(writeSet.descriptorType == vk::DescriptorType::eAccelerationStructureNV);

    writeSet.pNext = pAccel;
    return writeSet;
}

vk::WriteDescriptorSet DescriptorSetBindings::makeWriteArray(vk::DescriptorSet dstSet, 
    uint32_t dstBinding, const vk::WriteDescriptorSetInlineUniformBlockEXT* pInline) const
{
    vk::WriteDescriptorSet writeSet = makeWriteArray(dstSet, dstBinding);
    assert(writeSet.descriptorType == vk::DescriptorType::eInlineUniformBlockEXT);

    writeSet.pNext = pInline;
    return writeSet;
}


} // namespace app