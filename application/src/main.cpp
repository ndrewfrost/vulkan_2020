/*
 *
 * Andrew Frost
 * main.cpp
 * 2020
 *
 */

#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <iostream>
#include <array>

#include <vulkan/vulkan.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "vulkanbase.hpp"
#include "camera.h"
#include "examplevulkan.h"

static int g_winWidth  = 800;
static int g_winHeight = 600;
static bool g_resizeRequest = false;

///////////////////////////////////////////////////////////////////////////
// GLFW Callback functions   
///////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
// onErrorCallback
//
static void onErrorCallback(int error, const char* description) 
{
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

//--------------------------------------------------------------------------------------------------
// onScrollCallback
//
static void onScrollCallback(GLFWwindow * window, double xOffset, double yOffset) 
{

}

//--------------------------------------------------------------------------------------------------
// onKeyCallback
//
static void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) 
{
    if (action == GLFW_RELEASE) 
        return; // No action on key up

    if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q)
        glfwSetWindowShouldClose(window, 1);
}

//--------------------------------------------------------------------------------------------------
// onMouseMoveCallback
//
static void onMouseMoveCallback(GLFWwindow* window, double mouseX, double mouseY) 
{

}

//--------------------------------------------------------------------------------------------------
// onMouseButtonCallback
//
static void onMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) 
{

}

//--------------------------------------------------------------------------------------------------
// onResizeCallback
//
static void onResizeCallback(GLFWwindow* window, int w, int h) 
{

}

///////////////////////////////////////////////////////////////////////////
// IMGUI  
///////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
// 
//
static void setupImGUI(app::VulkanBase& vulkanBase, GLFWwindow* window)
{

}

//--------------------------------------------------------------------------------------------------
// 
//
static void destroyImGUI(const vk::Device& device)
{

}
//--------------------------------------------------------------------------------------------------
// 
//
static void renderUI(/*ExampleVulkan& exampleVK*/)
{

}

///////////////////////////////////////////////////////////////////////////
// Application Entry  
///////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) 
{
    void(argc), void(argv);

    try {
        // Setup Window
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        GLFWwindow* window = glfwCreateWindow(g_winWidth, g_winHeight, "Vulkan", nullptr, nullptr);

        glfwSetScrollCallback(window, onScrollCallback);
        glfwSetKeyCallback(window, onKeyCallback);
        glfwSetCursorPosCallback(window, onMouseMoveCallback);
        glfwSetMouseButtonCallback(window, onMouseButtonCallback);
        glfwSetFramebufferSizeCallback(window, onResizeCallback);

        // Setup Camera

        // Setup Vulkan 
        if (!glfwVulkanSupported())
        {
            std::cout << "GLFW; Vulkan not supported" << std::endl;
        }

        // Create Vulkan Base Application
        app::ContextCreateInfo contextInfo = {};
        contextInfo.addInstanceExtension(VK_KHR_SURFACE_EXTENSION_NAME);
        contextInfo.addInstanceExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
        contextInfo.addInstanceExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        contextInfo.addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);        
        contextInfo.addDeviceExtension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);

        // Base Vulkan application
        app::VulkanBase vulkanBase;
        vulkanBase.setupVulkan(contextInfo, window);
        vulkanBase.createSurface(g_winWidth, g_winHeight);
        vulkanBase.createDepthBuffer();
        vulkanBase.createRenderPass();
        vulkanBase.createFrameBuffers();

        // Imgui
        setupImGUI(vulkanBase, window);

        // Nvidea Example
        ExampleVulkan exampleVulkan;
        exampleVulkan.init(vulkanBase.getDevice(), vulkanBase.getPhysicalDevice(), 
                           vulkanBase.getGraphicsQueueFamily(), vulkanBase.getSize());
               
        // main loop
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            if (g_resizeRequest) {
                vulkanBase.onWindowResize(g_winWidth, g_winHeight);
                g_resizeRequest = false;
            }

        }

        // cleanup
        vulkanBase.getDevice().waitIdle();
        destroyImGUI(vulkanBase.getDevice());

        vulkanBase.destroy();

        glfwDestroyWindow(window);
        glfwTerminate();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}