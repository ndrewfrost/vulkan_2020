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
#include "memorymanagement.hpp"
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
// Staging Memory Manager  VMA                                           //
///////////////////////////////////////////////////////////////////////////

class StagingMemoryManagerVma : public StagingMemoryManager
{
public:
    //-------------------------------------------------------------------------
    // 
    //
    StagingMemoryManagerVma(vk::Device device, vk::PhysicalDevice physicalDevice, VmaAllocator memAllocator, vk::DeviceSize stagingBlockSize = APP_DEFAULT_STAGING_BLOCKSIZE)
    {
        init(device, physicalDevice, memAllocator, stagingBlockSize);
    }
    StagingMemoryManagerVma() {};

    //-------------------------------------------------------------------------
    // Initialization
    //
    void init(vk::Device device, vk::PhysicalDevice physicalDevice, VmaAllocator memAllocator, vk::DeviceSize stagingBlockSize = APP_DEFAULT_STAGING_BLOCKSIZE)
    {
        StagingMemoryManager::init(device, physicalDevice, stagingBlockSize);
        m_allocator = memAllocator;
    }

protected:
    
    //-------------------------------------------------------------------------
    // 
    //
    vk::Result allocBlockMemory(uint32_t index, vk::DeviceSize size, bool toDevice, Block& block) override
    {
        VkBufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.usage = toDevice ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        createInfo.size  = size;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = toDevice ? VMA_MEMORY_USAGE_CPU_TO_GPU : VMA_MEMORY_USAGE_GPU_TO_CPU;

        VkResult result = vmaCreateBuffer(m_allocator, &createInfo, &allocInfo, reinterpret_cast<VkBuffer*>(&block.buffer), &m_blockAllocations[index], nullptr);
        if (result != VK_SUCCESS)
            return vk::Result(result);

        result = vmaMapMemory(m_allocator, m_blockAllocations[index], (void**)& block.mapping);
        return vk::Result(result);
    }

    //-------------------------------------------------------------------------
    // 
    //
    void freeBlockMemory(uint32_t index, const Block& block) override
    {
        m_device.destroyBuffer(block.buffer);
        vmaUnmapMemory(m_allocator, m_blockAllocations[index]);
        vmaFreeMemory(m_allocator, m_blockAllocations[index]);
    }

    //-------------------------------------------------------------------------
    // 
    //
    void resizeBlocks(uint32_t num) override
    {
        if (num)
            m_blockAllocations.resize(num);
        else {
            m_blockAllocations.clear();
        }
    }

