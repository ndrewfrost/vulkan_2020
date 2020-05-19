/*
 *
 * Andrew Frost
 * allocator.cpp
 * 2020
 *
 */

#include "allocator.h"

const uint32_t vkDeviceLocalMemoryMB = 128;
const uint32_t vkHostVisibleMemoryMB = 64;

static const char* memoryUsageStrings[VULKAN_MEMORY_USAGES] = {
    "VULKAN_MEMORY_USAGE_UNKNOWN",
    "VULKAN_MEMORY_USAGE_GPU_ONLY",
    "VULKAN_MEMORY_USAGE_CPU_ONLY",
    "VULKAN_MEMORY_USAGE_CPU_TO_GPU",
    "VULKAN_MEMORY_USAGE_GPU_TO_CPU",
};

static const char* allocationTypeStrings[VULKAN_ALLOCATION_TYPES] = {
    "VULKAN_ALLOCATION_TYPE_FREE",
    "VULKAN_ALLOCATION_TYPE_BUFFER",
    "VULKAN_ALLOCATION_TYPE_IMAGE",
    "VULKAN_ALLOCATION_TYPE_IMAGE_LINEAR",
    "VULKAN_ALLOCATION_TYPE_IMAGE_OPTIMAL",
};

/*
===========================================================================
MemoryPool
===========================================================================
*/

/*
===============
MemoryPool::MemoryPool
===============
*/
MemoryPool::MemoryPool(const uint32_t memoryTypeIndex,
                       const vk::DeviceSize size,
                       memoryUsage_t usage) {
    this->nextPoolId = 0;
    this->size = size;
    this->allocated = 0;
    this->memoryTypeIndex = memoryTypeIndex;
    this->usage = usage;
    this->deviceMemory = vk::UniqueDeviceMemory();
}

/*
===============
MemoryPool::~MemoryPool
===============
*/
MemoryPool::~MemoryPool() {
    close();
}

/*
===============
MemoryPool::init
===============
*/
bool MemoryPool::init() {
    if (memoryTypeIndex == UINT64_MAX) return false;

    vk::MemoryAllocateInfo memoryAllocateInfo(size, memoryTypeIndex);

    try {
        //deviceMemory = vkContext.device.allocateMemoryUnique(memoryAllocateInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to allocate memory!");
    }

    if (*deviceMemory == VK_NULL_HANDLE) return false;

    if (isHostVisible()) {}
        //void* data = vkContext.device.mapMemory(*deviceMemory, 0, size);

    head = new block_t();
    head->id = nextPoolId++;
    head->size = size;
    head->offset = 0;
    head->prev = nullptr;
    head->next = nullptr;
    head->type = VULKAN_ALLOCATION_TYPE_FREE;

    return true;
}

/*
===============
MemoryPool::close
===============
*/
bool MemoryPool::close() {
    if (isHostVisible())
        //vkContext.device.unmapMemory(*deviceMemory);

    //vkContext.device.freeMemory(*deviceMemory);
    deviceMemory = vk::UniqueDeviceMemory();

    block_t* prev = nullptr;
    block_t* current = head;
    
    while (1) {
        if (current->next = nullptr) {
            delete current;
            break;
        }
        else {
            prev = current;
            current = current->next;
            delete prev;
        }
    }

    head = nullptr;
}

/*
===============
MemoryPool::allocate
===============
*/
bool MemoryPool::allocate(const uint32_t size, 
                          const uint32_t align, 
                          const vk::DeviceSize granularity, 
                          const allocationType_t allocType, 
                          allocation_t& allocation) {
    return false;
}

/*
===============
MemoryPool::free
===============
*/
void MemoryPool::free(allocation_t& allocation) {
}

/*
===============
MemoryPool::print
===============
*/
void MemoryPool::print() {
    int count = 0;
    for (block_t* current = head; current != NULL; current = current->next) { count++; }

    printf("Type Index: %lu\n", memoryTypeIndex);
    printf("Usage:      %s\n", memoryUsageStrings[usage]);
    printf("Count:      %d\n", count);
    printf("Size:       %llu\n", size);
    printf("Allocated:  %llu\n", allocated);
    printf("Next Block: %lu\n", nextPoolId);
    printf("------------------------\n");

    for (block_t* current = head; current != NULL; current = current->next) {
        printf("{\n");

        printf("\tId:     %lu\n", current->id);
        printf("\tSize:   %llu\n", current->size);
        printf("\tOffset: %llu\n", current->offset);
        printf("\tType:   %s\n", allocationTypeStrings[current->type]);

        printf("}\n");
    }

    printf("\n");
}

/*
===========================================================================
Allocator
===========================================================================
*/

/*
===============
Allocator::Allocator
===============
*/
Allocator::Allocator() {

}

/*
===============
Allocator::~Allocator
===============
*/
Allocator::~Allocator() {
    close();
}

/*
===============
Allocator::init
===============
*/
void Allocator::init() {
}

/*
===============
Allocator::close
===============
*/
void Allocator::close() {
}

/*
===============
Allocator::allocate
===============
*/
allocation_t Allocator::allocate(const uint32_t size, 
                                 const uint32_t align, 
                                 const uint32_t memoryTypeBits, 
                                 const memoryUsage_t usage, 
                                 const allocationType_t allocType) {
    return allocation_t();
}

/*
===============
Allocator::free
===============
*/
void Allocator::free(const allocation_t allocation) {
}

/*
===============
Allocator::emptyGarbage
===============
*/
void Allocator::emptyGarbage() {
}

/*
===============
Allocator::print
===============
*/
void Allocator::print() {
    printf("Device Local MB: %d\n", int(deviceLocalMemoryBytes / 1024 * 1024));
    printf("Host Visible MB: %d\n", int(hostVisibleMemoryBytes / 1024 * 1024));
    printf("Buffer Granularity: %llu\n", bufferImageGranularity);
    printf("\n");

    for (int i = 0; i < VK_MAX_MEMORY_TYPES; ++i) {
        std::vector<MemoryPool* >& poolsByType = pools[i];

        const int numBlocks = poolsByType.size();
        for (int j = 0; j < numBlocks; ++j) {
            poolsByType[j]->print();
        }
    }
}
