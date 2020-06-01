/*
 *
 * Andrew Frost
 * debug.hpp
 * 2020
 *
 */

#pragma once

#include <vulkan/vulkan.hpp>

///////////////////////////////////////////////////////////////////////////
// Debug                                                                 //
///////////////////////////////////////////////////////////////////////////

namespace app {
namespace debug {

struct DebugUtil
{
    //-------------------------------------------------------------------------
    // Constructors
    //
    DebugUtil() = default;
    DebugUtil(const vk::Device& device) : m_device(device) {}

    //-------------------------------------------------------------------------
    // Setup 
    //
    void setup(const vk::Device& device, const vk::Instance& instance) 
    { 
        SetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(
                instance, "vkSetDebugUtilsObjectNameEXT");        m_device = device; 
    }

    //-------------------------------------------------------------------------
    // Set Object Name
    //
    template <typename T>
    void setObjectName(const T& object, const char* name, VkObjectType t)
    {
#ifdef _DEBUG
        VkDebugUtilsObjectNameInfoEXT info = {};        
        info.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        info.pNext        = NULL;
        info.objectType   = t;
        info.objectHandle = reinterpret_cast<const uint64_t&>(object);
        info.pObjectName  = name;
        SetDebugUtilsObjectNameEXT(m_device, &info);
#endif // _DEBUG
    }

    //-------------------------------------------------------------------------
    // Collection of set object names
    //
    void setObjectName(const vk::Buffer& object, const char* name)                  { setObjectName(object, name, VK_OBJECT_TYPE_BUFFER); }
    void setObjectName(const vk::CommandBuffer& object, const char* name)           { setObjectName(object, name, VK_OBJECT_TYPE_COMMAND_BUFFER); }
    void setObjectName(const vk::Image& object, const char* name)                   { setObjectName(object, name, VK_OBJECT_TYPE_IMAGE); }
    void setObjectName(const vk::ImageView& object, const char* name)               { setObjectName(object, name, VK_OBJECT_TYPE_IMAGE_VIEW); }
    void setObjectName(const vk::RenderPass& object, const char* name)              { setObjectName(object, name, VK_OBJECT_TYPE_RENDER_PASS); }
    void setObjectName(const vk::ShaderModule& object, const char* name)            { setObjectName(object, name, VK_OBJECT_TYPE_SHADER_MODULE); }
    void setObjectName(const vk::Pipeline& object, const char* name)                { setObjectName(object, name, VK_OBJECT_TYPE_PIPELINE); }
    void setObjectName(const vk::AccelerationStructureNV& object, const char* name) { setObjectName(object, name, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV); }
    void setObjectName(const vk::DescriptorSetLayout& object, const char* name)     { setObjectName(object, name, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT); }
    void setObjectName(const vk::DescriptorSet& object, const char* name)           { setObjectName(object, name, VK_OBJECT_TYPE_DESCRIPTOR_SET); }
    void setObjectName(const vk::Semaphore& object, const char* name)               { setObjectName(object, name, VK_OBJECT_TYPE_SEMAPHORE); }
    void setObjectName(const vk::SwapchainKHR& object, const char* name)            { setObjectName(object, name, VK_OBJECT_TYPE_SWAPCHAIN_KHR); }
    void setObjectName(const vk::Queue& object, const char* name)                   { setObjectName(object, name, VK_OBJECT_TYPE_QUEUE); }

    //-------------------------------------------------------------------------
    //
    //
    void beginLabel(const vk::CommandBuffer& cmdBuf, const char* label)
    {
#ifdef _DEBUG
        cmdBuf.beginDebugUtilsLabelEXT({ label });
#endif  // _DEBUG
    }

    //-------------------------------------------------------------------------
    //
    //
    void endLabel(const vk::CommandBuffer& cmdBuf)
    {
#ifdef _DEBUG
        cmdBuf.endDebugUtilsLabelEXT();
#endif  // _DEBUG
    }

    //-------------------------------------------------------------------------
    //
    //
    void insertLabel(const vk::CommandBuffer& cmdBuf, const char* label)
    {
#ifdef _DEBUG
        cmdBuf.insertDebugUtilsLabelEXT({ label });
#endif  // _DEBUG
    }

    //
    // Begin and End Command Label MUST be balanced, this helps as it will always close the opened label
    //
    struct ScopedCmdLabel
    {
        //-------------------------------------------------------------------------
        //
        //
        ScopedCmdLabel(const vk::CommandBuffer& cmdBuf, const char* label)
            : m_commandBuffer(cmdBuf)
        {
#ifdef _DEBUG
            cmdBuf.beginDebugUtilsLabelEXT({ label });
#endif  // _DEBUG
        }

        //-------------------------------------------------------------------------
        //
        //
        ~ScopedCmdLabel()
        {
#ifdef _DEBUG
            m_commandBuffer.endDebugUtilsLabelEXT();
#endif  // _DEBUG
        }

        //-------------------------------------------------------------------------
        //
        //
        void setLabel(const char* label)
        {
#ifdef _DEBUG
            m_commandBuffer.insertDebugUtilsLabelEXT({ label });
#endif  // _DEBUG
        }

    private:
        const vk::CommandBuffer& m_commandBuffer;
    };

    //-------------------------------------------------------------------------
    //
    //
    ScopedCmdLabel scopeLabel(const vk::CommandBuffer& cmdBuf, const char* label)
    {
        return ScopedCmdLabel(cmdBuf, label);
    }

private:
    PFN_vkSetDebugUtilsObjectNameEXT SetDebugUtilsObjectNameEXT;    vk::Device m_device;
};

} // namespace debug
} // namespace app