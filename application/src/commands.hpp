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
        cmdPoolCreateInfo.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        cmdPoolCreateInfo.queueFamilyIndex = queueFamilyIdx;
        
        try {
            m_commandPool = m_device.createCommandPool(cmdPoolCreateInfo);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to create single command buffer's command pool!");
        }
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

        vk::CommandBuffer commandBuffer;
        try {
            commandBuffer = m_device.allocateCommandBuffers(allocInfo)[0];
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to allocate single command buffer!");
        }

        vk::CommandBufferBeginInfo beginInfo = {};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        try {
            commandBuffer.begin(beginInfo);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to begin single command buffer!");
        }

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
        submitInfo.pWaitDstStageMask    = nullptr;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = &commandBuffer;
        
        try {
            m_queue.submit(submitInfo, vk::Fence());
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to submit single command buffer!");
        }
        m_queue.waitIdle();

        m_device.freeCommandBuffers(m_commandPool, commandBuffer);
    }

private:
    const vk::Device& m_device;
    vk::CommandPool   m_commandPool;
    vk::Queue         m_queue;
};

} // namespace app