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
    VkBuffer      buffer;
    VmaAllocation allocation;
};

struct ImageDedicated
{
    VkImage       image;
    VmaAllocation allocation;
};

struct TextureDedicated : public ImageDedicated
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
    // All staging buffers must be cleared before
    //
    ~Allocator() { assert(m_stagingBuffers.empty()); }

    //--------------------------------------------------------------------------------------------------
    // Initialization of the allocator
    //
    void init(vk::Device device, vk::PhysicalDevice physicalDevice, vk::Instance instance)
    {
        m_device = device;
        m_physicalDevice = physicalDevice;
        m_instance = instance;
        m_physicalMemoryProperties = m_physicalDevice.getMemoryProperties();

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device = device;
        allocatorInfo.instance = instance;
        vmaCreateAllocator(&allocatorInfo, &m_allocator);
    }

    //--------------------------------------------------------------------------------------------------
    // Basic Buffer creation
    //
    BufferDedicated createBuffer(
        const VkBufferCreateInfo& bufferInfo,
        const VkMemoryPropertyFlags memUsage = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    {
        BufferDedicated result;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.requiredFlags = memUsage;

        vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &result.buffer, &result.allocation, nullptr);
        
        return result;
    }

    //--------------------------------------------------------------------------------------------------
    // Simple Buffer creation
    //
    BufferDedicated createBuffer(
        VkDeviceSize                size     = 0,
        VkBufferUsageFlags          usage    = VkBufferUsageFlags(),
        const VkMemoryPropertyFlags memUsage = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    {
        VkBufferCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.size  = size;
        info.usage =  usage;
        return createBuffer(info , memUsage);
    }

    //--------------------------------------------------------------------------------------------------
    // Staging buffer creation, uploading data to device buffer
    //
    BufferDedicated createBuffer(const vk::CommandBuffer&   cmdBuffer,
                                 const VkDeviceSize&        size  = 0,
                                 const void*                data  = nullptr,
                                 const VkBufferUsageFlags & usage = VkBufferUsageFlags())
    {
        // Create Staging Buffer
        BufferDedicated stagingBuffer = createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        m_stagingBuffers.push_back(stagingBuffer);

        // Copy Data to memory
        if (data) {
            void* mappedData;
            vmaMapMemory(m_allocator, stagingBuffer.allocation, &mappedData);
            memcpy(mappedData, data, size);
            vmaUnmapMemory(m_allocator, stagingBuffer.allocation);
        }

        // Create Result buffer
        VkBufferCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.size  = size;
        info.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        BufferDedicated result = createBuffer(info);

        // Copy staging
        cmdBuffer.copyBuffer(stagingBuffer.buffer, result.buffer, vk::BufferCopy(0, 0, size));

        return result;
        
    }

    //--------------------------------------------------------------------------------------------------
    // Staging buffer creation, uploading data to device buffer
    //
    template <typename T>
    BufferDedicated createBuffer(const vk::CommandBuffer&  cmdBuffer,
                                 const std::vector<T>&     data,
                                 const VkBufferUsageFlags& usage = VkBufferUsageFlags())
    {
        return createBuffer(cmdBuffer, sizeof(T) * data.size(), data.data(), usage);
    }

    //--------------------------------------------------------------------------------------------------
    // Create Image
    //
    ImageDedicated createImage(
        const VkImageCreateInfo& imageInfo,
        const VkMemoryPropertyFlags memUsage = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    {
        ImageDedicated  result;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.requiredFlags = memUsage;

        vmaCreateImage(m_allocator, &imageInfo, &allocInfo, &result.image, &result.allocation, nullptr);

        return result;
    }

    //--------------------------------------------------------------------------------------------------
    // Create Image with data
    //
    ImageDedicated createImage(
        const vk::CommandBuffer& cmdBuffer,
        size_t                   size,
        const void*              data,
        const VkImageCreateInfo& info,
        const vk::ImageLayout&   layout = vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        ImageDedicated result = createImage(info);

        // Copy the data to staging buffer than to image
        if (data != nullptr) {
            BufferDedicated stageBuffer = createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            m_stagingBuffers.push_back(stageBuffer);

            // copy data to buffer
            void* mappedData;
            vmaMapMemory(m_allocator, stageBuffer.allocation, &mappedData);
            memcpy(mappedData, data, size);
            vmaUnmapMemory(m_allocator, stageBuffer.allocation);

            // copy buffer to image
            vk::ImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
            subresourceRange.baseMipLevel   = 0;
            subresourceRange.levelCount     = info.mipLevels;
            subresourceRange.baseArrayLayer = 0;
            subresourceRange.layerCount     = 1;

            app::image::setImageLayout(cmdBuffer, result.image, vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal, subresourceRange);

            vk::BufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageExtent                 = info.extent;

            cmdBuffer.copyBufferToImage(stageBuffer.buffer, result.image, 
                                    vk::ImageLayout::eTransferDstOptimal, bufferCopyRegion);

            // Setting final image Layout
            subresourceRange.levelCount = 1;
            app::image::setImageLayout(cmdBuffer, result.image, vk::ImageLayout::eTransferDstOptimal,
                layout, subresourceRange);

        }
        else {
            app::image::setImageLayout(cmdBuffer, result.image, vk::ImageLayout::eUndefined, layout );
        }

        return result;
    }

    //--------------------------------------------------------------------------------------------------
    // Create the acceleration structure
    //
    AccelerationDedicated createAcceleration(vk::AccelerationStructureCreateInfoNV& accel)
    {
        AccelerationDedicated result;
        return result;
    }

    //--------------------------------------------------------------------------------------------------
    // Flushing staging buffers, must be done after the command buffer is submitted
    //
    void flushStaging(vk::Fence fence = vk::Fence())
    {
        if(!m_stagingBuffers.empty()) {
            m_garbageBuffers.push_back({ fence, m_stagingBuffers });
            m_stagingBuffers.clear();
        }
        cleanGarbage();
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

    void destroy(TextureDedicated& texture)
    {
        m_device.destroyImageView(texture.descriptor.imageView);
        m_device.destroySampler(texture.descriptor.sampler);
        vmaDestroyImage(m_allocator, texture.image, texture.allocation);
    }

    void destroy(AccelerationDedicated& acceleration)
    {

    }

    //--------------------------------------------------------------------------------------------------
    // get Allocator
    //
    VmaAllocator& getAllocator() { return m_allocator; }

protected:

    //--------------------------------------------------------------------------------------------------
    // Find memory type for memory alloc
    //
    uint32_t getMemoryType(uint32_t memoryTypeBits, const vk::MemoryPropertyFlags& properties)
    {    
        for (uint32_t i = 0; i < m_physicalMemoryProperties.memoryTypeCount; ++i) {
            if (((memoryTypeBits & (1 << i)) > 0) 
                && (m_physicalMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        return UINT32_MAX;
    }

    //--------------------------------------------------------------------------------------------------
    // Clean All Staging buffers, only if associated fence is set to ready
    //
    void cleanGarbage()
    {
        auto garBuffer = m_garbageBuffers.begin();
        while (garBuffer != m_garbageBuffers.end()) {
            vk::Result result = vk::Result::eSuccess;

            if (garBuffer->fence) // fence could not be set
                result = m_device.getFenceStatus(garBuffer->fence);

            if (result == vk::Result::eSuccess) {
                for (auto & staging : garBuffer ->stagingBuffers) {
                    // Delete all buffers and free memory
                    vmaDestroyBuffer(m_allocator, staging.buffer, staging.allocation);
                }
                garBuffer = m_garbageBuffers.erase(garBuffer);
            }
            else {
                ++garBuffer;
            }
        }
    }

    struct GarbageCollection
    {
        vk::Fence                    fence;
        std::vector<BufferDedicated> stagingBuffers;
    };
    std::vector<GarbageCollection>     m_garbageBuffers;

    std::vector<BufferDedicated>       m_stagingBuffers;

    vk::Device                         m_device;
    vk::PhysicalDevice                 m_physicalDevice;
    vk::PhysicalDeviceMemoryProperties m_physicalMemoryProperties;
    vk::Instance                       m_instance;
    VmaAllocator                       m_allocator;

}; // class Allocator

} // namespace app