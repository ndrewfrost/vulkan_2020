/*
 *
 * Andrew Frost
 * images.cpp
 * 2020
 *
 */

#include "images.hpp"

namespace app {
namespace image {
//////////////////////////////////////////////////////////////////////////
// Images                                                               //
//////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------
// Return the access flag for an image layout
//
vk::AccessFlags accessFlagsForLayout(vk::ImageLayout layout)
{
    switch (layout) {
    case vk::ImageLayout::ePreinitialized:
        return vk::AccessFlagBits::eHostWrite;
    case vk::ImageLayout::eTransferDstOptimal:
        return vk::AccessFlagBits::eTransferWrite;
    case vk::ImageLayout::eTransferSrcOptimal:
        return vk::AccessFlagBits::eTransferRead;
    case vk::ImageLayout::eColorAttachmentOptimal:
        return vk::AccessFlagBits::eColorAttachmentWrite;
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        return vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
        return vk::AccessFlagBits::eShaderRead;
    default:
        return vk::AccessFlags();
    }
}

//-------------------------------------------------------------------------
// Return the pipeline stage for an image layout
//
vk::PipelineStageFlags pipelineStageForLayout(vk::ImageLayout layout)
{
    switch (layout) {
    case vk::ImageLayout::eTransferDstOptimal:
    case vk::ImageLayout::eTransferSrcOptimal:
        return vk::PipelineStageFlagBits::eTransfer;
    case vk::ImageLayout::eColorAttachmentOptimal:
        return vk::PipelineStageFlagBits::eColorAttachmentOutput;
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        return vk::PipelineStageFlagBits::eEarlyFragmentTests;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
        return vk::PipelineStageFlagBits::eFragmentShader;
    case vk::ImageLayout::ePreinitialized:
        return vk::PipelineStageFlagBits::eHost;
    case vk::ImageLayout::eUndefined:
        return vk::PipelineStageFlagBits::eTopOfPipe;
    default:
        return vk::PipelineStageFlagBits::eBottomOfPipe;
    }
}

//-------------------------------------------------------------------------
// set Image Layout
//
void cmdBarrierImageLayout(
    vk::CommandBuffer cmdbuffer,
    vk::Image image,
    vk::ImageLayout oldImageLayout,
    vk::ImageLayout newImageLayout,
    const vk::ImageSubresourceRange& subresourceRange)
{
    vk::ImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.oldLayout = oldImageLayout;
    imageMemoryBarrier.newLayout = newImageLayout;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;
    imageMemoryBarrier.srcAccessMask = accessFlagsForLayout(oldImageLayout);
    imageMemoryBarrier.dstAccessMask = accessFlagsForLayout(newImageLayout);

    vk::PipelineStageFlags srcStageMask = pipelineStageForLayout(oldImageLayout);
    vk::PipelineStageFlags destStageMask = pipelineStageForLayout(newImageLayout);

    cmdbuffer.pipelineBarrier(srcStageMask, destStageMask,
        vk::DependencyFlags(), nullptr, nullptr, imageMemoryBarrier);
}

void cmdBarrierImageLayout(
    vk::CommandBuffer cmdbuffer,
    vk::Image image,
    vk::ImageLayout oldImageLayout,
    vk::ImageLayout newImageLayout,
    vk::ImageAspectFlags aspectMask)
{
    vk::ImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = aspectMask;
    subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.baseArrayLayer = 0;

    cmdBarrierImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange);
}

//-------------------------------------------------------------------------
// Create a vk::ImageCreateInfo
//
vk::ImageCreateInfo create2DInfo(
    const vk::Extent2D& size,
    const vk::Format& format,
    const vk::ImageUsageFlags& usage,
    const bool mipmaps,
    const vk::SampleCountFlagBits samples)
{
    vk::ImageCreateInfo createInfo = {};
    createInfo.imageType = vk::ImageType::e2D;
    createInfo.format = format;
    createInfo.samples = samples;
    createInfo.mipLevels = mipmaps ? mipLevels(size) : 1;
    createInfo.arrayLayers = 1;
    createInfo.extent = vk::Extent3D{ size.width, size.height, 1 };
    createInfo.usage = usage | vk::ImageUsageFlagBits::eTransferSrc
        | vk::ImageUsageFlagBits::eTransferDst;

    return createInfo;
}

//-------------------------------------------------------------------------
// creates vk::ImageViewCreateInfo
//
vk::ImageViewCreateInfo makeImageViewCreateInfo(
    vk::Image image,
    const vk::ImageCreateInfo& imageInfo,
    bool isCube)
{
    vk::ImageViewCreateInfo viewInfo = {};
    viewInfo.pNext = nullptr;
    viewInfo.image = image;

    switch (imageInfo.imageType) {
    case vk::ImageType::e1D:
        viewInfo.viewType = vk::ImageViewType::e1D;
        break;
    case vk::ImageType::e2D:
        viewInfo.viewType = isCube ? vk::ImageViewType::eCube : vk::ImageViewType::e2D;
        break;
    case vk::ImageType::e3D:
        viewInfo.viewType = vk::ImageViewType::e3D;
        break;
    default:
        assert(0);
    }

    viewInfo.format = imageInfo.format;
    viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return viewInfo;
}


//-------------------------------------------------------------------------
// mipmap generation relies on blitting
// a more sophisticated version could be done with computer shader <-- TODO
//
void generateMipmaps(
    vk::CommandBuffer cmdBuffer,
    vk::Image image,
    vk::Format imageFormat,
    const VkExtent2D& size,
    uint32_t levelCount,
    uint32_t layerCount,
    vk::ImageLayout currentLayout)
{
    // Transfer the top level image to a layout 'eTransferSrcOptimal` and its access to 'eTransferRead'
    vk::ImageMemoryBarrier barrier = {};
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.layerCount = layerCount;
    barrier.subresourceRange.levelCount = 1;
    barrier.image = image;
    barrier.oldLayout = currentLayout;
    barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.srcAccessMask = accessFlagsForLayout(currentLayout);
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

    cmdBuffer.pipelineBarrier(pipelineStageForLayout(currentLayout), vk::PipelineStageFlagBits::eTransfer,
        vk::DependencyFlags(), nullptr, nullptr, barrier);

    // transfer remaining mips to DST optimal
    barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.subresourceRange.baseMipLevel = 1;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

    cmdBuffer.pipelineBarrier(pipelineStageForLayout(currentLayout), vk::PipelineStageFlagBits::eTransfer,
        vk::DependencyFlags(), nullptr, nullptr, barrier);

    int32_t mipWidth = size.width;
    int32_t mipHeight = size.height;

    for (uint32_t i = 1; i < levelCount; i++) {
        vk::ImageBlit blit;
        blit.srcOffsets[0] = vk::Offset3D{ 0, 0, 0 };
        blit.srcOffsets[1] = vk::Offset3D{ mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = layerCount;
        blit.dstOffsets[0] = vk::Offset3D{ 0, 0, 0 };
        blit.dstOffsets[1] =
            vk::Offset3D{ mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = layerCount;

        cmdBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image,
            vk::ImageLayout::eTransferDstOptimal, { blit }, vk::Filter::eLinear);

        // Next
        {
            // Transition the current miplevel into a eTransferSrcOptimal layout, to be used as the source for the next one.
            barrier.subresourceRange.baseMipLevel = i;
            barrier.subresourceRange.levelCount   = 1;
            barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite; 
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

            cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(), nullptr,
                nullptr, barrier);
        }

        if (mipWidth > 1)
            mipWidth /= 2;
        if (mipHeight > 1)
            mipHeight /= 2;
    }

    //Transition all miplevels into a eShaderReadOnlyOptimal layout.
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.newLayout = currentLayout;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
    barrier.dstAccessMask = accessFlagsForLayout(currentLayout);

    cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
        pipelineStageForLayout(currentLayout), vk::DependencyFlags(), nullptr,
        nullptr, barrier);

}

} //namespace image
} //namespace app