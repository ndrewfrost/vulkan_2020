/*
 *
 * Andrew Frost
 * renderbackend.cpp
 * 2020
 *
 */

#include "renderbackend.h"

vulkanContext_t vkContext;

/*
===========================================================================
RenderBackend
===========================================================================
*/

/*
===============
RenderBackend::run
===============
*/
void RenderBackend::run() {
    initWindow();
    initVulkan();
    renderLoop();
    cleanup();
}

/*
===============
RenderBackend::initWindow
===============
*/
void RenderBackend::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

/*
===============
RenderBackend::initVulkan
===============
*/
void RenderBackend::initVulkan() {
    createInstance();
    
    debug::setupDebugMessenger(*instance, enableValidationLayers);
    
    createSurface();
    
    pickPhysicalDevice();
    
    createLogicalDeviceAndQueues();

    createSyncObjects();

    createCommandPool();

    createCommandBuffer();

    /*
    Allocator.init();

    stagingManager.init();
    */
    
    createSwapChain();

    createRenderTargets();

    createRenderPass();

    createPipelineCache();

    createFrameBuffers();

    /*
    renderProgManager.init();

    vertexCache.init(vkcontext.gpu->props.limits.minUniformBufferOffsetAlignment );
    */    
}

/*
===============
RenderBackend::renderLoop
===============
*/
void RenderBackend::renderLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

/*
===============
RenderBackend::cleanupSwapchain
===============
*/
void RenderBackend::cleanupSwapchain() {

    for (auto framebuffer : swapchainFrameBuffers) {
        vkContext.device.destroyFramebuffer(framebuffer);
    }

    vkContext.device.freeCommandBuffers(commandPool, commandBuffers);

    vkContext.device.destroyPipelineCache(vkContext.pipelineCache);
    vkContext.device.destroyRenderPass(vkContext.renderPass);

    for (auto imageView : swapchainViews) {
        vkContext.device.destroyImageView(imageView);
    }

    vkContext.device.destroySwapchainKHR(swapchain);
}

/*
===============
RenderBackend::cleanup
===============
*/
void RenderBackend::cleanup() {

    cleanupSwapchain();

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkContext.device.destroySemaphore(renderFinishedSemaphores[i]);
        vkContext.device.destroySemaphore(imageAvailableSemaphores[i]);
        vkContext.device.destroyFence(inFlightFences[i]);
    }

    vkContext.device.destroyCommandPool(commandPool);

    vkDestroyDevice(vkContext.device, nullptr);

    instance->destroySurfaceKHR(surface);
    
    if (enableValidationLayers) {
        debug::DestroyDebugUtilsMessengerEXT(*instance, debug::debugMessenger, nullptr);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}

/*
===============
RenderBackend::getRequiredExtensions
===============
*/
std::vector<const char*> RenderBackend::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);


    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

