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