/*
 *
 * Andrew Frost
 * images.hpp
 * 2020
 *
 */

#pragma once

#include <vulkan/vulkan.hpp>

#include <algorithm>

namespace app {

//--------------------------------------------------------------------------------------------------
// Various Image Utilities

namespace image {

#ifdef max
#undef max
#endif

//--------------------------------------------------------------------------------------------------
// mipLevels - Returns the number of mipmaps an image can have
//
inline uint32_t mipLevels(vk::Extent2D extent)
{
    return static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
}

//--------------------------------------------------------------------------------------------------
// Return the access flag for an image layout
//
vk::AccessFlags accessFlagsForLayout(vk::ImageLayout layout)
{
    switch (layout)
    {
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
vk::PipelineStageFlags pipelineStageForLayout(vk::ImageLayout layout)
{
    switch (layout)
    {
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
inline void setImageLayout(
    const vk::CommandBuffer& commandBuffer,
    const vk::Image& image,
    const vk::ImageLayout& oldImageLayout,
    const vk::ImageLayout& newImageLayout)
{
    setImageLayout(commandBuffer, image, vk::ImageAspectFlagBits::eColor, oldImageLayout, newImageLayout);
}

void setImageLayout(
    const vk::CommandBuffer& commandBuffer,
    const vk::Image& image,
    const vk::ImageLayout& oldImageLayout,
    const vk::ImageLayout& newImageLayout,
    const vk::ImageSubresourceRange& subresourceRange)
{
    vk::ImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.oldLayout         = oldImageLayout;
    imageMemoryBarrier.newLayout         = newImageLayout;
    imageMemoryBarrier.image             = image;
    imageMemoryBarrier.subresourceRange  = subresourceRange;
    imageMemoryBarrier.srcAccessMask     = accessFlagsForLayout(oldImageLayout);
    imageMemoryBarrier.dstAccessMask     = accessFlagsForLayout(newImageLayout);
    
    vk::PipelineStageFlags srcStageMask  = pipelineStageForLayout(oldImageLayout);
    vk::PipelineStageFlags destStageMask = pipelineStageForLayout(newImageLayout);
    
    commandBuffer.pipelineBarrier(srcStageMask, destStageMask, 
        vk::DependencyFlags(), nullptr, nullptr, imageMemoryBarrier);
}

void setImageLayout(
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
vk::ImageCreateInfo create2DInfo(
    const vk::Extent2D& size,
    const vk::Format& format = vk::Format::eR8G8B8A8Unorm,
    const vk::ImageUsageFlags& usage = vk::ImageUsageFlagBits::eSampled,
    const bool mipmaps = false)
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
vk::DescriptorImageInfo create2DDescriptor(
    const vk::Device& device,
    const vk::Image& image,
    const vk::SamplerCreateInfo& samplerCreateInfo = vk::SamplerCreateInfo(),
    const vk::Format& format = vk::Format::eR8G8B8A8Unorm,
    const vk::ImageLayout& layout = vk::ImageLayout::eShaderReadOnlyOptimal)
{

    vk::ImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.image            = image;
    imageViewCreateInfo.viewType         = vk::ImageViewType::e2D;
    imageViewCreateInfo.format           = format;
    imageViewCreateInfo.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, ~0u, 0, 1 };

    vk::DescriptorImageInfo descriptorInfo;
    descriptorInfo.sampler     = device.createSampler(samplerCreateInfo);
    descriptorInfo.imageView   = device.createImageView(imageViewCreateInfo);
    descriptorInfo.imageLayout = layout;

    return descriptorInfo;
}

//--------------------------------------------------------------------------------------------------
// Create a generate all mipmaps for a vk::Image 
//
vk::ImageCreateInfo generateMipmaps();

} // namespace image
} // namespace app