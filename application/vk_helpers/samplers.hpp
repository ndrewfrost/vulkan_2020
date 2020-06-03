/*
 *
 * Andrew Frost
 * samplers.hpp
 * 2020
 *
 */

#pragma once

#include <vulkan/vulkan.hpp>
#include <assert.h>
#include <functional>
#include <unordered_map>
#include <vector>
#include <string.h>
#include <float.h>

namespace app {

///////////////////////////////////////////////////////////////////////////
// SamplerPool                                                           //
///////////////////////////////////////////////////////////////////////////

class SamplerPool {
public:
    SamplerPool(SamplerPool const&) = delete;
    SamplerPool& operator=(SamplerPool const&) = delete;

    SamplerPool() {}
    SamplerPool(vk::Device device) { init(device); }
    ~SamplerPool() { deinit(); }

    void init(vk::Device device) { m_device = device; }
    void deinit();

    vk::Sampler acquireSampler(const vk::SamplerCreateInfo& createInfo);

    void releaseSampler(vk::Sampler sampler);

private:
    struct SamplerState
    {
        vk::SamplerCreateInfo                createInfo;
        vk::SamplerReductionModeCreateInfo   reduction;
        vk::SamplerYcbcrConversionCreateInfo ycbr;

        SamplerState() { memset(this, 0, sizeof(SamplerState)); }

        bool operator==(const SamplerState& other) const { return memcmp(this, &other, sizeof(SamplerState)) == 0; }
    };

    struct HashFn
    {
        std::size_t operator()(const SamplerState& s) const
        {
            std::hash<uint32_t> hasher;
            const uint32_t* data = (const uint32_t*)& s;
            size_t              seed = 0;
            for (size_t i = 0; i < sizeof(SamplerState) / sizeof(uint32_t); i++)
            {
                // https://www.boost.org/doc/libs/1_35_0/doc/html/boost/hash_combine_id241013.html
                seed ^= hasher(data[i]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            return seed;
        }
    };

    struct Chain
    {
        vk::StructureType sType;
        const Chain* pNext;
    };

    struct Entry
    {
        vk::Sampler    sampler = nullptr;
        uint32_t       nextFreeIndex = ~0;
        uint32_t       refCount = 0;
        SamplerState   state;
    };

    vk::Device         m_device = nullptr;
    uint32_t           m_freeIndex = ~0;
    std::vector<Entry> m_entries;

    std::unordered_map<SamplerState, uint32_t, HashFn> m_stateMap;
    std::unordered_map<vk::Sampler, uint32_t>          m_samplerMap;

}; // class SamplerPool


} // namespace app