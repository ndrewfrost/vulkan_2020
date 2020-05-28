/*
 *
 * Andrew Frost
 * allocator.hpp
 * 2020
 *
 */

#pragma once

#include "..//external/vk_mem_alloc.h"
#include "vulkan/vulkan.hpp"
#include "images.hpp"

namespace app {

// Objects

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

struct TextureDedicated : public ImageDedicated
{
    VkDescriptorImageInfo descriptor;

    TextureDedicated& operator=(const ImageDedicated& buffer)
    {
        static_cast<ImageDedicated&>(*this) = buffer;
        return *this;
    }
};

struct AccelerationDedicated
{
    VkAccelerationStructureNV   acceleration;
    VmaAllocation               allocation;
};

///////////////////////////////////////////////////////////////////////////
// Allocator
///////////////////////////////////////////////////////////////////////////
// Allocator for buffers, images and acceleration structures
///////////////////////////////////////////////////////////////////////////

class Allocator
{
public:
    //--------------------------------------------------------------------------------------------------
    // All staging buffers must be cleared before TODO
    //
    ~Allocator()
    {
    }

    //--------------------------------------------------------------------------------------------------
    // Initialization of the allocator
    //
    void init(vk::Device device, vk::PhysicalDevice physicalDevice, vk::Instance instance)
    {
        m_device = device;
        m_physicalDevice = physicalDevice;
        m_instance = instance;

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device = device;
        allocatorInfo.instance = instance;
        vmaCreateAllocator(&allocatorInfo, &m_allocator);
    }

    //--------------------------------------------------------------------------------------------------
    // Basic Buffer creation
    //
    BufferDedicated createBuffer()
    {

    }

    //--------------------------------------------------------------------------------------------------
    // Basic Image creation
    //
    ImageDedicated createImage(
        const VkImageCreateInfo& info,
        const vk::MemoryPropertyFlags memUsage_ = vk::MemoryPropertyFlagBits::eDeviceLocal)
    {
        ImageDedicated resultImage;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        vmaCreateImage(m_allocator, &info, &allocInfo, &resultImage.image, &resultImage.allocation, nullptr);

        return resultImage;
    }

    //--------------------------------------------------------------------------------------------------
    // Flushing staging buffers, must be done after the command buffer is submitted
    //
    void flushStaging(vk::Fence fence = vk::Fence())
    {

    }

    //--------------------------------------------------------------------------------------------------
    // Destroy
    //
    void destroy(BufferDedicated& buffer)
    {
        vmaDestroyBuffer(m_allocator, buffer.buffer, buffer.allocation);
    }

    void destroy(ImageDedicated& image)
    {
        vmaDestroyImage(m_allocator, image.image, image.allocation);
    }

    void destroy(AccelerationDedicated& acceleration)
    {
        
    }

    void destroy(TextureDedicated& texture)
    {
        m_device.destroyImageView(texture.descriptor.imageView);
        m_device.destroySampler(texture.descriptor.sampler);
        vmaDestroyImage(m_allocator, texture.image, texture.allocation);
    }

protected:
    vk::Device                         m_device;
    vk::PhysicalDevice                 m_physicalDevice;
    vk::Instance                       m_instance;
    VmaAllocator                       m_allocator;

}; // class Allocator

} // namespace app