/*
===============
RenderBackend::createInstance
===============
*/
void RenderBackend::createInstance() {
    if (enableValidationLayers && !debug::checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    vk::ApplicationInfo appInfo(
            "Hello Triangle",
            VK_MAKE_VERSION(1, 0, 0),
            "No Engine",
            VK_MAKE_VERSION(1, 0, 0),
            VK_API_VERSION_1_0);
    
    auto extensions = getRequiredExtensions();
    vk::InstanceCreateInfo createInfo(
        vk::InstanceCreateFlags(), 
        &appInfo, 
        0, nullptr,
        static_cast<uint32_t>(extensions.size()),
        extensions.data());
    
    for (auto e : extensions) {
        std::cout << e << std::endl;

    }

    if (enableValidationLayers) {
        createInfo.setEnabledLayerCount(static_cast<uint32_t>(debug::validationLayers.size()));
        createInfo.setPpEnabledLayerNames(debug::validationLayers.data());
        /*vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo(
            vk::DebugUtilsMessengerCreateFlagsEXT(),
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            debug::debugCallback);

        createInfo.setPNext((vk::DebugUtilsMessengerCreateInfoEXT*)& debugCreateInfo);
    }
    else {
        createInfo.setEnabledExtensionCount(0);
        createInfo.setPNext(nullptr);*/
    }

    try {
        instance = vk::createInstanceUnique(createInfo, nullptr);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create instance!");
    }
}

/*
===============
RenderBackend::createSurface
===============
*/
void RenderBackend::createSurface() {
    VkSurfaceKHR rawSurface;
    if (glfwCreateWindowSurface(*instance, window, nullptr, &rawSurface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }   
    surface = vk::SurfaceKHR(rawSurface);
}

/*
===============
RenderBackend::pickPhysicalDevice
===============
*/
void RenderBackend::pickPhysicalDevice() {

    std::vector<vk::PhysicalDevice> devices = instance->enumeratePhysicalDevices();
    if (devices.size() == 0)
        throw std::runtime_error("failed to find GPUs with Vulkan support!");

    // Fill in GPU Info for each GPU
    std::vector<GPUInfo_t> gpus(devices.size());
    for (uint32_t i = 0; i < devices.size(); ++i) {
        GPUInfo_t& gpu = gpus[i];
        gpu.device = devices[i];  

        gpu.queueFamilyProps = gpu.device.getQueueFamilyProperties();       
        gpu.extensionProps = gpu.device.enumerateDeviceExtensionProperties();
        gpu.surfaceCaps = gpu.device.getSurfaceCapabilitiesKHR(surface);
        gpu.surfaceFormats = gpu.device.getSurfaceFormatsKHR(surface);
        gpu.presentModes = gpu.device.getSurfacePresentModesKHR(surface);
        gpu.memoryProps = gpu.device.getMemoryProperties();
        gpu.props = gpu.device.getProperties();
        gpu.features = gpu.device.getFeatures();
    }

    // Pick GPU
    for (uint32_t i = 0; i < gpus.size(); ++i) {
        GPUInfo_t& gpu = gpus[i];

        uint32_t graphicsIdx = -1;
        uint32_t presentIdx = -1;

        if (!checkDeviceExtensionSupport(gpu)) continue;
        if (gpu.surfaceFormats.size() == 0) continue;
        if (gpu.presentModes.size() == 0) continue;

        // graphics queue family
        for (uint32_t j = 0; j < gpu.queueFamilyProps.size(); ++j) {
            vk::QueueFamilyProperties& queueFamily = gpu.queueFamilyProps[j];

            if (queueFamily.queueCount == 0) continue;

            if (queueFamily.queueFlags & vk::QueueFlagBits::eCompute) {
                graphicsIdx = j;
            }
        }

        // present queue family
        for (uint32_t j = 0; j < gpu.queueFamilyProps.size(); ++j) {
            vk::QueueFamilyProperties& queueFamily = gpu.queueFamilyProps[j];

            if (queueFamily.queueCount == 0) continue;

            vk::Bool32 supportsPresent = VK_FALSE;
            supportsPresent = gpu.device.getSurfaceSupportKHR(j, surface);
            if (supportsPresent) {
                presentIdx = j;
                break;
            }
        }

        if (graphicsIdx >= 0 && presentIdx >= 0) {
            physicalDevice = gpu.device;

            vkContext.graphicsFamilyIdx = graphicsIdx;
            vkContext.presentFamilyIdx = presentIdx;
            vkContext.gpu = gpu;
            vkContext.sampleCount = getMaxUsableSampleCount();
            vkContext.depthFormat = findDepthFormat();

            return;
        }
    }

    // Could not find a suitable device
    if (!physicalDevice) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

/*
===============
RenderBackend::checkDeviceExtensionSupport
===============
*/
bool RenderBackend::checkDeviceExtensionSupport(GPUInfo_t& gpu) {
    uint32_t required = numDeviceExtensions;

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : gpu.extensionProps) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

/*
===============
RenderBackend::getMaxUsableSampleCount
===============
*/
vk::SampleCountFlagBits RenderBackend::getMaxUsableSampleCount() {
    VkPhysicalDeviceProperties physicalDeviceProperties = vkContext.gpu.device.getProperties();
    VkSampleCountFlags rawCounts = std::min(physicalDeviceProperties.limits.framebufferColorSampleCounts, physicalDeviceProperties.limits.framebufferDepthSampleCounts);
    vk::SampleCountFlags counts(rawCounts);

    if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
    if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
    if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
    if (counts & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
    if (counts & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
    if (counts & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }

    return vk::SampleCountFlagBits::e1;
}

/*
===============
RenderBackend::findDepthFormat
===============
*/
vk::Format RenderBackend::findDepthFormat() {
    return findSupportedFormat(
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}

/*
===============
RenderBackend::findSupportedFormat
===============
*/
vk::Format RenderBackend::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
    for (vk::Format format : candidates) {
        vk::FormatProperties props = vkContext.gpu.device.getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

/*
===============
RenderBackend::createLogicalDeviceAndQueues
===============
*/
void RenderBackend::createLogicalDeviceAndQueues() {
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {vkContext.graphicsFamilyIdx, vkContext.presentFamilyIdx};

    const float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo(
            vk::DeviceQueueCreateFlags(),
            queueFamily,
            1,
            &queuePriority);
        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::PhysicalDeviceFeatures deviceFeatures = vk::PhysicalDeviceFeatures();
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    vk::DeviceCreateInfo createInfo(
        {},
        static_cast<uint32_t>(queueCreateInfos.size()),
        queueCreateInfos.data(),
        0, nullptr,
        numDeviceExtensions,
        deviceExtensions.data(),
        &deviceFeatures);

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(debug::validationLayers.size());
        createInfo.ppEnabledLayerNames = debug::validationLayers.data();
    }

    try {
        vkContext.device = physicalDevice.createDevice(createInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkContext.graphicsQueue = vkContext.device.getQueue(vkContext.graphicsFamilyIdx, 0);
    vkContext.presentQueue = vkContext.device.getQueue(vkContext.presentFamilyIdx, 0);
}

/*
===============
RenderBackend::createSyncObjects
===============
*/
void RenderBackend::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    try {
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {        
            imageAvailableSemaphores[i] = vkContext.device.createSemaphore({});
            renderFinishedSemaphores[i] = vkContext.device.createSemaphore({});
            inFlightFences[i] = vkContext.device.createFence({ vk::FenceCreateFlagBits::eSignaled });
        }
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create synchronization objects for a frame!");
    }
}

/*
===============
RenderBackend::createCommandPool
===============
*/
void RenderBackend::createCommandPool() {
    vk::CommandPoolCreateInfo poolInfo;
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    poolInfo.queueFamilyIndex = vkContext.graphicsFamilyIdx;        

    try {
        commandPool = vkContext.device.createCommandPool(poolInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create command pool!");
    }
}

/*
===============
RenderBackend::createCommandBuffer
===============
*/
void RenderBackend::createCommandBuffer() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    vk::CommandBufferAllocateInfo allocInfo(
        commandPool, 
        vk::CommandBufferLevel::ePrimary,
        MAX_FRAMES_IN_FLIGHT);

    try {
        commandBuffers = vkContext.device.allocateCommandBuffers(allocInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

/*
===============
RenderBackend::createSwapChain
===============
*/
void RenderBackend::createSwapChain() {
    GPUInfo_t& gpu = vkContext.gpu;

    vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat();
    vk::PresentModeKHR presentMode = chooseSwapPresentMode();
    vk::Extent2D extent = chooseSwapExtent();

    uint32_t imageCount = gpu.surfaceCaps.minImageCount + 1;
    if (gpu.surfaceCaps.maxImageCount > 0 && imageCount > gpu.surfaceCaps.maxImageCount) {
        imageCount = gpu.surfaceCaps.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo(
        vk::SwapchainCreateFlagsKHR(),
        surface,
        imageCount,
        surfaceFormat.format,
        surfaceFormat.colorSpace,
        extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment);

    if (vkContext.graphicsFamilyIdx != vkContext.presentFamilyIdx) {
        uint32_t indices[] = { (uint32_t)vkContext.graphicsFamilyIdx, (uint32_t)vkContext.presentFamilyIdx };

        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = indices;
    }
    else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    createInfo.preTransform = gpu.surfaceCaps.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    try {
        swapchain = vkContext.device.createSwapchainKHR(createInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create swap chain!");
    }

    swapchainImages = vkContext.device.getSwapchainImagesKHR(swapchain);

    this->swapchainFormat = surfaceFormat.format;
    this->presentMode = presentMode;
    this->swapchainExtent = extent;
}

/*
===============
RenderBackend::chooseSwapSurfaceFormat
===============
*/
vk::SurfaceFormatKHR RenderBackend::chooseSwapSurfaceFormat() {
    for (const auto& availableFormat : vkContext.gpu.surfaceFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Unorm && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }

    return vkContext.gpu.surfaceFormats[0];
}

/*
===============
RenderBackend::chooseSwapPresentMode
===============
*/
vk::PresentModeKHR RenderBackend::chooseSwapPresentMode() {
    vk::PresentModeKHR bestMode = vk::PresentModeKHR::eFifo;

    for (const auto& availablePresentMode : vkContext.gpu.presentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
        else if (availablePresentMode == vk::PresentModeKHR::eImmediate) {
            bestMode = availablePresentMode;
        }
    }
    return bestMode;
}

/*
===============
RenderBackend::chooseSwapExtent
===============
*/
vk::Extent2D RenderBackend::chooseSwapExtent() {
    if (vkContext.gpu.surfaceCaps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return vkContext.gpu.surfaceCaps.currentExtent;
    }
    else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        vk::Extent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::max(
            vkContext.gpu.surfaceCaps.minImageExtent.width,
            std::min(
                vkContext.gpu.surfaceCaps.maxImageExtent.width,
                actualExtent.width));
        actualExtent.height = std::max(
            vkContext.gpu.surfaceCaps.minImageExtent.height,
            std::min(
                vkContext.gpu.surfaceCaps.maxImageExtent.height,
                actualExtent.height));

        return actualExtent;
    }
}

/*
===============
RenderBackend::createImageViews
===============
*/
void RenderBackend::createImageViews() {

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vk::ImageViewCreateInfo imageViewCreateInfo({},
            swapchainImages[i],
            vk::ImageViewType::e2D,
            swapchainFormat,
            vk::ComponentMapping(vk::ComponentSwizzle::eR, 
                                 vk::ComponentSwizzle::eG,
                                 vk::ComponentSwizzle::eB,
                                 vk::ComponentSwizzle::eA),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 
                                      0,   // base Mip Level
                                      1,   // level Count
                                      0,   // base Array Layer
                                      1)); // layer count

        try {
            swapchainViews[i] = vkContext.device.createImageView(imageViewCreateInfo);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to create image views!");
        }

    }
}

/*
===============
RenderBackend::createRenderPass
===============
*/
void RenderBackend::createRenderPass() {

    const bool resolve = vkContext.sampleCount > vk::SampleCountFlagBits::e1;

    vk::AttachmentDescription colorAttachment({},
        swapchainFormat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal);

    vk::AttachmentDescription depthAttachment({},
        vkContext.depthFormat,
        vkContext.sampleCount,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::AttachmentDescription resolveAttachment({},
        swapchainFormat,
        vkContext.sampleCount,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference colorRef(resolve ? 2 : 0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference depthRef(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    vk::AttachmentReference resolveRef(0, vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription subpass = {};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;
    if(resolve)
        subpass.pResolveAttachments = &resolveRef;

    vk::SubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    //dependency.srcAccessMask = 0;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;


    std::array<vk::AttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, resolveAttachment };

    vk::RenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.attachmentCount = resolve ? 3 : 2;
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    try {
        vkContext.renderPass = vkContext.device.createRenderPass(renderPassInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create render pass!");
    }
}

/*
===============
RenderBackend::createPipelineCache
===============
*/
void RenderBackend::createPipelineCache() {
    vk::PipelineCacheCreateInfo pipelineCacheInfo = {};
    try {
        vkContext.pipelineCache = vkContext.device.createPipelineCache(pipelineCacheInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create pipeline cache!");
    }
}

/*
===============
RenderBackend::createFrameBuffers
===============
*/
void RenderBackend::createFrameBuffers() {
    swapchainFrameBuffers.resize(swapchainViews.size());

    for (uint32_t i = 0; i < swapchainViews.size(); i++) {
        vk::ImageView attachments[] = {
            swapchainViews[i]};
        

        vk::FramebufferCreateInfo framebufferInfo({},
            vkContext.renderPass,
            1,
            attachments,
            swapchainExtent.width,
            swapchainExtent.height,
            1);

        try {
            swapchainFrameBuffers[i] = vkContext.device.createFramebuffer(framebufferInfo);
        }
        catch (vk::SystemError err) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }    
}

/*
===============
RenderBackend::
===============

void RenderBackend:: {
}
*/