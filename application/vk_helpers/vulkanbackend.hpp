/*
 *
 * Andrew Frost
 * vulkanbackend.hpp
 * 2020
 *
 */

#pragma once 

#include <set>
#include <vector>
#include <iostream>
#include <vulkan/vulkan.hpp>
#include <algorithm>
#include <cassert>
#include <regex>
#include <sstream>
#include <mutex>

#include "../external/imgui/imgui.h"
#include "../external/imgui/imgui_impl_vulkan.h"
#include "../external/imgui/imgui_impl_glfw.h"

#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

#include "swapchain.hpp"
#include "commands.hpp"
#include "../general_helpers/manipulator.h"

namespace app {

///////////////////////////////////////////////////////////////////////////
// ContextCreateInfo                                                     //
///////////////////////////////////////////////////////////////////////////
// Allows app to specify features that are expected of                   //
// - vk::Instance                                                        //
// - vk::Device                                                          //
///////////////////////////////////////////////////////////////////////////
struct ContextCreateInfo
{
    ContextCreateInfo();

    void addDeviceExtension(const char* name);

    void addInstanceExtension(const char* name);

    void addValidationLayer(const char* name);

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
    const char* appTitle = "Application";
};

///////////////////////////////////////////////////////////////////////////
// VulkanBackend                                                         //
///////////////////////////////////////////////////////////////////////////

class VulkanBackend
{
public:
    VulkanBackend()          = default;
    virtual ~VulkanBackend() = default;

    virtual void setupVulkan(const ContextCreateInfo& info, GLFWwindow* window);

    virtual void destroy();

    void initInstance(const ContextCreateInfo& info);

    void createSurface(GLFWwindow* window);

    void pickPhysicalDevice(const ContextCreateInfo& info);

    void createLogicalDeviceAndQueues(const ContextCreateInfo& info);

    void createSwapChain();

    void createCommandPool();

    void createCommandBuffer();

    virtual void createRenderPass();

    void createPipelineCache();

    void createColorBuffer();

    virtual void createDepthBuffer();

    virtual void createFrameBuffers();

    void createSyncObjects();
    
    void prepareFrame();

    void submitFrame();

    void setViewport(const vk::CommandBuffer& cmdBuffer);

    bool isMinimized(bool doSleeping = true);

    ///////////////////////////////////////////////////////////////////////////
    // GLFW Callbacks / ImGUI                                                //
    ///////////////////////////////////////////////////////////////////////////

    void setupGlfwCallbacks(GLFWwindow* window);

    virtual void onKeyboard(int key, int scancode, int action, int mods);
    static  void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    virtual void onKeyboardChar(unsigned int key);
    static  void onCharCallback(GLFWwindow* window, unsigned int key);

    virtual void onMouseMove(int x, int y);
    static  void onMouseMoveCallback(GLFWwindow* window, double x, double y);

    virtual void onMouseButton(int button, int action, int mod);
    static  void onMouseButtonCallback(GLFWwindow* window, int button, int action, int mod);

    virtual void onScroll(int delta);
    static  void onScrollCallback(GLFWwindow* window, double x, double y);

    virtual void onWindowResize(uint32_t width, uint32_t height);
    static  void onWindowSizeCallback(GLFWwindow* window, int w, int h);

    void initGUI();
    
    vk::DescriptorPool m_imguiDescPool;

    ///////////////////////////////////////////////////////////////////////////
    // Debug System Tools                                                    //
    ///////////////////////////////////////////////////////////////////////////

    void setupDebugMessenger(bool enabelValidationLayers);

    void destroyDebugUtilsMessengerEXT(vk::Instance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator);

    VkResult createDebugUtilsMessengerEXT(vk::Instance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

    bool checkValidationLayerSupport(const ContextCreateInfo& info);

    bool checkDeviceExtensionSupport(const ContextCreateInfo& info, std::vector<vk::ExtensionProperties>& extensionProperties);

    VkDebugUtilsMessengerEXT m_debugMessenger = nullptr;

    //-------------------------------------------------------------------------
    // Collection of Getter Methods
    //
    vk::Instance                          getInstance()         { return m_instance; }
    vk::Device                            getDevice()           { return m_device; }
    vk::PhysicalDevice                    getPhysicalDevice()   { return m_physicalDevice; }
    vk::Queue                             getGraphicsQueue()    { return m_graphicsQueue; }
    uint32_t                              getGraphicsQueueIdx() { return m_graphicsQueueIdx; }
    vk::Queue                             getPresentQueue()     { return m_presentQueue; }
    uint32_t                              getPresentQueueIdx()  { return m_presentQueueIdx; }
    vk::Extent2D                          getSize()             { return m_size; }
    vk::RenderPass                        getRenderPass()       { return m_renderPass; }
    vk::PipelineCache                     getPipelineCache()    { return m_pipelineCache; }
    const std::vector<vk::Framebuffer>&   getFramebuffers()     { return m_framebuffers; }
    const std::vector<vk::CommandBuffer>& getCommandBuffers()   { return m_commandBuffers; }
    uint32_t                              getCurrentFrame()     { return m_currentFrame; }
    vk::SampleCountFlagBits               getSampleCount()      { return m_sampleCount;  }
     
protected:
    vk::Instance                   m_instance;
    vk::PhysicalDevice             m_physicalDevice;
    vk::Device                     m_device;

    vk::SurfaceKHR                 m_surface;

    vk::Queue                      m_graphicsQueue;
    vk::Queue                      m_presentQueue;
    uint32_t                       m_graphicsQueueIdx{ VK_QUEUE_FAMILY_IGNORED };
    uint32_t                       m_presentQueueIdx{ VK_QUEUE_FAMILY_IGNORED };

    vk::CommandPool                m_commandPool;

    app::Swapchain                 m_swapchain;
    std::vector<vk::Framebuffer>   m_framebuffers;      // All framebuffers, correspond to the Swapchain
    std::vector<vk::CommandBuffer> m_commandBuffers;    // Command buffer per nb element in Swapchain

    vk::RenderPass                 m_renderPass;        // Base render pass
    vk::PipelineCache              m_pipelineCache;     // Cache for pipeline/shaders

    vk::Image                      m_depthImage;        // Depth/Stencil
    vk::DeviceMemory               m_depthMemory;       // Depth/Stencil
    vk::ImageView                  m_depthView;         // Depth/Stencil

    vk::SampleCountFlagBits        m_sampleCount{ vk::SampleCountFlagBits::e1 };
    vk::Image                      m_colorImage;     
    vk::DeviceMemory               m_colorMemory;      
    vk::ImageView                  m_colorView;        

    std::vector<vk::Fence>         m_fences;            // Fences per nb element in Swapchain
    vk::Semaphore                  m_imageAvailable;    // Swap chain image presentation
    vk::Semaphore                  m_renderFinished;    // Command buffer submission and execution

    vk::Extent2D                   m_size{ 0, 0 };      // Size of the window
    bool                           m_vsync{ false };    // Swapchain v-Sync
    GLFWwindow*                    m_window{ nullptr }; // GLFW Window
        
    uint32_t                       m_currentFrame{0};   // Current Frame in use

    // Surface buffer formats
    vk::Format                     m_colorFormat{ vk::Format::eB8G8R8A8Unorm };
    vk::Format                     m_depthFormat{ vk::Format::eUndefined };

}; // class VulkanBackend

} // namespace app