/*
 *
 * Andrew Frost
 * vulkanbase.hpp
 * 2020
 *
 */

#pragma once
#include <set>
#include <vulkan/vulkan.hpp>

#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

#include "context.hpp"
#include "swapchain.hpp"

namespace app {

///////////////////////////////////////////////////////////////////////////
// VulkanBase
///////////////////////////////////////////////////////////////////////////

class VulkanBase
{
public:
    VulkanBase()          = default;
    virtual ~VulkanBase() = default;

    //--------------------------------------------------------------------------------------------------
    // 
    //
    void setupVulkan(app::ContextCreateInfo deviceInfo, GLFWwindow * window) 
    {
        // Create vulkan instance
        m_context.initInstance(deviceInfo);
        m_instance = m_context.m_instance;
                
        // Create Window Surface
        {
            VkSurfaceKHR rawSurface;
            if (glfwCreateWindowSurface(m_context.m_instance, window, nullptr, &rawSurface) != VK_SUCCESS) {
                throw std::runtime_error("failed to create window surface!");
            }
            m_surface = vk::SurfaceKHR(rawSurface);
        }

        // find sutable physical device and create vk::Device
        m_context.pickPhysicalDevice(deviceInfo, m_surface);
        m_context.initDevice(deviceInfo);

        setup(m_context.m_device, m_context.m_physicalDevice, m_context.m_queueGraphics.familyIndex, m_context.m_queuePresent.familyIndex);
    }

    //--------------------------------------------------------------------------------------------------
    // Setup low level vulkan
    //
    void setup(const vk::Device & device,
               const vk::PhysicalDevice & physicalDevice,
               uint32_t graphicsQueueIndex,
               uint32_t presentQueueIndex) 
    {
        m_device           = device;
        m_physicalDevice   = physicalDevice;
        m_graphicsQueueIdx = graphicsQueueIndex;
        m_presentQueueIdx  = presentQueueIndex;
        m_graphicsQueue    = m_device.getQueue(m_graphicsQueueIdx, 0);
        m_presentQueue     = m_device.getQueue(m_presentQueueIdx, 0);

        // Command Pool
        vk::CommandPoolCreateInfo poolInfo;
        poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        poolInfo.queueFamilyIndex = m_graphicsQueueIdx;
        try {
            m_commandPool = m_device.createCommandPool(poolInfo);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to create command pool!");
        }

        // Pipeline Cache
        try {
            m_pipelineCache = m_device.createPipelineCache(vk::PipelineCacheCreateInfo());
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to create pipeline cache!");
        }

        // Max Usuable Sample Count
        VkPhysicalDeviceProperties physicalDeviceProperties = m_physicalDevice.getProperties();
        VkSampleCountFlags rawCounts = std::min(physicalDeviceProperties.limits.framebufferColorSampleCounts, physicalDeviceProperties.limits.framebufferDepthSampleCounts);
        vk::SampleCountFlags counts(rawCounts);

        if (counts & vk::SampleCountFlagBits::e64) { m_sampleCount = vk::SampleCountFlagBits::e64; }
        if (counts & vk::SampleCountFlagBits::e32) { m_sampleCount = vk::SampleCountFlagBits::e32; }
        if (counts & vk::SampleCountFlagBits::e16) { m_sampleCount = vk::SampleCountFlagBits::e16; }
        if (counts & vk::SampleCountFlagBits::e8) { m_sampleCount = vk::SampleCountFlagBits::e8; }
        if (counts & vk::SampleCountFlagBits::e4) { m_sampleCount = vk::SampleCountFlagBits::e4; }
        if (counts & vk::SampleCountFlagBits::e2) { m_sampleCount = vk::SampleCountFlagBits::e2; }
    }

    //--------------------------------------------------------------------------------------------------
    // Call on exit
    //
    void destroy()
    {
        m_device.waitIdle();

        m_device.destroyRenderPass(m_renderPass);

        m_device.destroyImageView(m_depthView);
        m_device.destroyImage(m_depthImage);
        m_device.freeMemory(m_depthMemory);

        m_device.destroyPipelineCache(m_pipelineCache);

        for (uint32_t i = 0; i < m_swapchain.imageCount; i++)
        {
            m_device.destroyFence(m_fences[i]);
            m_device.destroyFramebuffer(m_framebuffers[i]);
            m_device.destroySemaphore(m_acquireComplete[i]);
            m_device.destroySemaphore(m_renderComplete[i]);
            m_device.freeCommandBuffers(m_commandPool, m_commandBuffers[i]);
        }

        m_swapchain.deinit();

        m_device.destroyCommandPool(m_commandPool);

        m_instance.destroySurfaceKHR(m_surface);
        m_context.deinit();
    }

