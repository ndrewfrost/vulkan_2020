/*
 *
 * Andrew Frost
 * memorymanagement.cpp
 * 2020
 *
 */

#include "memorymanagement.hpp"

namespace app {

#ifdef max
#undef max
#endif

///////////////////////////////////////////////////////////////////////////
// Staging Memory Manager                                                //
///////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------
// 
//
void StagingMemoryManager::init(vk::Device device, vk::PhysicalDevice physicalDevice, vk::DeviceSize stagingBlockSize)
{
    assert(!m_device);

    m_device           = device;
    m_physicalDevice   = physicalDevice;
    m_stagingBlockSize = stagingBlockSize;
    m_memoryTypeIndex  = ~0;
    m_freeOnRelease    = true;

    m_freeStagingIndex = INVALID_ID_INDEX;
    m_freeBlockIndex   = INVALID_ID_INDEX;
    m_usedSize         = 0;
    m_allocatedSize    = 0;

    m_stagingIndex = newStagingIndex();
}

//-------------------------------------------------------------------------
//
//
void StagingMemoryManager::deinit()
{
    if (!m_device)
        return;

    free(false);

    m_sets.clear();
    m_blocks.clear();
    m_device = nullptr;
}

//-------------------------------------------------------------------------
// Test if there is enough space in current allocations
//
bool StagingMemoryManager::fitsInAllocated(vk::DeviceSize size) const
{
    for (const auto& block : m_blocks) {
        if (block.buffer) {
            if (block.range.isAvailable((uint32_t)size, 16)) {
                return true;
            }
        }
    }

    return false;
}

//-------------------------------------------------------------------------
// if data != nullptr, memcpies to mapping and returns nullptr
// otherwise returns temporary mapping (valid until "Complete" functions)
//
void* StagingMemoryManager::cmdToImage(
    vk::CommandBuffer cmdBuffer, vk::Image image, const vk::Offset3D& offset,
    const vk::Extent3D& extent, const vk::ImageSubresourceLayers& subresource, 
    vk::DeviceSize size, const void* data)
{
    vk::Buffer srcBuffer;
    vk::DeviceSize srcOffset;

    void* mapping = getStagingSpace(size, srcBuffer, srcOffset);

    assert(mapping);

    if (data)
        memcpy(mapping, data, size);

    vk::BufferImageCopy copy = {};
    copy.bufferOffset         = srcOffset;
    copy.bufferRowLength      = 0;
    copy.bufferImageHeight    = 0;
    copy.imageSubresource     = subresource;
    copy.imageOffset          = offset;
    copy.imageExtent          = extent;

    cmdBuffer.copyBufferToImage(srcBuffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &copy);

    return data ? nullptr : mapping;
}

//-------------------------------------------------------------------------
// if data != nullptr, memcpies to mapping and returns nullptr
// otherwise returns temporary mapping (valid until "Complete" functions)
//
void* StagingMemoryManager::cmdToBuffer(vk::CommandBuffer cmdBuffer, vk::Buffer buffer, 
    vk::DeviceSize offset, vk::DeviceSize size, const void* data)
{
    if (!size)
        return nullptr;

    vk::Buffer     srcBuffer;
    vk::DeviceSize srcOffset;

    void* mapping = getStagingSpace(size, srcBuffer, srcOffset);

    assert(mapping);

    if (data)
        memcpy(mapping, data, size);

    vk::BufferCopy copy;
    copy.size      = size;
    copy.srcOffset = srcOffset;
    copy.dstOffset = offset;

    cmdBuffer.copyBuffer(srcBuffer, buffer, 1, &copy);

    return data ? nullptr : (void*)mapping;
}

//-------------------------------------------------------------------------
// Closes the batch of staging resources since last finalize Resources call
// and associates it with a fence for later release
//
void StagingMemoryManager::finalizeResources(vk::Fence fence)
{
    if (m_sets[m_stagingIndex].entries.empty())
        return;

    m_sets[m_stagingIndex].fence = fence;
    m_stagingIndex               = newStagingIndex();
}

//-------------------------------------------------------------------------
// Releases the staging resources whose fences have complete and
// those who had no fence at all
//
void StagingMemoryManager::releaseResources()
{
    for (auto& set : m_sets) {
        if (!set.entries.empty() && (!set.fence || m_device.getFenceStatus(set.fence) == vk::Result::eSuccess)) {
            releaseResources(set.index);
            set.fence = nullptr;
        }
    }
}

//-------------------------------------------------------------------------
//
//
void StagingMemoryManager::releaseResources(uint32_t stagingID)
{
    assert(stagingID != INVALID_ID_INDEX);
    StagingSet& set = m_sets[stagingID];
    assert(set.index == stagingID);

    // free used allocation ranges
    for (auto& entry : set.entries) {
        Block& block = getBlock(entry.block);
        block.range.subFree(entry.offset, entry.size);

        m_usedSize -= entry.size;

        if (block.range.isEmpty() && m_freeOnRelease) {
            freeBlock(block);
        }
    }

    set.entries.clear();

    // update set.index with current head of the free list
    // pop its old value
    m_freeStagingIndex = setIndexValue(set.index, m_freeStagingIndex);
}

//-------------------------------------------------------------------------
//
//
float StagingMemoryManager::getUtilisation(vk::DeviceSize& allocatedSize, vk::DeviceSize& usedSize) const
{
    allocatedSize = m_allocatedSize;
    usedSize = m_usedSize;

    return float(double(usedSize) / double(allocatedSize));
}

//-------------------------------------------------------------------------
//
//
void StagingMemoryManager::free(bool unusedOnly)
{
    for (uint32_t i = 0; i < (uint32_t)m_blocks.size(); i++) {
        Block& block = m_blocks[i];
        if (block.buffer && (block.range.isEmpty() || !unusedOnly)) {
            freeBlock(block);
        }
    }

    if (!unusedOnly) {
        m_blocks.clear();
        resizeBlocks(0);
        m_freeBlockIndex = INVALID_ID_INDEX;
    }
}

//-------------------------------------------------------------------------
//
//
void StagingMemoryManager::freeBlock(Block& block)
{
    m_allocatedSize -= block.size;
    freeBlockMemory(block.index, block);
    block.memory = nullptr;
    block.buffer = nullptr;
    block.mapping = nullptr;
    block.range.deinit();
    // update the block.index with the current head of the free list
    // pop its old value
    m_freeBlockIndex = setIndexValue(block.index, m_freeBlockIndex);
}

//-------------------------------------------------------------------------
//
//
uint32_t StagingMemoryManager::newStagingIndex()
{
    // find free slot
    if (m_freeStagingIndex != INVALID_ID_INDEX) {
        uint32_t newIndex = m_freeStagingIndex;
        // update free link-list
        m_freeStagingIndex = setIndexValue(m_sets[newIndex].index, newIndex);
        assert(m_sets[newIndex].index == newIndex);
        return m_sets[newIndex].index;
    }

    // else push to end
    uint32_t newIndex = (uint32_t)m_sets.size();

    StagingSet info = {};
    info.index = newIndex;
    m_sets.push_back(info);

    assert(m_sets[newIndex].index == newIndex);
    return newIndex;

}

//-------------------------------------------------------------------------
//
//
void* StagingMemoryManager::getStagingSpace(vk::DeviceSize size, vk::Buffer& buffer, vk::DeviceSize& offset)
{
    assert(m_sets[m_stagingIndex].index == m_stagingIndex && "illegal index, did you forget finalizeResources");

    uint32_t usedOffset;
    uint32_t usedSize;
    uint32_t usedAligned;

    uint32_t blockIndex = INVALID_ID_INDEX;

    for (uint32_t i = 0; i < (uint32_t)m_blocks.size(); i++) {
        Block& block = m_blocks[i];
        if (block.buffer && block.range.subAllocate((uint32_t)size, 16, usedOffset, usedAligned, usedSize)) {
            blockIndex = block.index;

            offset = usedAligned;
            buffer = block.buffer;
        }
    }

    if (blockIndex == INVALID_ID_INDEX) {
        if (m_freeBlockIndex != INVALID_ID_INDEX) {
            Block& block = m_blocks[m_freeBlockIndex];
            m_freeBlockIndex = setIndexValue(block.index, m_freeBlockIndex);

            blockIndex = block.index;
        }
        else {
            uint32_t newIndex = (uint32_t)m_blocks.size();
            m_blocks.resize(m_blocks.size() + 1);
            resizeBlocks((uint32_t)m_blocks.size());
            Block& block = m_blocks[newIndex];
            block.index = newIndex;

            blockIndex = newIndex;
        }

        Block& block = m_blocks[blockIndex];
        block.size = std::max(m_stagingBlockSize, size);
        block.size = block.range.alignedSize((uint32_t)block.size);

        vk::Result result = allocBlockMemory(blockIndex, block.size, true, block);
        if (result != vk::Result::eSuccess) {
            throw std::runtime_error("failed to allocate block memory");
        }

        m_allocatedSize += block.size;

        block.range.init((uint32_t)block.size);
        block.range.subAllocate((uint32_t)size, 16, usedOffset, usedAligned, usedSize);

        offset = usedAligned;
        buffer = block.buffer;
    }

    // append used space to current staging set list
    m_usedSize += usedSize;
    m_sets[m_stagingIndex].entries.push_back({ blockIndex, usedOffset, usedSize });

    return m_blocks[blockIndex].mapping + offset;
}



} // namespace app