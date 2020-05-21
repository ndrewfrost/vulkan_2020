/*
 *
 * Andrew Frost
 * commands.hpp
 * 2020
 *
 */

#pragma once
#include <iostream>
#include <vulkan/vulkan.hpp>

namespace app {

///////////////////////////////////////////////////////////////////////////
// SingleCommandBuffer
///////////////////////////////////////////////////////////////////////////

class SingleCommandBuffer
{
public:
    //--------------------------------------------------------------------------------------------------
    // 
    //
    SingleCommandBuffer(const vk::Device& device, uint32_t queueFamilyIdx) : m_device(device)
    {
        m_queue = m_device.getQueue(queueFamilyIdx, 0);

        vk::CommandPoolCreateInfo cmdPoolCreateInfo = {};
        cmdPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        cmdPoolCreateInfo.queueFamilyIndex = queueFamilyIdx;
        m_commandPool = m_device.createCommandPool(cmdPoolCreateInfo);
    }

    //--------------------------------------------------------------------------------------------------
    // 
    //
    ~SingleCommandBuffer()
    {
        m_device.destroyCommandPool(m_commandPool);
    }

    //--------------------------------------------------------------------------------------------------
    // 
    //
    vk::CommandBuffer createCommandBuffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const
    {
        vk::CommandBufferAllocateInfo allocInfo = {};
        allocInfo.commandPool        = m_commandPool;
        allocInfo.level              = level;
        allocInfo.commandBufferCount = 1;
        vk::CommandBuffer commandBuffer = m_device.allocateCommandBuffers(allocInfo)[0];

        vk::CommandBufferBeginInfo beginInfo = {};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        commandBuffer.begin(beginInfo);

        return commandBuffer;
    }

    //--------------------------------------------------------------------------------------------------
    // 
    //
    void flushCommandBuffer(const vk::CommandBuffer& commandBuffer) const
    {
        commandBuffer.end();
        vk::SubmitInfo submitInfo = {};
        submitInfo.waitSemaphoreCount   = 0;
        submitInfo.pWaitSemaphores      = nullptr;
        submitInfo.setPWaitDstStageMask = nullptr;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = &commandBuffer;
        m_queue.submit(submitInfo, vk::Fence());
        m_queue.waitIdle();
        m_device.freeCommandBuffers(m_commandPool, commandBuffer);
    }

private:
    const vk::Device& m_device;
    vk::CommandPool   m_commandPool;
    vk::Queue         m_queue;
};

} // namespace app