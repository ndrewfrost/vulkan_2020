/*
 *
 * Andrew Frost
 * samplers.cpp
 * 2020
 *
 */

#include "samplers.hpp"

namespace app {

///////////////////////////////////////////////////////////////////////////
// SamplerPool                                                           //
///////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------
// 
//
void SamplerPool::deinit()
{
    if (!m_device)
        return;

    for (auto& entry : m_entries) {
        if (entry.sampler)
            m_device.destroySampler(entry.sampler);
    }

    m_freeIndex = ~0;
    m_entries.clear();
    m_samplerMap.clear();
    m_stateMap.clear();
    m_device = nullptr;
}

//-------------------------------------------------------------------------
// creates a new sampler or re-uses an existing one with ref-count
// createInfo may contain vk::SamplerReductionModeCreateInfo and 
// vk::SamplerYcbcrConversionCreateInfo
//
vk::Sampler SamplerPool::acquireSampler(const vk::SamplerCreateInfo& createInfo)
{
    SamplerState state = {};
    state.createInfo = createInfo;

    const Chain* ext = (const Chain*)createInfo.pNext;
    while (ext) {
        switch (ext->sType) {
        case vk::StructureType::eSamplerReductionModeCreateInfo:
            state.reduction = *(const vk::SamplerReductionModeCreateInfo*)ext;
            break;
        case vk::StructureType::eSamplerYcbcrConversionCreateInfo:
            state.ycbr = *(const vk::SamplerYcbcrConversionCreateInfo*)ext;
            break;
        default:
            assert(0 && "unsupported sampler create");
        }
        ext = ext->pNext;
    }

    // remove pointers for comparison lookup
    state.createInfo.pNext = nullptr;
    state.reduction.pNext  = nullptr;
    state.ycbr.pNext       = nullptr;

    auto it = m_stateMap.find(state);
    if (it == m_stateMap.end()) {
        uint32_t index = 0;
        if (m_freeIndex != ~0) {
            index = m_freeIndex;
            m_freeIndex = m_entries[index].nextFreeIndex;
        }
        else {
            index = (uint32_t)m_entries.size();
            m_entries.resize(m_entries.size() + 1);
        }

        vk::Sampler sampler = nullptr;

        try {
            m_device.createSampler(createInfo);
        }
        catch (vk::SystemError err){
            throw std::runtime_error("failed to create Sampler!");
        }

        m_entries[index].refCount = 1;
        m_entries[index].sampler  = sampler;
        m_entries[index].state    = state;

        m_stateMap.insert({ state, index });
        m_samplerMap.insert({ sampler, index });

        return sampler;
    }
    else {
        m_entries[it->second].refCount++;
        return m_entries[it->second].sampler;
    }
}

//-------------------------------------------------------------------------
// decrements ref-count and destroys sampler if possible
//
void SamplerPool::releaseSampler(vk::Sampler sampler)
{
    auto it = m_samplerMap.find(sampler);
    assert(it != m_samplerMap.end());

    uint32_t index = it->second;
    Entry& entry = m_entries[index];

    assert(entry.sampler == sampler);
    assert(entry.refCount);

    entry.refCount--;

    if (!entry.refCount) {
        m_device.destroySampler(sampler);
        entry.sampler = nullptr;
        entry.nextFreeIndex = m_freeIndex;
        m_freeIndex = index;

        m_stateMap.erase(entry.state);
        m_samplerMap.erase(sampler);
    }
}

} // namespace app