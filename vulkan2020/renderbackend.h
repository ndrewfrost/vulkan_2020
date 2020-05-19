/*
 *
 * Andrew Frost
 * renderbackend.h
 * 2020
 *
 */

#pragma once 

#ifndef __RENDERER_BACKEND_H__
#define __RENDERER_BACKEND_H__

#include "header.h"
#include "debug.h"

struct GPUInfo_t {
    vk::PhysicalDevice                     device;
    vk::PhysicalDeviceProperties           props;
    vk::PhysicalDeviceMemoryProperties     memoryProps;
    vk::PhysicalDeviceFeatures             features;
    vk::SurfaceCapabilitiesKHR             surfaceCaps;
    std::vector<vk::SurfaceFormatKHR>      surfaceFormats;
    std::vector<vk::PresentModeKHR>        presentModes;
    std::vector<vk::QueueFamilyProperties> queueFamilyProps;
    std::vector<vk::ExtensionProperties>   extensionProps;
};

struct vulkanContext_t {
    GPUInfo_t               gpu;

    vk::Device              device;
    uint32_t                graphicsFamilyIdx;
    uint32_t                presentFamilyIdx;
    vk::Queue               graphicsQueue;
    vk::Queue               presentQueue;

    vk::Format              depthFormat;
    vk::RenderPass          renderPass;
    vk::PipelineCache       pipelineCache;
    vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;
    bool                    superSampling;
};

extern vulkanContext_t vkContext;

/*
===========================================================================
RenderBackend
===========================================================================
*/

class RenderBackend {
public:
    RenderBackend() {};
    ~RenderBackend() {};

    void run();

    
private:
    void initWindow();

    void initVulkan();

    void renderLoop();

    void cleanupSwapchain();
    void cleanup();

    void createInstance();
    std::vector<const char*> getRequiredExtensions();

    void createSurface();
    
    void pickPhysicalDevice();
    bool checkDeviceExtensionSupport(GPUInfo_t& gpu);
    vk::SampleCountFlagBits getMaxUsableSampleCount();
    vk::Format findDepthFormat();
    vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);

    void createLogicalDeviceAndQueues();

    void createSyncObjects();

    void createCommandPool();

    void createCommandBuffer();

    void createSwapChain();
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat();
    vk::PresentModeKHR chooseSwapPresentMode();
    vk::Extent2D chooseSwapExtent();

    void createImageViews();

    void createRenderTargets();

    void createRenderPass();

    void createPipelineCache();

    void createFrameBuffers();

private:
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif
    GLFWwindow* window;
    const int WIDTH  = 800;
    const int HEIGHT = 600;

    struct Settings {
        bool enableFullscreen = true;
        bool enableVSync      = true;
        bool enableOverlay    = true;
    } settings;

    const int MAX_FRAMES_IN_FLIGHT = 2;

    const int numDeviceExtensions = 1;
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

private:
    vk::UniqueInstance             instance;
    vk::PhysicalDevice             physicalDevice;

    vk::SurfaceKHR                 surface;
    vk::PresentModeKHR             presentMode;
    
    vk::SwapchainKHR               swapchain;
    vk::Format			           swapchainFormat;
    vk::Extent2D		           swapchainExtent;
    uint32_t			           currentSwapIndex;

    vk::CommandPool                commandPool;

    std::vector<vk::Image>         swapchainImages;
    std::vector<vk::ImageView>     swapchainViews;
    std::vector<vk::Framebuffer>   swapchainFrameBuffers;

    std::vector<vk::CommandBuffer> commandBuffers;
    std::vector<bool>			   commandBufferRecorded;
    std::vector<vk::Semaphore>     imageAvailableSemaphores;
    std::vector<vk::Semaphore>     renderFinishedSemaphores;
    std::vector<vk::Fence>         inFlightFences;

};

#endif