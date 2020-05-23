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
vk::ImageCreateInfo create2DInfo();

//--------------------------------------------------------------------------------------------------
// creates vk::create2DDescriptor
//
vk::ImageCreateInfo create2DDescriptor();

//--------------------------------------------------------------------------------------------------
// Create a generate all mipmaps for a vk::Image 
//
vk::ImageCreateInfo generateMipmaps();

} // namespace image
} // namespace app