/*
 *
 * Andrew Frost
 * swapchain.cpp
 * 2020
 *
 */

#pragma once

#include "swapchain.hpp"

#ifdef _DEBUG
#include "../vk_helpers/debug.hpp"
static app::debug::DebugUtil s_debug;
#endif // _DEBUG

namespace app {

///////////////////////////////////////////////////////////////////////////
// Swapchain                                                             //
///////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------
// Initialization of swapchain class variables
//
bool SwapChain::init(vk::Instance instance, vk::Device device, 
    vk::PhysicalDevice physicalDevice, vk::Queue graphicsQueue, 
    uint32_t graphicsQueueIdx, vk::SurfaceKHR surface, vk::Format format)
{
    assert(!m_device && "VkDevice must exist for swapchain");
    m_device           = device;
    m_physicalDevice   = physicalDevice;
    m_graphicsQueue    = graphicsQueue;
    m_graphicsQueueIdx = graphicsQueueIdx;
    m_surface          = surface;

    m_changeID = 0;
    m_currentSemaphore = 0;

#if _DEBUG
    s_debug.setup(device, instance);
#endif

    // Choose Format
    std::vector<vk::SurfaceFormatKHR> surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface);

    m_surfaceFormat = vk::Format::eB8G8R8A8Unorm;
    m_surfaceColor  = surfaceFormats[0].colorSpace;

    // Check if  format is supported 
    for (uint32_t i = 0; i < static_cast<uint32_t>(surfaceFormats.size()); i++) {
        if (surfaceFormats[i].format == format) {
            m_surfaceFormat = format;
            m_surfaceColor  = surfaceFormats[i].colorSpace;
            return true;
        }
    }
    
    return false;        
}

//-------------------------------------------------------------------------
// Deinitiate Resources of SwapChain and Swapchain
//
void SwapChain::deinitResources()
{
    if (!m_device)
        return;

    vkDeviceWaitIdle(m_device);
    
    for (auto iter : m_entries) {
        m_device.destroyImageView(iter.imageView);
        m_device.destroySemaphore(iter.readSemaphore);
        m_device.destroySemaphore(iter.writtenSemaphore);
    }

    if(m_swapchain) 
        m_device.destroySwapchainKHR(m_swapchain);
   
    m_entries.clear();
    m_barriers.clear();
}

//-------------------------------------------------------------------------
// Destroy Swapchain and variables
//
void SwapChain::destroy()
{
    deinitResources();
    m_physicalDevice = vk::PhysicalDevice();
    m_device         = vk::Device();
    m_surface        = vk::SurfaceKHR();
    m_changeID       = 0;
}

//-------------------------------------------------------------------------
// Update the swapchain configuration
//
void SwapChain::update(int width, int height, bool vsync)
{
    m_changeID++;

    if (!m_physicalDevice || !m_device || !m_surface) {
        throw std::runtime_error(" failed to initialize the physicalDevice, device, and queue members for swapchain");
    }

    const vk::SwapchainKHR oldSwapchain = m_swapchain;

    vkDeviceWaitIdle(m_device);

    // get physical device surface capabilities
    vk::SurfaceCapabilitiesKHR surfaceCaps = m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface);

    // get present modes
    std::vector<vk::PresentModeKHR> presentModes = m_physicalDevice.getSurfacePresentModesKHR(m_surface);
    // everyone must support FIFO mode
    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;

    for (const auto& availablePresentMode : presentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            presentMode = availablePresentMode; break;
        }
        else if (availablePresentMode == vk::PresentModeKHR::eImmediate) {
            presentMode = availablePresentMode; break;
        }
    }

    // get Extent
    VkExtent2D swapchainExtent;
    // width and height are either both - 1, or both not - 1.
    if (surfaceCaps.currentExtent.width == -1) {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapchainExtent = vk::Extent2D{width, height};
    }
    else {
        // If the surface size is defined, the swap chain size must match
        swapchainExtent = surfaceCaps.currentExtent;
    }

    // Determine number of images
    // We desire 1 image at a time, beside images being displayed and queued
    uint32_t desiredSwapchainImages = surfaceCaps.minImageCount + 1;
    if (surfaceCaps.maxImageCount > 0 && desiredSwapchainImages > surfaceCaps.maxImageCount) {
        // application must settle for fewer than desired images
        desiredSwapchainImages = surfaceCaps.maxImageCount;
    }

    // Transform Flag bits
    vk::SurfaceTransformFlagBitsKHR preTransform = {};
    if (surfaceCaps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) {
        preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    }
    else {
        preTransform = surfaceCaps.currentTransform;
    }

    vk::SwapchainCreateInfoKHR createInfo = {};
    createInfo.surface          = m_surface;
    createInfo.minImageCount    = desiredSwapchainImages;
    createInfo.imageFormat      = m_surfaceFormat;
    createInfo.imageColorSpace  = m_surfaceColor;
    createInfo.imageExtent      = swapchainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment 
                                | vk::ImageUsageFlagBits::eStorage 
                                | vk::ImageUsageFlagBits::eTransferDst;
    createInfo.preTransform     = preTransform;
    createInfo.presentMode      = presentMode;
    createInfo.clipped          = VK_TRUE;
    createInfo.compositeAlpha  = vk::CompositeAlphaFlagBitsKHR::eOpaque;

    createInfo.oldSwapchain = oldSwapchain;

    if (m_graphicsQueueIdx != m_presentQueueIdx) {
        uint32_t indices[] = { m_graphicsQueueIdx, m_presentQueueIdx };

        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = indices;
    }
    else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    try {
        m_swapchain = m_device.createSwapchainKHR(createInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create swap chain!");
    }

#if _DEBUG
    s_debug.setObjectName(m_swapchain, "SwapChain::m_swapchain");
#endif

    // if existing swapchain is re-created, destroy old swapchain and cleanup
    if (oldSwapchain) {
        for (auto iter : m_entries){
            m_device.destroyImageView(iter.imageView);
            m_device.destroySemaphore(iter.readSemaphore);
            m_device.destroySemaphore(iter.writtenSemaphore);
        }
        m_device.destroySwapchainKHR(oldSwapchain);
    }

    // get Images
    vk::ImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.format = m_surfaceFormat;
    imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
    imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    auto swapchainImages = m_device.getSwapchainImagesKHR(m_swapchain);
    m_imageCount = (uint32_t)swapchainImages.size();

    m_entries.resize(m_imageCount);
    m_barriers.resize(m_imageCount);
    
    for (uint32_t i = 0; i < m_imageCount; i++) {
        Entry& entry = m_entries[i];
        
        // image
        entry.image = swapchainImages[i];
       
        // imageview
        imageViewCreateInfo.image = swapchainImages[i];
        try {
            entry.imageView = m_device.createImageView(imageViewCreateInfo);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to create image views!");
        }

        // semaphore
        vk::SemaphoreCreateInfo semCreateInfo = {};
        entry.readSemaphore    = m_device.createSemaphore(semCreateInfo);
        entry.writtenSemaphore = m_device.createSemaphore(semCreateInfo);

        // initial barriers
        vk::ImageSubresourceRange range = {};
        range.aspectMask     = vk::ImageAspectFlagBits::eColor;
        range.baseMipLevel   = 0;
        range.levelCount     = VK_REMAINING_MIP_LEVELS;
        range.baseArrayLayer = 0;
        range.layerCount     = VK_REMAINING_ARRAY_LAYERS;

        vk::ImageMemoryBarrier barrier = {};
        barrier.oldLayout        = vk::ImageLayout::eUndefined;
        barrier.newLayout        = vk::ImageLayout::ePresentSrcKHR;
        barrier.image            = entry.image;
        barrier.subresourceRange = range;

        m_barriers[i] = barrier;

#if _DEBUG
    entry.debugImageName            = "swapchainImage:" + std::to_string(i);
    entry.debugImageViewName        = "swapchainImageView:" + std::to_string(i);
    entry.debugReadSemaphoreName    = "swapchainReadSemaphore:" + std::to_string(i);
    entry.debugWrittenSemaphoreName = "swapchainWrittenSemaphore:" + std::to_string(i);
    s_debug.setObjectName(entry.image, entry.debugImageName.c_str());
    s_debug.setObjectName(entry.imageView, entry.debugImageViewName.c_str());
    s_debug.setObjectName(entry.readSemaphore, entry.debugReadSemaphoreName.c_str());
    s_debug.setObjectName(entry.writtenSemaphore, entry.debugWrittenSemaphoreName.c_str());
#endif
    }

    m_width  = width;
    m_height = height;
    m_vsync  = vsync;

    m_currentSemaphore = 0;
    m_currentImage     = 0;
}

