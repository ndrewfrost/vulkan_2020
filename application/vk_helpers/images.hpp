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

//////////////////////////////////////////////////////////////////////////
// Image Utilities                                                      //
//////////////////////////////////////////////////////////////////////////
// - Transition Pipeline Layout tools                                   //
// - 2D Texture creation helper                                         //
// - Cubic Texture creation helper                                      //
//////////////////////////////////////////////////////////////////////////

namespace image {

#ifdef max
#undef max
#endif

//-------------------------------------------------------------------------
// Transition Pipeline Layout tools
//
vk::AccessFlags        accessFlagsForLayout(vk::ImageLayout layout);
vk::PipelineStageFlags pipelineStageForLayout(vk::ImageLayout layout);

//-------------------------------------------------------------------------
// mipLevels - Returns the number of mipmaps an image can have
//
inline uint32_t mipLevels(vk::Extent2D extent)
{
    return static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
}

//-------------------------------------------------------------------------
// set Image Layout
//
void cmdBarrierImageLayout(
    vk::CommandBuffer                cmdbuffer,
    vk::Image                        image,
    vk::ImageLayout                  oldImageLayout,
    vk::ImageLayout                  newImageLayout,
    const vk::ImageSubresourceRange& subresourceRange);

void cmdBarrierImageLayout(
    vk::CommandBuffer    cmdbuffer,
    vk::Image            image,
    vk::ImageLayout      oldImageLayout,
    vk::ImageLayout      newImageLayout,
    vk::ImageAspectFlags aspectMask);

inline void cmdBarrierImageLayout(vk::CommandBuffer cmdbuffer, vk::Image image, vk::ImageLayout oldImageLayout, vk::ImageLayout newImageLayout)
{
    cmdBarrierImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, vk::ImageAspectFlagBits::eColor);
}

//-------------------------------------------------------------------------
// Create a vk::ImageCreateInfo
//
vk::ImageCreateInfo create2DInfo(
    const vk::Extent2D& size,
    const vk::Format& format = vk::Format::eR8G8B8A8Unorm,
    const vk::ImageUsageFlags& usage = vk::ImageUsageFlagBits::eSampled,
    const bool mipmaps = false);

//-------------------------------------------------------------------------
// Create a vk::ImageViewCreateInfo
//
vk::ImageViewCreateInfo makeImageViewCreateInfo(
    vk::Image                  image,
    const vk::ImageCreateInfo& imageInfo,
    bool                       isCube = false);

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
    uint32_t layerCount = 1, 
    vk::ImageLayout currentLayout = vk::ImageLayout::eShaderReadOnlyOptimal);

} // namespace images
} // namespace app