/*
 *
 * Andrew Frost
 * debugutil.hpp
 * 2020
 *
 */

#pragma once

#include <vulkan/vulkan.hpp>

///////////////////////////////////////////////////////////////////////////
// Debug Utility
///////////////////////////////////////////////////////////////////////////

namespace app {

struct DebugUtil
{
    DebugUtil() = default;
    DebugUtil(const vk::Device& device) : m_device(device) {}

    void setup(const vk::Device& device) { m_device = device; }

    template <typename T>
    void setObjectName(const T& object, const char* name, vk::ObjectType t)
    {
#ifdef _DEBUG
        m_device.setDebugUtilsObjectNameEXT({t, reinterpret_cast<const uint64_t&>(object), name});
#endif // _DEBUG
    }

    void setObjectName(const vk::Buffer& object, const char* name) { setObjectName(object, name, vk::ObjectType::eBuffer); }
    void setObjectName(const vk::CommandBuffer& object, const char* name) { setObjectName(object, name, vk::ObjectType::eCommandBuffer); }
    void setObjectName(const vk::Image& object, const char* name) { setObjectName(object, name, vk::ObjectType::eImage); }
    void setObjectName(const vk::ImageView& object, const char* name) { setObjectName(object, name, vk::ObjectType::eImageView); }
    void setObjectName(const vk::RenderPass& object, const char* name) { setObjectName(object, name, vk::ObjectType::eRenderPass); }
    void setObjectName(const vk::ShaderModule& object, const char* name) { setObjectName(object, name, vk::ObjectType::eShaderModule); }
    void setObjectName(const vk::Pipeline& object, const char* name) { setObjectName(object, name, vk::ObjectType::ePipeline); }
    void setObjectName(const vk::AccelerationStructureNV& object, const char* name) { setObjectName(object, name, vk::ObjectType::eAccelerationStructureNV); }
    void setObjectName(const vk::DescriptorSetLayout& object, const char* name) { setObjectName(object, name, vk::ObjectType::eDescriptorSetLayout); }
    void setObjectName(const vk::DescriptorSet& object, const char* name) { setObjectName(object, name, vk::ObjectType::eDescriptorSet); }
    void setObjectName(const vk::Semaphore& object, const char* name) { setObjectName(object, name, vk::ObjectType::eSemaphore); }
    void setObjectName(const vk::SwapchainKHR& object, const char* name) { setObjectName(object, name, vk::ObjectType::eSwapchainKHR); }
    void setObjectName(const vk::Queue& object, const char* name) { setObjectName(object, name, vk::ObjectType::eQueue); }

    //---------------------------------------------------------------------------
    //
    //
    void beginLabel(const vk::CommandBuffer& cmdBuf, const char* label)
    {
#ifdef _DEBUG
        cmdBuf.beginDebugUtilsLabelEXT({ label });
#endif  // _DEBUG
    }

    //---------------------------------------------------------------------------
    //
    //
    void endLabel(const vk::CommandBuffer& cmdBuf)
    {
#ifdef _DEBUG
        cmdBuf.endDebugUtilsLabelEXT();
#endif  // _DEBUG
    }

    //---------------------------------------------------------------------------
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
        //---------------------------------------------------------------------------
        //
        //
        ScopedCmdLabel(const vk::CommandBuffer& cmdBuf, const char* label)
            : m_commandBuffer(cmdBuf)
        {
#ifdef _DEBUG
            cmdBuf.beginDebugUtilsLabelEXT({ label });
#endif  // _DEBUG
        }

        //---------------------------------------------------------------------------
        //
        //
        ~ScopedCmdLabel()
        {
#ifdef _DEBUG
            m_commandBuffer.endDebugUtilsLabelEXT();
#endif  // _DEBUG
        }

        //---------------------------------------------------------------------------
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

    //---------------------------------------------------------------------------
    //
    //
    ScopedCmdLabel scopeLabel(const vk::CommandBuffer& cmdBuf, const char* label)
    {
        return ScopedCmdLabel(cmdBuf, label);
    }

private:
    vk::Device m_device;
};

} // namespace app