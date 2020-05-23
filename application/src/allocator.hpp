/*
 *
 * Andrew Frost
 * allocator.hpp
 * 2020
 *
 */

#pragma once

#define VMA_IMPLEMENTATION
#include "..//external/vk_mem_alloc.h"
#include "vulkan/vulkan.hpp"


namespace app {

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
    vk::AccelerationStructureNV accel;
    vk::DeviceMemory            allocation;
};

///////////////////////////////////////////////////////////////////////////
// Allocator
///////////////////////////////////////////////////////////////////////////

class Allocator
{
public:
    //--------------------------------------------------------------------------------------------------
    // All staging buffers must be cleared before TODO
    //
    ~Allocator() { }

    //--------------------------------------------------------------------------------------------------
    // Initialization of the allocator
    //
    void init(vk::Device device, vk::PhysicalDevice physicalDevice, vk::Instance instance)
    {
        m_device         = device;
        m_physicalDevice = physicalDevice;
        m_instance       = instance;

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device         = device;
        allocatorInfo.instance       = instance;
        vmaCreateAllocator(&allocatorInfo, &m_allocator);
    }

    //--------------------------------------------------------------------------------------------------
    // Basic Image creation
    //
    ImageDedicated createImage(const VkImageCreateInfo& info,
                               const vk::MemoryPropertyFlags memUsage_ = vk::MemoryPropertyFlagBits::eDeviceLocal)
    {
        ImageDedicated resultImage;
        
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        vmaCreateImage(m_allocator, &info, &allocInfo, &resultImage.image, &resultImage.allocation, nullptr);

        return resultImage;
    }

    //--------------------------------------------------------------------------------------------------
    // Staging buffer creation, uploading data to device buffer
    //
    template <typename T>
    BufferDedicated createBuffer(const std::vector<T>& data_,
                                 const VkBufferUsageFlags& usage_ = VkBufferUsageFlags())
    {
        return createBuffer(sizeof(T) * data_.size(), data_.data(), usage_);
    }

    //--------------------------------------------------------------------------------------------------
    // 
    //
    BufferDedicated createBuffer(const vk::DeviceSize& size_ = 0,
                                 const void* data_ = nullptr,
                                 const VkBufferUsageFlags & usage_ = VkBufferUsageFlags())
    {
        BufferDedicated resultBuffer;

        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = size_;
        bufferInfo.usage = usage_ | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

        vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &resultBuffer.buffer, &resultBuffer.allocation, nullptr);

        return resultBuffer;
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
    void destroy(BufferDedicated& b_)
    {
        vmaDestroyBuffer(m_allocator, b_.buffer, b_.allocation);
    }

    void destroy(ImageDedicated& i_)
    {
        vmaDestroyImage(m_allocator, i_.image, i_.allocation);
    }

    void destroy(AccelerationDedicated& a_)
    {
        m_device.destroyAccelerationStructureNV(a_.accel);
        m_device.freeMemory(a_.allocation);
    }

    void destroy(TextureDedicated& t_)
    {
        m_device.destroyImageView(t_.descriptor.imageView);
        m_device.destroySampler(t_.descriptor.sampler);
        vmaDestroyImage(m_allocator, t_.image, t_.allocation);
    }

protected:
    vk::Device                         m_device;
    vk::PhysicalDevice                 m_physicalDevice;
    vk::Instance                       m_instance;
    VmaAllocator                       m_allocator;

}; // class Allocator

} // namespace app