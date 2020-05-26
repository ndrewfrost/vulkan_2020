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
// Swapchain Struct
///////////////////////////////////////////////////////////////////////////

struct Swapchain
{
    struct SwapchainImage
    {
        vk::Image     image;
        vk::ImageView view;
    };

    vk::PhysicalDevice          physicalDevice;
    vk::Device                  device;

    vk::SurfaceKHR              surface;

    vk::SwapchainKHR            swapchain;
    std::vector<SwapchainImage> images;
    uint32_t                    imageCount{ 0 };

    vk::Format                  imageFormat{ vk::Format::eUndefined };
    vk::ColorSpaceKHR           colorSpace{ vk::ColorSpaceKHR::eSrgbNonlinear };

    vk::Queue                   graphicsQueue;
    uint32_t                    graphicsQueueIdx{ VK_QUEUE_FAMILY_IGNORED };

    vk::Queue                   presentQueue;
    uint32_t                    presentQueueIdx{ VK_QUEUE_FAMILY_IGNORED };

    //--------------------------------------------------------------------------------------------------
    //
    //
    void init(const vk::PhysicalDevice& newPhysicalDevice,
              const vk::Device&         newDevice,
              const vk::Queue&          newGraphicsQueue,
              uint32_t                  newGraphicsQueueIdx,
              const vk::Queue&          newPresentQueue,
              uint32_t                  newPresentQueueIdx,
              const vk::SurfaceKHR&     newSurface,
              vk::Format                newColorFormat = vk::Format::eUndefined)
    {
        physicalDevice   = newPhysicalDevice;
        device           = newDevice;
        graphicsQueue    = newGraphicsQueue;
        graphicsQueueIdx = newGraphicsQueueIdx;
        presentQueue     = newPresentQueue;
        presentQueueIdx  = newPresentQueueIdx;

        surface = newSurface;

        // Choose Format
        {
            std::vector<vk::SurfaceFormatKHR> surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface);

            imageFormat = vk::Format::eB8G8R8A8Unorm;
            colorSpace = surfaceFormats[0].colorSpace;

            // Check if new color format is supported 
            if (newColorFormat != vk::Format::eB8G8R8A8Unorm) {
                for (auto& availableFormat : surfaceFormats) {
                    if (availableFormat.format == newColorFormat) {
                        imageFormat = availableFormat.format;
                        colorSpace  = availableFormat.colorSpace;
                        return;
                    }
                }
            }
            // Else select here
            for (const auto& availableFormat : surfaceFormats) {
                if (availableFormat.format == vk::Format::eB8G8R8A8Unorm && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                    imageFormat = availableFormat.format;
                    colorSpace  = availableFormat.colorSpace;
                    return;
                }
            }
        }
    }

    //--------------------------------------------------------------------------------------------------
    //
    //
    void deinit()
    {
        for (uint32_t i = 0; i < imageCount; i++) {
            device.destroyImageView(images[i].view);
        }

        if (swapchain) {
            device.destroySwapchainKHR(swapchain);
        }
    }

    //--------------------------------------------------------------------------------------------------
    //
    //
    void update(vk::Extent2D& size, bool vsync = false)
    {
        if (!physicalDevice || !device || !surface) {
            throw std::runtime_error("Initialize the physicalDevice, device, and queue members");
        }

        const vk::SwapchainKHR oldSwapchain = swapchain;

        // get physical device surface capabilities
        vk::SurfaceCapabilitiesKHR surfaceCaps = physicalDevice.getSurfaceCapabilitiesKHR(surface);

        // get present modes
        std::vector<vk::PresentModeKHR> presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
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
            swapchainExtent = size;
        }
        else {
            // If the surface size is defined, the swap chain size must match
            swapchainExtent = surfaceCaps.currentExtent;
            size = surfaceCaps.currentExtent;
        }

        // Find Number of images
        uint32_t desiredSwapchainImages = surfaceCaps.minImageCount + 1;
        if (surfaceCaps.maxImageCount > 0 && imageCount > surfaceCaps.maxImageCount) {
            surfaceCaps.minImageCount = surfaceCaps.maxImageCount;
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
        createInfo.surface          = surface;
        createInfo.minImageCount    = desiredSwapchainImages;
        createInfo.imageFormat      = imageFormat;
        createInfo.imageColorSpace  = colorSpace;
        createInfo.imageExtent      = swapchainExtent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
        createInfo.preTransform     = preTransform;
        createInfo.presentMode      = presentMode;
        createInfo.clipped          = VK_TRUE;
        createInfo.compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque;

        createInfo.oldSwapchain     = oldSwapchain;

        if (graphicsQueueIdx != presentQueueIdx) {
            uint32_t indices[] = { graphicsQueueIdx, presentQueueIdx };

            createInfo.imageSharingMode      = vk::SharingMode::eConcurrent;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices   = indices;
        }
        else {
            createInfo.imageSharingMode      = vk::SharingMode::eExclusive;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices   = nullptr;
        }

        try {
            swapchain = device.createSwapchainKHR(createInfo);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to create swap chain!");
        }

        // if existing swapchain is re-created, destroy old swapchain and cleanup
        if (oldSwapchain) {
            for (uint32_t i = 0; i < imageCount; i++) {
                device.destroyImageView(images[i].view);
            }
            device.destroySwapchainKHR(oldSwapchain);
        }

        // get Images
        vk::ImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.format                      = imageFormat;
        imageViewCreateInfo.viewType                    = vk::ImageViewType::e2D;
        imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        auto swapchainImages = device.getSwapchainImagesKHR(swapchain);
        imageCount = (uint32_t)swapchainImages.size();

        images.resize(imageCount);

        for (uint32_t i = 0; i < imageCount; i++) {
            images[i].image           = swapchainImages[i];
            imageViewCreateInfo.image = swapchainImages[i];

            try {
                images[i].view = device.createImageView(imageViewCreateInfo);
            }
            catch (vk::SystemError err) {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }

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

    //--------------------------------------------------------------------------------------------------
    //
    //
    vk::Result acquire(const vk::Semaphore& presentCompleteSemaphore, uint32_t* imageIndex)
    {
        const vk::Result result = device.acquireNextImageKHR(swapchain, UINT64_MAX, presentCompleteSemaphore, {}, imageIndex);
        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            throw std::error_code(result);
        }
        return result;
    }

    //--------------------------------------------------------------------------------------------------
    //
    //
    vk::Result present(uint32_t imageIndex, vk::Semaphore waitSemaphore) 
    {
        vk::PresentInfoKHR presentInfo = {};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains    = &swapchain;
        presentInfo.pImageIndices  = &imageIndex;

        // Check if wait semaphore has been specified to wait for, before presenting the image
        if (waitSemaphore) {
            presentInfo.pWaitSemaphores    = &waitSemaphore;
            presentInfo.waitSemaphoreCount = 1;
        }

        return graphicsQueue.presentKHR(presentInfo);
    }

}; // struct SwapChain

} // namespace app