    VmaAllocator               m_allocator;
    std::vector<VmaAllocation> m_blockAllocations;

}; // StagingMemoryManagerVma

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

    BufferVma createBuffer(vk::DeviceSize            size, 
                           vk::BufferUsageFlags      usage,
                           vk::MemoryPropertyFlags memProps)
    {
        createBuffer(static_cast<VkDeviceSize>(size), static_cast<VkBufferUsageFlags>(usage), vkToVmaMemoryUsage(memProps));
    }

    //-------------------------------------------------------------------------
    // Staging buffer creation, uploading data to device buffer
    //
    BufferVma createBuffer(vk::CommandBuffer  cmdBuffer,
                           VkDeviceSize       size,
                           const void*        data,
                           VkBufferUsageFlags usage,
                           VmaMemoryUsage memUsage = VMA_MEMORY_USAGE_GPU_ONLY)
    {
        BufferVma resultBuffer = createBuffer(size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, memUsage);
        
        if (data) {
            m_staging.cmdToBuffer(cmdBuffer, resultBuffer.buffer, 0, size, data);
        }

        return resultBuffer;
    }

    BufferVma createBuffer(vk::CommandBuffer  cmdBuffer,
                           VkDeviceSize       size,
                           const void*        data,
                           VkBufferUsageFlags usage,
                           vk::MemoryPropertyFlags memProps)
    {
        createBuffer(cmdBuffer, size, data, usage, vkToVmaMemoryUsage(memProps));
    }

    //-------------------------------------------------------------------------
    // Staging buffer creation, uploading data to device buffer
    //
    template <typename T>
    BufferVma createBuffer(vk::CommandBuffer     cmdBuffer,
                           const std::vector<T>& data,
                           VkBufferUsageFlags    usage,
                           VmaMemoryUsage        memUsage = VMA_MEMORY_USAGE_GPU_ONLY)
    {
        VkDeviceSize size = sizeof(T) * data.size();
        BufferVma resultBuffer = createBuffer(size, usage, memUsage);
        if (!data.empty()) {
            m_staging.cmdToBuffer(cmdBuffer, resultBuffer.buffer, 0, size, data.data());
        }

        return resultBuffer;
    }

    template <typename T>
    BufferVma createBuffer(vk::CommandBuffer       cmdBuffer,
                           const std::vector<T>&   data,
                           VkBufferUsageFlags      usage,
                           vk::MemoryPropertyFlags memProps)
    {
        return createBuffer(cmdBuffer, data, usage, vkToVmaMemoryUsage(memProps));
    }

    //-------------------------------------------------------------------------
    // Create Image
    //
    ImageVma createImage(
        const VkImageCreateInfo& imageInfo,
        const VmaMemoryUsage     memUsage = VMA_MEMORY_USAGE_GPU_ONLY)
    {
        ImageVma imageResult;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = memUsage;

        VkResult result = vmaCreateImage(m_allocator, &imageInfo, &allocInfo, &imageResult.image, &imageResult.allocation, nullptr);
        assert(result == VK_SUCCESS);

        return imageResult;
    }

    //-------------------------------------------------------------------------
    // Create Image with data
    //
    ImageVma createImage(
        const vk::CommandBuffer  cmdBuffer,
        vk::DeviceSize           size,
        const void*              data,
        const VkImageCreateInfo& info,
        VkImageLayout            layout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VmaMemoryUsage           memUsage = VMA_MEMORY_USAGE_GPU_ONLY)
    {
        ImageVma imageResult = createImage(info);

        // Copy the data to staging buffer than to image
        if (data != nullptr) {
            // copy buffer to image
            vk::ImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
            subresourceRange.baseMipLevel   = 0;
            subresourceRange.levelCount     = info.mipLevels;
            subresourceRange.baseArrayLayer = 0;
            subresourceRange.layerCount     = 1;

            // doing these transitions per copy is not efficient, should do in bulk for many images
            app::image::cmdBarrierImageLayout(cmdBuffer, imageResult.image, vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal, subresourceRange);

            VkOffset3D offset = { 0 };
            VkImageSubresourceLayers subresource = { 0 };
            subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresource.layerCount = 1;

            m_staging.cmdToImage(cmdBuffer, imageResult.image, offset, info.extent, subresource, size, data);

            // Setting final image Layout
            app::image::cmdBarrierImageLayout(vk::CommandBuffer(cmdBuffer), vk::Image(imageResult.image), vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout(layout));
        }
        else {
            // Setting final image layout
            app::image::cmdBarrierImageLayout(vk::CommandBuffer(cmdBuffer), vk::Image(imageResult.image), vk::ImageLayout::eUndefined, vk::ImageLayout(layout));
        }

        return imageResult;
    }

    //-------------------------------------------------------------------------
    // Create Textures
    // 
    TextureVma createTexture(
        const ImageVma& image,
        const VkImageViewCreateInfo& imageViewCreateInfo)
    {
        TextureVma textureResult;
        textureResult.image      = image.image;
        textureResult.allocation = image.allocation;
        textureResult.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        assert(imageViewCreateInfo.image == image.image);

        try {
            textureResult.descriptor.imageView = m_device.createImageView(imageViewCreateInfo);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to create texture imageView!");
        }
        return textureResult;
    }

    TextureVma createTexture(
        const ImageVma& image,
        const VkImageViewCreateInfo& imageViewCreateInfo,
        const VkSamplerCreateInfo& samplerCreateInfo)
    {
        TextureVma resultTexture = createTexture(image, imageViewCreateInfo);
        resultTexture.descriptor.sampler = m_samplerPool.acquireSampler(samplerCreateInfo);

        return resultTexture;
    }

    //-------------------------------------------------------------------------
    // creates image for the texture
    // - creates image
    // - creates texture part associating image and sampler
    // 
    TextureVma createTexture(
        const vk::CommandBuffer&     cmdBuffer,
        size_t                       size,
        const void*                  data,
        const vk::ImageCreateInfo&   info,
        const vk::SamplerCreateInfo& samplerCreateInfo,
        const vk::ImageLayout&       layout = vk::ImageLayout::eShaderReadOnlyOptimal,
        bool                         isCube = false)
    {
        ImageVma image = createImage(static_cast<VkCommandBuffer>(cmdBuffer), size, data, info, static_cast<VkImageLayout>(layout));
    
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                           = image.image;
        viewInfo.format                          = static_cast<VkFormat>(info.format);
        viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
        viewInfo.pNext                           = nullptr;
        
        switch (info.imageType) {
        case vk::ImageType::e1D:
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_1D;
            break;
        case vk::ImageType::e2D:
            viewInfo.viewType = isCube ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
            break;
        case vk::ImageType::e3D:
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
            break;
        default:
            assert(0);
        }

        TextureVma resultTexture = createTexture(image, viewInfo, samplerCreateInfo);
        resultTexture.descriptor.imageLayout = static_cast<VkImageLayout>(layout);
        return resultTexture;
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
    // implicit staging operations triggered by creates
    //
    void finalizeStaging(VkFence fence = VK_NULL_HANDLE) { m_staging.finalizeResources(fence); }
    void finalizeAndReleaseStaging(VkFence fence = VK_NULL_HANDLE)
    {
        m_staging.finalizeResources(fence);
        m_staging.releaseResources();
    }
    void releaseStaging() { m_staging.releaseResources(); }

    StagingMemoryManager* getStaging() { return &m_staging; }
    const StagingMemoryManager* getStaging() const { return &m_staging; }

    //-------------------------------------------------------------------------
    // Destroy
    //
    void destroy(BufferVma& buffer)
    {
        if (buffer.buffer)
            vkDestroyBuffer(static_cast<VkDevice>(m_device), buffer.buffer, nullptr);
        if (buffer.allocation)
            vmaFreeMemory(m_allocator, buffer.allocation);      

        buffer = BufferVma();
    }

    void destroy(ImageVma& image)
    {
        if (image.image)
            vkDestroyImage(static_cast<VkDevice>(m_device), image.image, nullptr);
        if (image.allocation)
            vmaFreeMemory(m_allocator, image.allocation);

        image = ImageVma();
    }

    void destroy(TextureVma& texture)
    {
        vkDestroyImageView(static_cast<VkDevice>(m_device), texture.descriptor.imageView, nullptr);
        vkDestroyImage(static_cast<VkDevice>(m_device), texture.image, nullptr);

        if (texture.descriptor.sampler)
            m_samplerPool.releaseSampler(texture.descriptor.sampler);
        if (texture.allocation)
            vmaFreeMemory(m_allocator, texture.allocation);

        texture = TextureVma();
    }

    void destroy(AccelerationDedicated& acceleration)
    {

    }

    //-------------------------------------------------------------------------
    // get Allocator
    //
    VmaAllocator& getAllocator() { return m_allocator; }

    //-------------------------------------------------------------------------
    // Other
    //
    void* map(const BufferVma& buffer)
    {
        void* mapped;
        vmaMapMemory(m_allocator, buffer.allocation, &mapped);
        return mapped;
    }

    void unmap(const BufferVma& buffer) { vmaUnmapMemory(m_allocator, buffer.allocation); }

protected:    
    vk::Device                         m_device;
    VmaAllocator                       m_allocator;
    app::StagingMemoryManagerVma       m_staging;
    app::SamplerPool                   m_samplerPool;

}; // class Allocator

} // namespace app