/*
 *
 * Andrew Frost
 * debug.h
 * 2020
 *
 */

#pragma once

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "header.h"
#include "renderbackend.h"

namespace debug {

    extern VkDebugUtilsMessengerEXT debugMessenger;
    extern std::vector<const char*> validationLayers;
    
    bool checkValidationLayerSupport();

    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator);

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

    void setupDebugMessenger(vk::Instance instance, bool enableValidationLayers);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

}

#endif