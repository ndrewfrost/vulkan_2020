/*
 *
 * Andrew Frost
 * descriptorsets.hpp
 * 2020
 *
 */

#pragma once

#include <vulkan/vulkan.hpp>

namespace app {
namespace util {

//--------------------------------------------------------------------------------------------------
// Descriptor Set Pool
//
inline vk::DescriptorPool createDescriptorPool(vk::Device device,
                                               const std::vector<vk::DescriptorSetLayoutBinding>& bindings, 
                                               uint32_t maxSets = 1)
{
    // Aggregate the bindings to obtain the required size of the descriptors using that layout
    std::vector<vk::DescriptorPoolSize> counters;
    counters.reserve(bindings.size());

    for (const auto& b : bindings)
        counters.emplace_back(b.descriptorType, b.descriptorCount);

    // Create the pool information descriptor, contains the number of descriptors of each type
    vk::DescriptorPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.poolSizeCount = static_cast<uint32_t>(counters.size());
    poolCreateInfo.pPoolSizes    = counters.data();
    poolCreateInfo.maxSets    = maxSets;

    try {
        vk::DescriptorPool pool = device.createDescriptorPool(poolCreateInfo);
        return pool;
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

//--------------------------------------------------------------------------------------------------
// Descriptor Set Layout
//
inline vk::DescriptorSetLayout createDescriptorSetLayout(vk::Device device,
                                                         const std::vector<vk::DescriptorSetLayoutBinding>& bindings)
{
    vk::DescriptorBindingFlagsEXT bindFlag = vk::DescriptorBindingFlagBitsEXT::ePartiallyBound;

    vk::DescriptorSetLayoutBindingFlagsCreateInfoEXT extendedInfo{};
    extendedInfo.pNext = nullptr;
    extendedInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    extendedInfo.pBindingFlags = &bindFlag;

    vk::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutCreateInfo.pBindings    = bindings.data();
    //layoutCreateInfo.pNext        = &extendedInfo;

    try {
        vk::DescriptorSetLayout layout = device.createDescriptorSetLayout(layoutCreateInfo);
        return layout;
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

//--------------------------------------------------------------------------------------------------
// Descriptor Set 
//
inline vk::DescriptorSet createDescriptorSet(vk::Device device, 
                                             vk::DescriptorPool pool, 
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

//--------------------------------------------------------------------------------------------------
// Write Descriptor Sets 
//
inline vk::WriteDescriptorSet createWrite(
    vk::DescriptorSet                     ds,
    const vk::DescriptorSetLayoutBinding& binding,
    const vk::DescriptorBufferInfo*       info,
    uint32_t                              arrayElement = 0)
{
    return { ds, binding.binding, arrayElement, binding.descriptorCount, binding.descriptorType, nullptr, info };
}

inline vk::WriteDescriptorSet createWrite(
    vk::DescriptorSet                     ds,
    const vk::DescriptorSetLayoutBinding& binding,
    const vk::DescriptorImageInfo*        info,
    uint32_t                              arrayElement = 0)
{
    return { ds, binding.binding, arrayElement, binding.descriptorCount, binding.descriptorType, info };
}

inline vk::WriteDescriptorSet createWrite(
    vk::DescriptorSet                                    ds,
    const vk::DescriptorSetLayoutBinding&                binding,
    const vk::WriteDescriptorSetAccelerationStructureNV* info,
    uint32_t                                             arrayElement = 0)
{
    vk::WriteDescriptorSet res(ds, binding.binding, arrayElement, binding.descriptorCount, binding.descriptorType);
    res.setPNext(info);
    return res;
}

} // namespeace util
} // namespace app