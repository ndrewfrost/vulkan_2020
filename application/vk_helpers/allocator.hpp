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
//nclude "memorymanagement.hpp"
#include "samplers.hpp"
#include "images.hpp"

namespace app {

// Objects
struct BufferVma
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
};

struct ImageVma
{
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
};

struct TextureVma : public ImageVma
{
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    VkDescriptorImageInfo descriptor{};
};

struct AccelerationDedicated
{
    VkAccelerationStructureNV acceleration{ VK_NULL_HANDLE };
    VmaAllocation             allocation;
};

///////////////////////////////////////////////////////////////////////////
// Staging Memory Manager                                                //
///////////////////////////////////////////////////////////////////////////

class StagingMemoryManager
{
}; // StagingMemoryManager

///////////////////////////////////////////////////////////////////////////
// Allocator                                                             //
///////////////////////////////////////////////////////////////////////////
// Allocator for buffers, images and acceleration structures             //
///////////////////////////////////////////////////////////////////////////

class Allocator
{
public:
    //-------------------------------------------------------------------------
    //
    //
    Allocator(Allocator const&) = delete;
    Allocator& operator=(Allocator const&) = delete;
    Allocator() = default;

    //-------------------------------------------------------------------------
    // All staging buffers must be cleared before
    //
    void deinit()
    {
        m_samplerPool.deinit();
        m_staging.deinit();
    }

    //-------------------------------------------------------------------------
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

    //-------------------------------------------------------------------------
    // Converter utility from Vulkan memory property to VMA
    //
    VmaMemoryUsage vkToVmaMemoryUsage(vk::MemoryPropertyFlags flags) {
        if ((flags & vk::MemoryPropertyFlagBits::eDeviceLocal) == vk::MemoryPropertyFlagBits::eDeviceLocal)
            return VMA_MEMORY_USAGE_GPU_ONLY;
        else if ((flags & (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent))
            == (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent))
            return VMA_MEMORY_USAGE_CPU_ONLY;
        else if ((flags & vk::MemoryPropertyFlagBits::eHostVisible) == vk::MemoryPropertyFlagBits::eHostVisible)
            return VMA_MEMORY_USAGE_CPU_TO_GPU;

        return VMA_MEMORY_USAGE_UNKNOWN;
    }

    //-------------------------------------------------------------------------
    // Basic Buffer creation
    //
    BufferVma createBuffer(
        const VkBufferCreateInfo& info, VmaMemoryUsage memUsage = VMA_MEMORY_USAGE_GPU_ONLY)
    {
        BufferVma resultBuffer;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = memUsage;

        VkResult result = vmaCreateBuffer(m_allocator, &info, &allocInfo, &resultBuffer.buffer, &resultBuffer.allocation, nullptr);
        assert(result = VK_SUCCESS);
        return resultBuffer;
    }
    
    BufferVma createBuffer(
        const VkBufferCreateInfo& bufferInfo, const vk::MemoryPropertyFlags memProps)
    {
        return createBuffer(bufferInfo, vkToVmaMemoryUsage(memProps));
    }
    
    
    //-------------------------------------------------------------------------
    // Simple Buffer creation
    //
    BufferVma createBuffer(
        VkDeviceSize       size,
        VkBufferUsageFlags usage,
        VmaMemoryUsage     memUsage = VMA_MEMORY_USAGE_GPU_ONLY)
    {
        BufferVma resultBuffer;

        VkBufferCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.size = size;
        info.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        return createBuffer(info, memUsage);
    }

    BufferVma createBuffer(VkDeviceSize                  size, 
                           VkBufferUsageFlags            usage,
                           const vk::MemoryPropertyFlags memProps)
    {
        createBuffer(size, usage, vkToVmaMemoryUsage(memProps));
    }

    //-------------------------------------------------------------------------
    // Staging buffer creation, uploading data to device buffer
    //
    BufferVma createBuffer(const vk::CommandBuffer  cmdBuffer,
                           const VkDeviceSize       size,
                           const void*              data,
                           const VkBufferUsageFlags usage,
                           VmaMemoryUsage memUsage = VMA_MEMORY_USAGE_GPU_ONLY)
    {
        BufferVma resultBuffer = createBuffer(size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, memUsage);
        
        if (data) {
            m_staging.cmdToBuffer(cmd, resultBuffer.buffer, 0, size, data);
        }

        return resultBuffer;
    }

    BufferVma createBuffer(const vk::CommandBuffer cmdBuffer,
                           const VkDeviceSize& size,
                           const void* data,
                           const VkBufferUsageFlags usage,
                           vk::MemoryPropertyFlags memProps)
    {
        createBuffer(cmdBuffer, size, data, usage, vkToVmaMemoryUsage(memProps));
    }

    //-------------------------------------------------------------------------
    // Staging buffer creation, uploading data to device buffer
    //
    template <typename T>
    BufferVma createBuffer(const vk::CommandBuffer  cmdBuffer,
                           const std::vector<T>& data,
                           const VkBufferUsageFlags usage,
                           VmaMemoryUsage memUsage = VMA_MEMORY_USAGE_GPU_ONLY)
    {
        VkDeviceSize size = sizeof(T) * data.size();
        BufferVma resultBuffer = createBuffer(size, usage, memUsage);
        if (!data.empty()) {
            m_staging.cmdToBuffer(cmdBuffer, resultBuffer.buffer, 0, size, data.data());
        }

        return resultBuffer;
    }

    template <typename T>
    BufferVma createBuffer(const vk::CommandBuffer  cmdBuffer,
                           const std::vector<T>&    data,
                           const VkBufferUsageFlags usage,
                           vk::MemoryPropertyFlags  memProps)
    {
        return createBuffer(cmdBuffer, data, usage, vkToVmaMemoryUsage(memProps));
    }

    //-------------------------------------------------------------------------
    // Create Image
    //
    ImageVma createImage(
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

    //-------------------------------------------------------------------------
    // Create Image with data
    //
    ImageVma createImage(
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

    //-------------------------------------------------------------------------
    // Create the acceleration structure
    //
    AccelerationDedicated createAcceleration(vk::AccelerationStructureCreateInfoNV& accel)
    {
        AccelerationDedicated result;
        return result;
    }

    //-------------------------------------------------------------------------
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

    //-------------------------------------------------------------------------
    // Destroy
    //
    void destroy(BufferVma& buffer)
    {
        vmaDestroyBuffer(m_allocator, buffer.buffer, buffer.allocation);
    }

    void destroy(ImageVma& image)
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

    //-------------------------------------------------------------------------
    // get Allocator
    //
    VmaAllocator& getAllocator() { return m_allocator; }

protected:

    //-------------------------------------------------------------------------
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

    //-------------------------------------------------------------------------
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
    
    vk::Device                         m_device;
    vk::PhysicalDevice                 m_physicalDevice;
    vk::PhysicalDeviceMemoryProperties m_physicalMemoryProperties;
    vk::Instance                       m_instance;

    VmaAllocator                       m_allocator;

    app::StagingMemoryManager          m_staging;
    app::SamplerPool                   m_samplerPool;

}; // class Allocator

} // namespace app