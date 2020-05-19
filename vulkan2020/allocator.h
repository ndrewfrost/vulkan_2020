/*
 *
 * Andrew Frost
 * allocator.h
 * 2020
 *
 */

#pragma once
#include "header.h"
#include "renderbackend.h"

#ifndef __HEAP_VK_H__
#define __HEAP_VK_H__

enum memoryUsage_t {
    VULKAN_MEMORY_USAGE_UNKNOWN,
    VULKAN_MEMORY_USAGE_GPU_ONLY,
    VULKAN_MEMORY_USAGE_CPU_ONLY,
    VULKAN_MEMORY_USAGE_CPU_TO_GPU,
    VULKAN_MEMORY_USAGE_GPU_TO_CPU,
    VULKAN_MEMORY_USAGES,
};

enum allocationType_t {
    VULKAN_ALLOCATION_TYPE_FREE,
    VULKAN_ALLOCATION_TYPE_BUFFER,
    VULKAN_ALLOCATION_TYPE_IMAGE,
    VULKAN_ALLOCATION_TYPE_IMAGE_LINEAR,
    VULKAN_ALLOCATION_TYPE_IMAGE_OPTIMAL,
    VULKAN_ALLOCATION_TYPES,
};

class MemoryPool;

struct allocation_t {
    allocation_t() :
        poolId(nullptr),
        blockId(0),
        deviceMemory(),
        offset(0),
        size(0),
        data(nullptr) {}

    MemoryPool*            poolId;       // which pool does this come from
    uint32_t               blockId;      // what memory block is this
    vk::UniqueDeviceMemory deviceMemory; // vk device memory associated with block
    vk::DeviceSize         offset;       // offset in data below
    vk::DeviceSize         size;         // size of memory associated with this block
    std::byte*             data;         // if data is host visible we can map it
};

class MemoryPool {
    friend class Allocator;
public:
    MemoryPool(const uint32_t memoryTypeIndex, 
               const vk::DeviceSize size, 
               memoryUsage_t usage);

    ~MemoryPool();

    bool isHostVisible() const { return usage != VULKAN_MEMORY_USAGE_GPU_ONLY; };

    bool init();
    bool close();

    bool allocate(const uint32_t size, 
                  const uint32_t align, 
                  const vk::DeviceSize granularity, 
                  const allocationType_t allocType, 
                  allocation_t& allocation);

    void free(allocation_t& allocation);

    void print();

private:
    struct block_t {
        uint32_t         id;
        VkDeviceSize     size;
        VkDeviceSize     offset;
        block_t*         prev;
        block_t*         next;
        allocationType_t type;
    };

    block_t*               head;
    uint32_t               id;
    uint32_t               nextPoolId;
    uint32_t               memoryTypeIndex;
    vk::UniqueDeviceMemory deviceMemory;
    vk::DeviceSize         size;
    vk::DeviceSize         allocated;
    memoryUsage_t          usage;
    std::byte*             data;
};

class Allocator {
public:
    Allocator();
    ~Allocator();

    void init();
    void close();

    allocation_t allocate(const uint32_t size, 
                          const uint32_t align, 
                          const uint32_t memoryTypeBits,  
                          const memoryUsage_t usage, 
                          const allocationType_t allocType);

    void free(const allocation_t allocation);
    void emptyGarbage();

    void print();

private:
    int garbageIndx;

    int deviceLocalMemoryBytes;
    int hostVisibleMemoryBytes;
    vk::DeviceSize bufferImageGranularity;

    std::vector<std::vector<MemoryPool*>> pools;
    std::vector<allocation_t>             garbage[2];
};

#endif // !__ALLOCATOR_H__
