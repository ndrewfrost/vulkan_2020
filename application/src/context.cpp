/*
 *
 * Andrew Frost
 * context.cpp
 * 2020
 *
 */

#include "context.hpp"

// See: https://github.com/KhronosGroup/Vulkan-Hpp#extensions--per-device-function-pointers
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;
static_assert(VK_HEADER_VERSION >= 126, "Vulkan version need 1.1.126.0 or greater");

namespace app {

///////////////////////////////////////////////////////////////////////////
// Context
///////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
// Create Vulkan Instance
//
void Context::initInstance(const ContextCreateInfo& info)
{
    if (info.enableValidationLayers && !checkValidationLayerSupport(info)) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    vk::ApplicationInfo appInfo = {};
    appInfo.pApplicationName = info.appTitle;
    appInfo.pEngineName      = info.appEngine;
    appInfo.apiVersion       = VK_API_VERSION_1_0;

    vk::InstanceCreateInfo createInfo = {};
    createInfo.pApplicationInfo        = &appInfo;
    createInfo.enabledExtensionCount   = info.numInstanceExtensions;
    createInfo.ppEnabledExtensionNames = info.instanceExtensions.data();
    
    if (info.enableValidationLayers) {
        createInfo.setEnabledLayerCount(info.numValidationLayers);
        createInfo.setPpEnabledLayerNames(info.validationLayers.data());
    }
    
try {
    m_instance = vk::createInstance(createInfo);
}
catch (vk::SystemError err) {
    throw std::runtime_error("failed to create instance!");
}
}

//--------------------------------------------------------------------------------------------------
// pick Physical Device
//
void Context::pickPhysicalDevice(const ContextCreateInfo& info, const vk::SurfaceKHR& surface) {
    std::vector<vk::PhysicalDevice> devices = m_instance.enumeratePhysicalDevices();
    if (devices.size() == 0)
        throw std::runtime_error("failed to find GPUs with Vulkan support!");

    // Find a GPU
    for (auto device : devices) {
        uint32_t graphicsIdx = -1;
        uint32_t presentIdx = -1;

        auto queueFamilyProperties     = device.getQueueFamilyProperties();
        auto deviceExtensionProperties = device.enumerateDeviceExtensionProperties();

        //if (m_physicalDevice.getSurfaceFormatsKHR(surface).size() == 0) continue;
        //if (m_physicalDevice.getSurfaceFormatsKHR(surface).size() == 0) continue;
        if (!checkDeviceExtensionSupport(info, deviceExtensionProperties))
            continue;

        // supports graphics and compute?
        for (uint32_t j = 0; j < queueFamilyProperties.size(); ++j) {
            vk::QueueFamilyProperties& queueFamily = queueFamilyProperties[j];
            
            if (queueFamily.queueCount == 0) continue;

            if (queueFamily.queueFlags
                & (vk::QueueFlagBits::eGraphics
                    | vk::QueueFlagBits::eCompute
                    | vk::QueueFlagBits::eTransfer)) {
                graphicsIdx = j;
            }
        }

        // present queue 
        for (uint32_t j = 0; j < queueFamilyProperties.size(); ++j) {
            vk::QueueFamilyProperties& queueFamily = queueFamilyProperties[j];

            if (queueFamily.queueCount == 0) continue;

            vk::Bool32 supportsPresent = VK_FALSE;
            supportsPresent = device.getSurfaceSupportKHR(j, surface);
            if (supportsPresent) {
                presentIdx = j;
                break;
            }
        }

        if (graphicsIdx >= 0 && presentIdx >= 0) {
            m_physicalDevice = device;            
            m_queuePresent.familyIndex = presentIdx;
            m_queueGraphics.familyIndex = graphicsIdx;
            return;
        }
    }

    // Could not find suitable device
    if (!m_physicalDevice) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

//--------------------------------------------------------------------------------------------------
// Create Vulkan Device
//
void Context::initDevice(const ContextCreateInfo& info)
{
    auto queueFamilyProperties = m_physicalDevice.getQueueFamilyProperties();

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { m_queueGraphics.familyIndex,  m_queuePresent.familyIndex };
    
    const float queuePriority = 1.0f;    
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueInfo = {};
        queueInfo.queueFamilyIndex          = queueFamily;
        queueInfo.queueCount                = 1;
        queueInfo.pQueuePriorities          = &queuePriority;

        queueCreateInfos.push_back(queueInfo);
    }

    // Vulkan >= 1.1 uses pNext to enable features, and not pEnabledFeatures
    vk::PhysicalDeviceFeatures2 enabledFeatures2 = {};
    enabledFeatures2.features = m_physicalDevice.getFeatures();
    //m_physicalDevice.getFeatures2(&enabledFeatures2);
    enabledFeatures2.features.samplerAnisotropy = VK_TRUE;

    vk::DeviceCreateInfo deviceCreateInfo    = {};
    deviceCreateInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos       = queueCreateInfos.data();
    deviceCreateInfo.enabledExtensionCount   = info.numDeviceExtensions;
    deviceCreateInfo.ppEnabledExtensionNames = info.deviceExtensions.data();
    deviceCreateInfo.pEnabledFeatures        = nullptr;
    deviceCreateInfo.pNext                   = &enabledFeatures2;

    if (info.enableValidationLayers) {
        deviceCreateInfo.enabledLayerCount   = info.numValidationLayers;
        deviceCreateInfo.ppEnabledLayerNames = info.validationLayers.data();
    }

    try {
        m_device = m_physicalDevice.createDevice(deviceCreateInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create logical device!");
    }

    m_queueGraphics.queue = m_device.getQueue(m_queueGraphics.familyIndex, 0);
    m_queuePresent.queue = m_device.getQueue(m_queuePresent.familyIndex, 0);

    /*
    // get some default queues
    uint32_t queueFamilyIndex = 0;
    for (auto& queueFamilyProperty : queueFamilyProperties) {
        auto gtc =
            vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer;

        if ((queueFamilyProperty.queueFlags & gtc) == gtc){
            m_queueGCT.queue       = m_device.getQueue(queueFamilyIndex, 0);
            m_queueGCT.familyIndex = queueFamilyIndex;
        }
        else if ((queueFamilyProperty.queueFlags & vk::QueueFlagBits::eTransfer) == vk::QueueFlagBits::eTransfer) {
            m_queueT.queue       = m_device.getQueue(queueFamilyIndex, 0);
            m_queueT.familyIndex = queueFamilyIndex;
        }
        else if ((queueFamilyProperty.queueFlags & vk::QueueFlagBits::eCompute)) {
            m_queueC.queue       = m_device.getQueue(queueFamilyIndex, 0);
            m_queueC.familyIndex = queueFamilyIndex;
        }

        queueFamilyIndex++;
    }
    */
}


//--------------------------------------------------------------------------------------------------
// Destructor
//
void Context::deinit()
{
    if (m_device) {
        m_device.waitIdle();
        m_device.destroy();
        m_device = vk::Device();
    }
    
    if (m_debugMessenger)
        destroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);

    if (m_instance) {
        m_instance.destroy();
        m_instance = vk::Instance();
    }

    m_debugMessenger = nullptr;
}

//--------------------------------------------------------------------------------------------------
// check Device Extension Support
//
bool Context::checkDeviceExtensionSupport(const ContextCreateInfo& info, std::vector<vk::ExtensionProperties>& extensionProperties)
{
    std::set<std::string> requiredExtensions(info.deviceExtensions.begin(), info.deviceExtensions.end());

    for (const auto& extension : extensionProperties) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

//--------------------------------------------------------------------------------------------------
// check Validation Layer Support
//
bool Context::checkValidationLayerSupport(const ContextCreateInfo& info)
{
    std::vector<vk::LayerProperties> availableLayers =
        vk::enumerateInstanceLayerProperties();

    for (const char* layerName : info.validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
// 
//
static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
              void* pUserData)
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

//--------------------------------------------------------------------------------------------------
// Set up Debug Messenger
//
void Context::initDebugMessenger(bool& enableValidationLayers)
{
    if (!enableValidationLayers) return;

    vk::DebugUtilsMessengerCreateInfoEXT debugInfo = {};
    debugInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
                                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
                                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
    debugInfo.messageType     = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                                | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
                                | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    debugInfo.pfnUserCallback = debugCallback;

    try {
        createDebugUtilsMessengerEXT(m_instance, reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&debugInfo), nullptr, &m_debugMessenger);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to set up debug messenger!");
    }    
}

//--------------------------------------------------------------------------------------------------
// 
//
void Context::destroyDebugUtilsMessengerEXT(vk::Instance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator) 
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    }
}