    //--------------------------------------------------------------------------------------------------
    // Surface for Rendering
    //
    void createSurface(uint32_t width,
                       uint32_t height,
                       vk::Format colorFormat = vk::Format::eB8G8R8A8Unorm,
                       vk::Format depthFormat = vk::Format::eD32SfloatS8Uint,
                       bool vsync = false)
    {
        m_size        = vk::Extent2D(width, height);
        m_depthFormat = depthFormat;
        m_colorFormat = colorFormat;
        m_vsync       = vsync;

        m_swapchain.init(m_physicalDevice, m_device, m_graphicsQueue, m_graphicsQueueIdx, 
                         m_presentQueue, m_presentQueueIdx, m_surface);
        m_swapchain.update(m_size, vsync);

        // Create Synch Objects
        m_fences.resize(m_swapchain.imageCount);
        m_acquireComplete.resize(m_swapchain.imageCount);
        m_renderComplete.resize(m_swapchain.imageCount);

        try {
            for (int i = 0; i < m_swapchain.imageCount; ++i) {
                m_fences[i] = m_device.createFence({ vk::FenceCreateFlagBits::eSignaled });
                m_acquireComplete[i] = m_device.createSemaphore({});
                m_renderComplete[i]  = m_device.createSemaphore({});
            }
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }

        // Create Command Buffers, store a reference to framebuffer inside their render pass info
        m_commandBuffers.resize(m_swapchain.imageCount);

        vk::CommandBufferAllocateInfo cmdBufferAllocInfo = {};
        cmdBufferAllocInfo.commandPool        = m_commandPool;
        cmdBufferAllocInfo.level              = vk::CommandBufferLevel::ePrimary;
        cmdBufferAllocInfo.commandBufferCount = m_swapchain.imageCount;

        try {
            m_commandBuffers = m_device.allocateCommandBuffers(cmdBufferAllocInfo);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    //--------------------------------------------------------------------------------------------------
    // Create the framebuffers where the image will be rendered
    // make sure swapchain is created before
    //
    void createFrameBuffers()
    {
        // recreate frame buffers
        for (auto framebuffer : m_framebuffers) {
            m_device.destroyFramebuffer(framebuffer);
        }

        // create frame buffer for every swapchain image
        for (auto imageViews : m_swapchain.images) {
            vk::ImageView attachments[2];
            attachments[0] = imageViews.view;
            attachments[1] = m_depthView;

            vk::FramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.renderPass = m_renderPass;
            framebufferInfo.attachmentCount = 2;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = m_size.width;
            framebufferInfo.height = m_size.height;
            framebufferInfo.layers = 1;

            try {
                m_framebuffers = m_swapchain.createFramebuffers(framebufferInfo);
            }
            catch (vk::SystemError err) {
                throw std::runtime_error("failed to create framebuffer!");
            }            
        }       
    }

    //--------------------------------------------------------------------------------------------------
    // A Basic default RenderPass
    // Most likely to be overwritten
    //
    void createRenderPass()
    {
        if (m_renderPass) m_device.destroy(m_renderPass);

        const bool resolve = m_sampleCount > vk::SampleCountFlagBits::e1;

        // Color Attachment
        vk::AttachmentDescription colorAttachment = {};
        colorAttachment.format         = m_colorFormat;
        colorAttachment.samples        = vk::SampleCountFlagBits::e1;
        colorAttachment.loadOp         = vk::AttachmentLoadOp::eClear;
        colorAttachment.storeOp        = vk::AttachmentStoreOp::eStore;
        colorAttachment.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
        colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        colorAttachment.initialLayout  = vk::ImageLayout::eUndefined;
        colorAttachment.finalLayout    = vk::ImageLayout::eColorAttachmentOptimal;
        
        // Depth Attachment
        vk::AttachmentDescription depthAttachment = {};
        depthAttachment.format         = m_depthFormat;
        depthAttachment.samples        = m_sampleCount;
        depthAttachment.loadOp         = vk::AttachmentLoadOp::eClear;
        depthAttachment.storeOp        = vk::AttachmentStoreOp::eStore;
        depthAttachment.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
        depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        depthAttachment.initialLayout  = vk::ImageLayout::eUndefined;
        depthAttachment.finalLayout    = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        // multisampled images cannot be presented directly.
        // We first need to resolve them to a regular image.
        vk::AttachmentDescription resolveAttachment = {};
        resolveAttachment.format         = m_colorFormat;
        resolveAttachment.samples        = m_sampleCount;
        resolveAttachment.loadOp         = vk::AttachmentLoadOp::eDontCare;
        resolveAttachment.storeOp        = vk::AttachmentStoreOp::eStore;
        resolveAttachment.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
        resolveAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        resolveAttachment.initialLayout  = vk::ImageLayout::eUndefined;
        resolveAttachment.finalLayout    = vk::ImageLayout::ePresentSrcKHR;
        
        const vk::AttachmentReference colorReference{ (uint32_t) (resolve ? 2 : 0),  vk::ImageLayout::eColorAttachmentOptimal };
        const vk::AttachmentReference depthReference{ 1, vk::ImageLayout::eDepthStencilAttachmentOptimal };
        const vk::AttachmentReference resolveReference{ 0,  vk::ImageLayout::eColorAttachmentOptimal };

        std::array<vk::AttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, resolveAttachment };

        vk::SubpassDescription subpass = {};
        subpass.pipelineBindPoint       = vk::PipelineBindPoint::eGraphics;
        subpass.colorAttachmentCount    = 1;
        subpass.pColorAttachments       = &colorReference;
        subpass.pDepthStencilAttachment = &depthReference;
        if (resolve)
            subpass.pResolveAttachments = &resolveReference;

        vk::SubpassDependency dependency = {};
        dependency.srcSubpass      = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass      = 0;
        dependency.srcStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        //dependency.srcAccessMask = 0;
        dependency.dstStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.dstAccessMask   = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
        
        vk::RenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.attachmentCount = resolve ? 3 : 2;
        renderPassInfo.pAttachments    = attachments.data();
        renderPassInfo.subpassCount    = 1;
        renderPassInfo.pSubpasses      = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies   = &dependency;

        try {
            m_renderPass = m_device.createRenderPass(renderPassInfo);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    //--------------------------------------------------------------------------------------------------
    // Image to be used as depth buffer
    //
    void createDepthBuffer()
    {
        m_device.destroyImageView(m_depthView);
        m_device.destroyImage(m_depthImage);
        m_device.freeMemory(m_depthMemory);

        // Depth Info
        const vk::ImageAspectFlags aspect = 
            vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;

        vk::ImageCreateInfo depthStencilCreateInfo = {};
        depthStencilCreateInfo.imageType   = vk::ImageType::e2D;
        depthStencilCreateInfo.extent      = vk::Extent3D(m_size.width, m_size.height, 1);
        depthStencilCreateInfo.format      = m_depthFormat;
        depthStencilCreateInfo.mipLevels   = 1;
        depthStencilCreateInfo.arrayLayers = 1;
        depthStencilCreateInfo.usage       = vk::ImageUsageFlagBits::eDepthStencilAttachment
                                             | vk::ImageUsageFlagBits::eTransferSrc;

        try {
            m_depthImage = m_device.createImage(depthStencilCreateInfo);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to create depth images!");
        }

        // Allocate the memory
        const vk::MemoryRequirements memReqs = m_device.getImageMemoryRequirements(m_depthImage);
        vk::MemoryAllocateInfo memAllocInfo;
        memAllocInfo.allocationSize = memReqs.size;
        memAllocInfo.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
        
        try {
            m_depthMemory = m_device.allocateMemory(memAllocInfo);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to allocate depth image memory!");
        }

        // Bind image & memory
        m_device.bindImageMemory(m_depthImage, m_depthMemory, 0);

        // Create an image barrier to change the layout from undefined to DepthStencilAttachmentOptimal
        vk::CommandBufferAllocateInfo cmdBufAllocateInfo;
        cmdBufAllocateInfo.commandPool        = m_commandPool;
        cmdBufAllocateInfo.level              = vk::CommandBufferLevel::ePrimary;
        cmdBufAllocateInfo.commandBufferCount = 1;

        vk::CommandBuffer cmdBuffer;
        cmdBuffer = m_device.allocateCommandBuffers(cmdBufAllocateInfo)[0];       
        cmdBuffer.begin(vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

        // barrier on top, barrier inside steup cmdbuffer
        vk::ImageSubresourceRange subresourceRange;
        subresourceRange.aspectMask = aspect;
        subresourceRange.levelCount = 1;
        subresourceRange.layerCount = 1;

        vk::ImageMemoryBarrier imageMemoryBarrier;
        imageMemoryBarrier.oldLayout        = vk::ImageLayout::eUndefined;
        imageMemoryBarrier.newLayout        = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        imageMemoryBarrier.image            = m_depthImage;
        imageMemoryBarrier.subresourceRange = subresourceRange;
        imageMemoryBarrier.srcAccessMask    = vk::AccessFlags();
        imageMemoryBarrier.dstAccessMask    = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        
        // Added this to fix error
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        
        const vk::PipelineStageFlags srcStageMask = vk::PipelineStageFlagBits::eTopOfPipe;
        const vk::PipelineStageFlags destStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests;

        cmdBuffer.pipelineBarrier(srcStageMask, destStageMask, vk::DependencyFlags(), nullptr, nullptr,
            imageMemoryBarrier);
        cmdBuffer.end();

        m_graphicsQueue.submit(vk::SubmitInfo{ 0, nullptr, nullptr, 1, &cmdBuffer }, vk::Fence());
        m_graphicsQueue.waitIdle();
        m_device.freeCommandBuffers(m_commandPool, cmdBuffer);

        // Setting up the view
        vk::ImageViewCreateInfo depthStencilView;
        depthStencilView.setViewType(vk::ImageViewType::e2D);
        depthStencilView.setFormat(m_depthFormat);
        depthStencilView.setSubresourceRange({ aspect, 0, 1, 0, 1 });
        depthStencilView.setImage(m_depthImage);
        try {
            m_depthView = m_device.createImageView(depthStencilView);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to create depth image view!");
        }
        
    }

    //--------------------------------------------------------------------------------------------------
    // 
    //
    void prepareFrame()
    {

    }

    //--------------------------------------------------------------------------------------------------
    // 
    //
    void submitFrame()
    {

    }

    //--------------------------------------------------------------------------------------------------
    // 
    //
    void setViewport()
    {

    }

    //--------------------------------------------------------------------------------------------------
    // 
    //
    void onWindowResize(uint32_t width, uint32_t height)
    {

    }

    //--------------------------------------------------------------------------------------------------
    // Collection of get methods
    //
    vk::Instance        getInstance() { return m_instance; }
    vk::Device          getDevice() { return m_device; }
    vk::PhysicalDevice  getPhysicalDevice() { return m_physicalDevice; }
    
    uint32_t            getGraphicsQueueFamily() { return m_graphicsQueueIdx; }
    uint32_t            getPresentQueueFamily() { return m_presentQueueIdx; }

    vk::Extent2D        getSize() { return m_size; }

protected:
    //--------------------------------------------------------------------------------------------------
    // 
    //
    uint32_t getMemoryType(uint32_t typeFilter, const vk::MemoryPropertyFlags& properties) const
    {
        auto deviceMemoryProperties = m_physicalDevice.getMemoryProperties();
        for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) 
                && (deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

protected:

    app::Context m_context;

    // vulkan elements
    vk::Instance                   m_instance;
    vk::Device                     m_device;
    vk::PhysicalDevice             m_physicalDevice;

    vk::SurfaceKHR                 m_surface;

    vk::Queue                      m_graphicsQueue;
    vk::Queue                      m_presentQueue;
    uint32_t                       m_graphicsQueueIdx{ VK_QUEUE_FAMILY_IGNORED };
    uint32_t                       m_presentQueueIdx{ VK_QUEUE_FAMILY_IGNORED };

    vk::CommandPool                m_commandPool;

    // Drawing / Surface
    app::SwapChain                 m_swapchain;
    std::vector<vk::Framebuffer>   m_framebuffers;    // All framebuffers, correspond to the Swapchain
    std::vector<vk::CommandBuffer> m_commandBuffers;  // Command buffer per nb element in Swapchain

    vk::RenderPass                 m_renderPass;       // Base render pass
    vk::PipelineCache              m_pipelineCache;    // Cache for pipeline/shaders

    vk::Image                      m_depthImage;       // Depth/Stencil
    vk::DeviceMemory               m_depthMemory;      // Depth/Stencil
    vk::ImageView                  m_depthView;        // Depth/Stencil

    std::vector<vk::Fence>         m_fences;           // Fences per nb element in Swapchain
    std::vector<vk::Semaphore>     m_acquireComplete;  // Swap chain image presentation
    std::vector<vk::Semaphore>     m_renderComplete;   // Command buffer submission and execution

    vk::Extent2D                   m_size{ 0, 0 };     // Size of the window
    bool                           m_vsync{ false };   // Swapchain v-Sync

    vk::SampleCountFlagBits        m_sampleCount;
    
     // Surface buffer formats
    vk::Format m_colorFormat{ vk::Format::eB8G8R8A8Unorm };
    vk::Format m_depthFormat{ vk::Format::eUndefined };

}; // class VulkanBase

} // namespace app