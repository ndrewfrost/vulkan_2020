/*
 *
 * Andrew Frost
 * context.hpp
 * 2020
 *
 */

#pragma once

#include <set>
#include <vector>
#include <iostream>
#include <vulkan/vulkan.hpp>

#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

namespace app {

///////////////////////////////////////////////////////////////////////////
// ContextCreateInfo
///////////////////////////////////////////////////////////////////////////
/**

Allows app to specify features that are expected of 
- vk::Instance
- vk::Device

*/
struct ContextCreateInfo
{
    ContextCreateInfo();

    void addDeviceExtension(const char * name);

    void addInstanceExtension(const char* name);

    void addValidationLayer(const char * name);

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    uint32_t numDeviceExtensions;
    std::vector<const char*> deviceExtensions;

    uint32_t numValidationLayers;
    std::vector<const char*> validationLayers;

    uint32_t numInstanceExtensions;
    std::vector<const char*> instanceExtensions;

    const char* appEngine = "No Engine";
    const char* appTitle  = "Application";
};

///////////////////////////////////////////////////////////////////////////
// Context
///////////////////////////////////////////////////////////////////////////
class Context
{
public:
    Context() = default;
    void deinit();

    void initInstance(const ContextCreateInfo& info);

    void pickPhysicalDevice(const ContextCreateInfo& info, const vk::SurfaceKHR& surface);

    void initDevice(const ContextCreateInfo& info);

    bool checkDeviceExtensionSupport(const ContextCreateInfo& info, std::vector<vk::ExtensionProperties>& extensionProperties);

    operator vk::Device() const { return m_device; }

public:
    vk::Instance       m_instance;
    vk::Device         m_device;
    vk::PhysicalDevice m_physicalDevice;

    struct Queue
    {
        vk::Queue queue;
        uint32_t familyIndex = ~0;

        operator vk::Queue() const { return queue; }
        operator uint32_t() const { return familyIndex; }
    };

    Queue m_queueGraphics;  // for Graphics/Compute/Transfer (must exist)
    Queue m_queuePresent;

    Queue m_queueT;  // for pure async Transfer Queue (can exist, only contains transfer nothing else)
    Queue m_queueC;  // for async Compute (can exist, may contain other non-graphics support)

private:
    //////////////////
    // Debug System //
    //////////////////
    bool checkValidationLayerSupport(const ContextCreateInfo& info); 
    
    void initDebugMessenger(bool& enableValidationLayers);
    void destroyDebugUtilsMessengerEXT(vk::Instance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator);
    VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

    VkDebugUtilsMessengerEXT m_debugMessenger = nullptr;

}; // class Context

} // namespace app