//--------------------------------------------------------------------------------------------------
// 
//
VkResult Context::createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

///////////////////////////////////////////////////////////////////////////
// ContextCreateInfo
///////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
// 
//
ContextCreateInfo::ContextCreateInfo()
{
    numDeviceExtensions   = 0;
    deviceExtensions      = std::vector<const char*>();

    numValidationLayers   = 0;
    validationLayers      = std::vector<const char*>();

    numInstanceExtensions = 0;
    instanceExtensions    = std::vector<const char*>();

    if (enableValidationLayers) {
        numValidationLayers++;
        validationLayers.emplace_back("VK_LAYER_KHRONOS_validation");

        numInstanceExtensions++;
        instanceExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        
    }
}

//--------------------------------------------------------------------------------------------------
// 
//
void ContextCreateInfo::addDeviceExtension(const char* name)
{
    numDeviceExtensions++;
    deviceExtensions.emplace_back(name);
}

//--------------------------------------------------------------------------------------------------
// 
//
void ContextCreateInfo::addInstanceExtension(const char* name)
{
    numInstanceExtensions++;
    instanceExtensions.emplace_back(name);
}

//--------------------------------------------------------------------------------------------------
// 
//
void ContextCreateInfo::addValidationLayer(const char* name)
{
    numValidationLayers++;
    deviceExtensions.emplace_back(name);
}

} // namespace app