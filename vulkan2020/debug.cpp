#include "debug.h"
/*
 *
 * Andrew Frost
 * debug.cpp
 * 2020
 *
 */

 /*
 ===========================================================================
 Debug
 ===========================================================================
 */
namespace debug {

    VkDebugUtilsMessengerEXT debugMessenger;

    std::vector<const char*> validationLayers = {
         "VK_LAYER_KHRONOS_validation"
    };

    /*
    ===============
    debug::checkValidationLayerSupport
    ===============
    */
    bool checkValidationLayerSupport() {
        std::vector<vk::LayerProperties> availableLayers =
            vk::enumerateInstanceLayerProperties();

        for (const char* layerName : validationLayers) {
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

    /*
    ===============
    debug::DestroyDebugUtilsMessengerEXT
    ===============
    */
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, callback, pAllocator);
        }
    }

    /*
    ===============
    debug::CreateDebugUtilsMessengerEXT
    ===============
    */
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }
    
    /*
    ===============
    debug::setupDebugMessenger
    ===============
    */
    void setupDebugMessenger(vk::Instance instance, bool enableValidationLayers) {
        if (!enableValidationLayers) return;

        vk::DebugUtilsMessengerCreateInfoEXT createInfo( 
            vk::DebugUtilsMessengerCreateFlagsEXT(),
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            debugCallback);
        
        if (CreateDebugUtilsMessengerEXT(instance, reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&createInfo), nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    /*
    ===============
    debug::debugCallback
    ===============
    */
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }


}