//-------------------------------------------------------------------------
// Sets active index
//
vk::Result SwapChain::acquire()
{
    return acquireSemaphore(getActiveReadSemaphore());
}

//-------------------------------------------------------------------------
// Sets active index
// Can provide own semaphore
//
vk::Result SwapChain::acquireSemaphore(vk::Semaphore semaphore)
{
    const vk::Result result 
        = m_device.acquireNextImageKHR(m_swapchain, UINT64_MAX, semaphore, {}, &m_currentImage);
    
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        throw std::error_code(result);
    }
    return result;
}

//-------------------------------------------------------------------------
// present on provided queue
//
void SwapChain::present(vk::Queue queue)
{
    //vk::Semaphore& written = m_entries[(m_currentSemaphore % m_imageCount)].writtenSemaphore;
    const vk::Semaphore& written = getActiveWrittenSemaphore();

    vk::PresentInfoKHR presentInfo = {};
    presentInfo.swapchainCount     = 1;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &written;
    presentInfo.pSwapchains        = &m_swapchain;
    presentInfo.pImageIndices      = &m_currentImage;

    m_currentSemaphore++;

    m_graphicsQueue.presentKHR(presentInfo);
}

//-------------------------------------------------------------------------
// vkCmdPipelineBarrier for VK_IMAGE_LAYOUT_UNDEFINED to 
// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR. Must apply resource transitions
// after update calls
//
void SwapChain::cmdUpdateBarriers(vk::CommandBuffer cmdBuffer) const
{
    cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                              vk::PipelineStageFlagBits::eTopOfPipe, 
                              vk::DependencyFlags(), 0, NULL, 0, NULL, m_imageCount, 
                              m_barriers.data());
}

//-------------------------------------------------------------------------
// Get Methods
//
VkSemaphore SwapChain::getActiveReadSemaphore() const
{
    return m_entries[(m_currentSemaphore % m_imageCount)].readSemaphore;
}

VkSemaphore SwapChain::getActiveWrittenSemaphore() const
{
    return m_entries[(m_currentSemaphore % m_imageCount)].writtenSemaphore;
}

VkImage SwapChain::getActiveImage() const
{
    return m_entries[m_currentImage].image;
}

VkImageView SwapChain::getActiveImageView() const
{
    return m_entries[m_currentImage].imageView;
}

vk::Image SwapChain::getImage(uint32_t i) const
{
    if (i >= m_imageCount)
        return nullptr;
    return m_entries[i].image;
}

vk::ImageView SwapChain::getImageView(uint32_t i) const
{
    if (i >= m_imageCount)
        return nullptr;
    return m_entries[i].imageView;
}

} // namespace app