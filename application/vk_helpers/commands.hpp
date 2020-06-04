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

///////////////////////////////////////////////////////////////////////////
// SingleCommandBuffer                                                   //
///////////////////////////////////////////////////////////////////////////

namespace app {

class CommandPool
{
public:
    CommandPool(CommandPool const&) = delete;
    CommandPool& operator=(CommandPool const&) = delete;

    CommandPool() {}
    ~CommandPool() { deinit(); }

    //-------------------------------------------------------------------------
    // 
    //
    CommandPool(
        vk::Device                 device,
        uint32_t                   familyIndex,
        vk::CommandPoolCreateFlags flags = vk::CommandPoolCreateFlagBits::eTransient,
        vk::Queue                  defaultQueue = nullptr)
    {
        init(device, familyIndex, flags, defaultQueue);
    }

    //-------------------------------------------------------------------------
    //
    //
    void init(
        vk::Device                 device, 
        uint32_t                   familyIndex,
        vk::CommandPoolCreateFlags flags = vk::CommandPoolCreateFlagBits::eTransient,
        vk::Queue                  defaultQueue = nullptr)
    {
        assert(!m_device);
        m_device = device;

        vk::CommandPoolCreateInfo cmdPoolCreateInfo = {};
        cmdPoolCreateInfo.flags = flags;
        cmdPoolCreateInfo.queueFamilyIndex = familyIndex;

        try {
            m_commandPool = m_device.createCommandPool(cmdPoolCreateInfo);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to create single command buffer's command pool!");
        }

        if (defaultQueue)
            m_queue = defaultQueue;
        else
            m_queue = m_device.getQueue(familyIndex, 0);
    }

    //-------------------------------------------------------------------------
    // 
    //
    void deinit()
    {
        if (m_commandPool) {
            m_device.destroyCommandPool(m_commandPool);
            m_commandPool = nullptr;
        }
        m_device = nullptr;
    }

    //-------------------------------------------------------------------------
    // 
    //
    vk::CommandBuffer createBuffer(
        vk::CommandBufferLevel                  level = vk::CommandBufferLevel::ePrimary,
        bool                                    begin = true,
        vk::CommandBufferUsageFlags             flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        const vk::CommandBufferInheritanceInfo* pInheritance = nullptr)
    {
        vk::CommandBufferAllocateInfo allocInfo = {};
        allocInfo.commandPool        = m_commandPool;
        allocInfo.level              = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = 1;

        vk::CommandBuffer commandBuffer;
        try {
            commandBuffer = m_device.allocateCommandBuffers(allocInfo)[0];
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to allocate single command buffer!");
        }

        if (begin) {
            vk::CommandBufferBeginInfo beginInfo = {};
            beginInfo.flags            = flags;
            beginInfo.pInheritanceInfo = pInheritance;
            try {
                commandBuffer.begin(beginInfo);
            }
            catch (vk::SystemError err) {
                throw std::runtime_error("failed to begin single command buffer!");
            }
        }
        return commandBuffer;
    }

    //-------------------------------------------------------------------------
    // free cmdbuffers from this pool
    //
    void destroy(size_t count, const vk::CommandBuffer* commands)
    {
        m_device.freeCommandBuffers(m_commandPool, (uint32_t)count, commands);
    }

    void destroy(const std::vector<vk::CommandBuffer>& cmds) { destroy(cmds.size(), cmds.data()); }
    
    void destroy(vk::CommandBuffer cmd) { destroy(1, &cmd); }

    //-------------------------------------------------------------------------
    // ends and submits to queue, waits for queue idle and destroys cmds
    //
    void submitAndWait(size_t count, const vk::CommandBuffer* commands, vk::Queue queue)
    {
        for (size_t i = 0; i < count; i++) {
            commands[i].end();
        }
        vk::SubmitInfo submitInfo = {};
        submitInfo.waitSemaphoreCount = 0;
        submitInfo.pWaitSemaphores    = nullptr;
        submitInfo.pWaitDstStageMask  = nullptr;
        submitInfo.commandBufferCount = static_cast<uint32_t>(count);
        submitInfo.pCommandBuffers    = commands;

        try {
            m_queue.submit(submitInfo, nullptr);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to submit single command buffer!");
        }
        m_queue.waitIdle();
        m_device.freeCommandBuffers(m_commandPool, (uint32_t)count, commands);
    }
    
    void submitAndWait(const std::vector<vk::CommandBuffer>& cmds, vk::Queue queue)
    {
        submitAndWait(cmds.size(), cmds.data(), queue);
    }
    
    void submitAndWait(vk::CommandBuffer cmd, vk::Queue queue) { submitAndWait(1, &cmd, queue); }

    // to Default queue
    void submitAndWait(size_t count, const vk::CommandBuffer* cmds) { submitAndWait(count, cmds, m_queue); }
    
    void submitAndWait(const std::vector<vk::CommandBuffer>& cmds) { submitAndWait(cmds.size(), cmds.data(), m_queue); }
    
    void submitAndWait(vk::CommandBuffer cmd) { submitAndWait(1, &cmd, m_queue); }

    vk::CommandPool getCommandPool() const { return m_commandPool; }
private:
    vk::Device        m_device      = nullptr;
    vk::CommandPool   m_commandPool = nullptr;
    vk::Queue         m_queue       = nullptr;
};

} // namespace app