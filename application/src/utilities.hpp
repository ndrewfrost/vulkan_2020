/*
 *
 * Andrew Frost
 * utilities.hpp
 * 2020
 *
 */

#pragma once

#include <fstream>
#include <vulkan/vulkan.hpp>

#include "glm/glm.hpp"

namespace app {
namespace util {


//--------------------------------------------------------------------------------------------------
// Clear Color Constructor
//
inline vk::ClearColorValue clearColor(const glm::vec4& v = glm::vec4(0.f, 0.f, 0.f, 0.f))
{
    vk::ClearColorValue result;
    memcpy(&result.float32, &v.x, sizeof(result.float32));
    return result;
}

//--------------------------------------------------------------------------------------------------
// Convenient function to load the shaders (SPIR-V)
//
inline std::vector<char> readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    auto fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

} // namespace util
} // namespace app