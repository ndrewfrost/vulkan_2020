/*
 *
 * Andrew Frost
 * swapchain.hpp
 * 2020
 *
 */

#pragma once

#include <vulkan/vulkan.hpp>

namespace app {

///////////////////////////////////////////////////////////////////////////
// SwapChain                                                             //
///////////////////////////////////////////////////////////////////////////

class SwapChain
{
public:
    // Initiation functions
    SwapChain(SwapChain const&) = delete;
    SwapChain& operator=(SwapChain const&) = delete;

    SwapChain() {}
    SwapChain(vk::Instance instance, vk::Device device, vk::PhysicalDevice physicalDevice,
        vk::Queue graphicsQueue, uint32_t graphicsQueueIdx, vk::SurfaceKHR surface, vk::Format format = vk::Format::eB8G8R8A8Unorm)
    {
        init(instance, device, physicalDevice, graphicsQueue, graphicsQueueIdx, surface, format);
    }
    ~SwapChain() { destroy(); }

    bool init(vk::Instance instance, vk::Device device, vk::PhysicalDevice physicalDevice, vk::Queue graphicsQueue,
        uint32_t graphicsQueueIdx, vk::SurfaceKHR surface, vk::Format format = vk::Format::eB8G8R8A8Unorm);

    // Clear swapchain
    void deinitResources();
    void destroy();

    // Update swapchain Configuration
    void update(int width, int height, bool vsync);
    void update(int width, int height) { update(width, height, m_vsync); }
    
    // Aquire active index
    vk::Result acquire();
    vk::Result acquireSemaphore(vk::Semaphore semaphore);

    // Present 
    void present() { present(m_graphicsQueue); }
    void present(vk::Queue queue);

    // Update Barriers
    void cmdUpdateBarriers(vk::CommandBuffer cmdBuffer) const;

    // Getting Methods
    VkSemaphore getActiveReadSemaphore() const;
    VkSemaphore getActiveWrittenSemaphore() const;
    VkImage     getActiveImage() const;
    VkImageView getActiveImageView() const; 
    uint32_t    getActiveImageIndex() const { return m_currentImage; }

    uint32_t         getImageCount()          const { return m_imageCount; }
    vk::Image        getImage(uint32_t i)     const;
    vk::ImageView    getImageView(uint32_t i) const;
    vk::Format       getFormat()              const { return m_surfaceFormat; }
    uint32_t         getWidth()               const { return m_width; }
    uint32_t         getHeight()              const { return m_height; }
    bool             getVsync()               const { return m_vsync; }
    vk::SwapchainKHR getSwapchain()           const { return m_swapchain; }
    uint32_t         getChangeID()            const { return m_changeID;  }

private:

    struct Entry
    {
        vk::Image     image{};
        vk::ImageView imageView{};
        vk::Semaphore readSemaphore{};
        vk::Semaphore writtenSemaphore{};
#if _DEBUG
        std::string debugImageName;
        std::string debugImageViewName;
        std::string debugReadSemaphoreName;
        std::string debugWrittenSemaphoreName;
#endif
    };

    vk::Device                          m_device;
    vk::PhysicalDevice                  m_physicalDevice;

    vk::Queue                           m_graphicsQueue;
    uint32_t                            m_graphicsQueueIdx{ VK_QUEUE_FAMILY_IGNORED };
    vk::Queue                           m_presentQueue;
    uint32_t                            m_presentQueueIdx{ VK_QUEUE_FAMILY_IGNORED };

    vk::SurfaceKHR                      m_surface;
    vk::Format                          m_surfaceFormat{};
    vk::ColorSpaceKHR                   m_surfaceColor{};

    vk::SwapchainKHR                    m_swapchain;
    uint32_t                            m_imageCount{ 0 };

    std::vector<Entry>                  m_entries;
    std::vector<vk::ImageMemoryBarrier> m_barriers;

    uint32_t                            m_currentImage{ 0 };
    uint32_t                            m_currentSemaphore{ 0 };
    uint32_t                            m_changeID{ 0 };

    uint32_t                            m_width{ 0 };
    uint32_t                            m_height{ 0 };
    bool                                m_vsync = false;

    /*
    //--------------------------------------------------------------------------------------------------
    //
    //
    
    std::vector<vk::Framebuffer> createFramebuffer(vk::FramebufferCreateInfo framebufferCreateInfo)
    {
        // Verify that the first attachment is null
        assert(framebufferCreateInfo.pAttachments[0] == vk::ImageView());

        std::vector<vk::ImageView> attachments;
        attachments.resize(framebufferCreateInfo.attachmentCount);
        for (size_t i = 0; i < framebufferCreateInfo.attachmentCount; ++i) {
            attachments[i] = framebufferCreateInfo.pAttachments[i];
        }

        framebufferCreateInfo.pAttachments = attachments.data();

        std::vector<vk::Framebuffer> framebuffers;
        framebuffers.resize(imageCount);
        for (uint32_t i = 0; i < imageCount; i++) {
            attachments[0] = images[i].view;
            framebuffers[i] = device.createFramebuffer(framebufferCreateInfo);
        }
        return framebuffers;
    }
*/

}; // class SwapChain

} // namespace app