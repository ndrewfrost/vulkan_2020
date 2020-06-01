/*
 *
 * Andrew Frost
 * images.cpp
 * 2020
 *
 */

#include "images.hpp"

 //////////////////////////////////////////////////////////////////////////
 // Images
 //////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
// Return the access flag for an image layout
//
vk::AccessFlags app::image::accessFlagsForLayout(vk::ImageLayout layout)
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

//--------------------------------------------------------------------------------------------------
// Return the pipeline stage for an image layout
//
vk::PipelineStageFlags app::image::pipelineStageForLayout(vk::ImageLayout layout)
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

//--------------------------------------------------------------------------------------------------
// set Image Layout
//
void app::image::setImageLayout(
    const vk::CommandBuffer& commandBuffer,
    const vk::Image& image,
    const vk::ImageLayout& oldImageLayout,
    const vk::ImageLayout& newImageLayout,
    const vk::ImageSubresourceRange& subresourceRange)
{
    vk::ImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.oldLayout        = oldImageLayout;
    imageMemoryBarrier.newLayout        = newImageLayout;
    imageMemoryBarrier.image            = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;
    imageMemoryBarrier.srcAccessMask    = accessFlagsForLayout(oldImageLayout);
    imageMemoryBarrier.dstAccessMask    = accessFlagsForLayout(newImageLayout);

    vk::PipelineStageFlags srcStageMask  = pipelineStageForLayout(oldImageLayout);
    vk::PipelineStageFlags destStageMask = pipelineStageForLayout(newImageLayout);

    commandBuffer.pipelineBarrier(srcStageMask, destStageMask,
        vk::DependencyFlags(), nullptr, nullptr, imageMemoryBarrier);
}

void app::image::setImageLayout(
    const vk::CommandBuffer& commandBuffer,
    const vk::Image& image,
    const vk::ImageAspectFlags& aspectMask,
    const vk::ImageLayout& oldImageLayout,
    const vk::ImageLayout& newImageLayout)
{
    vk::ImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = aspectMask;
    subresourceRange.levelCount = 1;
    subresourceRange.layerCount = 1;

    setImageLayout(commandBuffer, image, oldImageLayout, newImageLayout, subresourceRange);
}

//--------------------------------------------------------------------------------------------------
// Create a vk::ImageCreateInfo
//
vk::ImageCreateInfo app::image::create2DInfo(
    const vk::Extent2D& size,
    const vk::Format& format,
    const vk::ImageUsageFlags& usage,
    const bool mipmaps)
{
    vk::ImageCreateInfo createInfo = {};
    createInfo.imageType   = vk::ImageType::e2D;
    createInfo.format      = format;
    createInfo.mipLevels   = mipmaps ? app::image::mipLevels(size) : 1;
    createInfo.arrayLayers = 1;
    createInfo.extent      = vk::Extent3D{ size.width, size.height, 1 };
    createInfo.usage       = usage | vk::ImageUsageFlagBits::eTransferSrc
                                   | vk::ImageUsageFlagBits::eTransferDst;

    return createInfo;
}

//--------------------------------------------------------------------------------------------------
// creates vk::create2DDescriptor
//
vk::DescriptorImageInfo app::image::create2DDescriptor(
    const vk::Device& device,
    const vk::Image& image,
    const vk::SamplerCreateInfo& samplerCreateInfo,
    const vk::Format& format,
    const vk::ImageLayout& layout)
{
    vk::ImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.image            = image;
    imageViewCreateInfo.viewType         = vk::ImageViewType::e2D;
    imageViewCreateInfo.format           = format;
    imageViewCreateInfo.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, ~0u, 0, 1 };

    vk::DescriptorImageInfo descriptorInfo = {};
    descriptorInfo.sampler                 = device.createSampler(samplerCreateInfo);
    descriptorInfo.imageView               = device.createImageView(imageViewCreateInfo);
    descriptorInfo.imageLayout             = layout;

    return descriptorInfo;
}

//--------------------------------------------------------------------------------------------------
// mipmap generation relies on blitting
// a more sophisticated version could be done with computer shader <-- TODO
//
void app::image::generateMipmaps(
    const vk::CommandBuffer& cmdBuffer,
    const vk::Image& image,
    const vk::Format& imageFormat,
    const vk::Extent2D& size,
    const uint32_t& mipLevels)
{
    // Transfer the top level image to a layout 'eTransferSrcOptimal` and its access to 'eTransferRead'
    vk::ImageMemoryBarrier barrier = {};
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.layerCount     = 1;
    barrier.subresourceRange.levelCount     = 1;
    barrier.image                           = image;
    barrier.oldLayout                       = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.newLayout                       = vk::ImageLayout::eTransferSrcOptimal;
    barrier.srcAccessMask                   = vk::AccessFlags();
    barrier.dstAccessMask                   = vk::AccessFlagBits::eTransferRead;

    cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
        vk::DependencyFlags(), nullptr, nullptr, barrier);

    int32_t mipWidth = size.width;
    int32_t mipHeight = size.height;

    for (uint32_t i = 1; i < mipLevels; i++) {
        vk::ImageBlit blit;
        blit.srcOffsets[0]                 = vk::Offset3D{ 0, 0, 0 };
        blit.srcOffsets[1]                 = vk::Offset3D{ mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask     = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel       = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount     = 1;
        blit.dstOffsets[0]                 = vk::Offset3D{ 0, 0, 0 };
        blit.dstOffsets[1] =
            vk::Offset3D{ mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask     = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel       = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount     = 1;

        cmdBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image,
            vk::ImageLayout::eTransferDstOptimal, { blit }, vk::Filter::eLinear);
        
        // Next
        if (i + 1 < mipLevels) {
            // Transition the current miplevel into a eTransferSrcOptimal layout, to be used as the source for the next one.
            barrier.subresourceRange.baseMipLevel = i;
            barrier.oldLayout                     = vk::ImageLayout::eTransferDstOptimal;
            barrier.newLayout                     = vk::ImageLayout::eTransferSrcOptimal;
            barrier.srcAccessMask                 = vk::AccessFlags();  //vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask                 = vk::AccessFlagBits::eTransferRead;

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
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.oldLayout = vk::ImageLayout::eUndefined;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlags();
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader, vk::DependencyFlags(), nullptr,
        nullptr, barrier);

}