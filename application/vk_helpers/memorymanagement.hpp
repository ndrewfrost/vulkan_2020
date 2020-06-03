/*
 *
 * Andrew Frost
 * memorymanagement.hpp
 * 2020
 *
 */

#pragma once

#include <assert.h>
#include <vector>
#include "../general_helpers/trangeallocator.hpp"
#include <vulkan/vulkan.hpp>

namespace app {

#define APP_DEFAULT_MAX_MEMORY_ALLOCATIONSIZE (VkDeviceSize(2 * 1024) * 1024 * 1024)

#define APP_DEFAULT_MEMORY_BLOCKSIZE (VkDeviceSize(128) * 1024 * 1024)

#define APP_DEFAULT_STAGING_BLOCKSIZE (VkDeviceSize(64) * 1024 * 1024)

static const uint32_t INVALID_ID_INDEX = ~0;

///////////////////////////////////////////////////////////////////////////
// Staging Memory Manager                                                //
///////////////////////////////////////////////////////////////////////////

class StagingMemoryManager
{
protected:
    //-------------------------------------------------------------------------
    // Block stores vk::Buffers taht we sub-allocate the staging space from.
    // The "index" element in the struct refers to the next free list item,
    // or itself when in use.
    //
    struct Block
    {
        uint32_t                    index = INVALID_ID_INDEX;
        vk::DeviceSize              size = 0;
        vk::Buffer                  buffer = nullptr;
        vk::DeviceMemory            memory = nullptr;
        tools::TRangeAllocator<256> range;
        uint8_t*                    mapping;
    };

    struct Entry
    {
        uint32_t block;
        uint32_t offset;
        uint32_t size;
    };

    //-------------------------------------------------------------------------
    // Staging set stores all such sub allocation ^ that were used in one 
    // batch of operations. 
    //
    struct StagingSet
    {
        uint32_t           index = INVALID_ID_INDEX;
        vk::Fence          fence = nullptr;
        std::vector<Entry> entries;
    };

public:
    //-------------------------------------------------------------------------
    // 
    //
    StagingMemoryManager(StagingMemoryManager const&) = delete;
    StagingMemoryManager& operator=(StagingMemoryManager const&) = delete;

    StagingMemoryManager() {}
    StagingMemoryManager(vk::Device device, vk::PhysicalDevice physicalDevice, vk::DeviceSize stagingBlockSize = APP_DEFAULT_STAGING_BLOCKSIZE)
    {
        init(device, physicalDevice, stagingBlockSize);
    }

    void init(vk::Device device, vk::PhysicalDevice physicalDevice, vk::DeviceSize stagingBlockSize = APP_DEFAULT_STAGING_BLOCKSIZE);
    void deinit();

    //-------------------------------------------------------------------------
    // If true, we free memory completely when release 
    //
    void setFreeUnusedOnRelease(bool state) { m_freeOnRelease = state; }

    bool fitsInAllocated(vk::DeviceSize size) const;

    void* cmdToImage(
        vk::CommandBuffer                 cmdBuffer,
        vk::Image                         image,
        const vk::Offset3D& offset,
        const vk::Extent3D& extent,
        const vk::ImageSubresourceLayers& subresource,
        vk::DeviceSize                    size,
        const void* data);

    void* cmdToBuffer(
        vk::CommandBuffer cmdBuffer,
        vk::Buffer buffer,
        vk::DeviceSize offset,
        vk::DeviceSize size,
        const void* data);

    template <class T>
    T* cmdToBufferT(vk::CommandBuffer cmd, vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize size)
    {
        return (T*)cmdToBuffer(cmd, buffer, offset, size, nullptr);
    }

    void finalizeResources(vk::Fence fence = nullptr);

    void releaseResources();

    //-------------------------------------------------------------------------
    // free staging memory no longer in use 
    //
    void freeUnused() { free(true); }

    float getUtilisation(vk::DeviceSize& allocatedSize, vk::DeviceSize& usedSize) const;

protected:

    uint32_t setIndexValue(uint32_t& index, uint32_t newValue)
    {
        uint32_t oldValue = index;
        index = newValue;
        return oldValue;
    }

    void free(bool unusedOnly);
    void freeBlock(Block& block);

    uint32_t newStagingIndex();

    void* getStagingSpace(vk::DeviceSize size, vk::Buffer& buffer, vk::DeviceSize& offset);

    Block& getBlock(uint32_t index)
    {
        Block& block = m_blocks[index];
        assert(block.index == index);
        return block;
    }

    void releaseResources(uint32_t stagingID);

    //-------------------------------------------------------------------------
    // must fill block.buffer, memory and mapping
    //
    virtual vk::Result allocBlockMemory(uint32_t id, vk::DeviceSize size, bool toDevice, Block& block) {}
    virtual void       freeBlockMemory(uint32_t id, const Block& block) {}
    virtual void       resizeBlocks(uint32_t num) {}


    vk::Device         m_device = nullptr;
    vk::PhysicalDevice m_physicalDevice = nullptr;
    uint32_t           m_memoryTypeIndex;
    vk::DeviceSize     m_stagingBlockSize;
    bool               m_freeOnRelease;

    std::vector<Block>      m_blocks;
    std::vector<StagingSet> m_sets;

    uint32_t m_stagingIndex;     // active staging Index, must be valid    
    uint32_t m_freeStagingIndex; // linked-list to next free staging set    
    uint32_t m_freeBlockIndex;   // linked list to next free block

    vk::DeviceSize m_allocatedSize;
    vk::DeviceSize m_usedSize;


}; // class StagingMemoryManager

} // namespace app