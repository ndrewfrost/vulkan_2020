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

#include "../external/imgui/imgui.h"
#include "../external/imgui/imgui_impl_glfw.h"
#include "../external/imgui/imgui_impl_vulkan.h"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "vulkanbackend.hpp"
#include "commands.hpp"
#include "camera.h"
#include "examplevulkan.hpp"

static int  g_winWidth      = 800;
static int  g_winHeight     = 600;
static bool g_resizeRequest = false;

static vk::DescriptorPool g_imguiDescPool;

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
static void onScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse)
        return;

    CameraView.wheel(static_cast<int>(yOffset));
}

//--------------------------------------------------------------------------------------------------
// onKeyCallback
//
static void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse)
        return;

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
    if (ImGui::GetCurrentContext() != nullptr) {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureKeyboard || io.WantCaptureMouse) return;
    }
    
    tools::Camera::Inputs inputs;
    inputs.lmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)   == GLFW_PRESS;
    inputs.mmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    inputs.rmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT)  == GLFW_PRESS;
    if (!inputs.lmb && !inputs.rmb && !inputs.mmb) {
        return;  // no mouse button pressed
    }

    inputs.ctrl  = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
    inputs.shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)   == GLFW_PRESS;
    inputs.alt   = glfwGetKey(window, GLFW_KEY_LEFT_ALT)     == GLFW_PRESS;

    CameraView.mouseMove(static_cast<int>(mouseX), static_cast<int>(mouseY), inputs);
}

//--------------------------------------------------------------------------------------------------
// onMouseButtonCallback
//
static void onMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse)
        return;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    CameraView.setMousePosition(static_cast<int>(xpos), static_cast<int>(ypos));
}

//--------------------------------------------------------------------------------------------------
// onResizeCallback
//
static void onResizeCallback(GLFWwindow* window, int w, int h)
{
    (void)(window);
    CameraView.setWindowSize(w, h);
    g_resizeRequest = true;
    g_winWidth      = w;
    g_winHeight     = h;
}

///////////////////////////////////////////////////////////////////////////
// IMGUI  
///////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
// 
//
static void checkVkResult(VkResult err)
{
    if (err == 0)
        return;
    printf("VkResult %d\n", err);
    if (err < 0)
        abort();
}

//--------------------------------------------------------------------------------------------------
// Setup ImGUI
//
static void setupImGUI(app::VulkanBackend& vkBackend, GLFWwindow* window)
{
    // create pool onfo descriptor used by ImGUI
    std::vector<vk::DescriptorPoolSize> counters{ {vk::DescriptorType::eCombinedImageSampler, 2} };

    vk::DescriptorPoolCreateInfo poolInfo = {};
    poolInfo.poolSizeCount = static_cast<uint32_t>(counters.size());
    poolInfo.pPoolSizes    = counters.data();
    poolInfo.maxSets       = static_cast<uint32_t>(counters.size());

    vk::Device device(vkBackend.getDevice());
    g_imguiDescPool = device.createDescriptorPool(poolInfo);

    // Setup Dear ImGUI binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo imGuiInitInfo = {};
    imGuiInitInfo.Allocator       = nullptr;
    imGuiInitInfo.DescriptorPool  = g_imguiDescPool;
    imGuiInitInfo.Device          = vkBackend.getDevice();
    imGuiInitInfo.ImageCount      = (uint32_t)vkBackend.getFramebuffers().size();
    imGuiInitInfo.Instance        = vkBackend.getInstance();
    imGuiInitInfo.MinImageCount   = (uint32_t)vkBackend.getFramebuffers().size();
    imGuiInitInfo.PhysicalDevice  = vkBackend.getPhysicalDevice();
    imGuiInitInfo.PipelineCache   = vkBackend.getPipelineCache();
    imGuiInitInfo.Queue           = vkBackend.getGraphicsQueue();
    imGuiInitInfo.QueueFamily     = vkBackend.getGraphicsQueueIdx();
    imGuiInitInfo.CheckVkResultFn = checkVkResult;

    ImGui_ImplVulkan_Init(&imGuiInitInfo, vkBackend.getRenderPass());

    // Setup style
    ImGui::StyleColorsDark();

    // Upload Fonts
    app::SingleCommandBuffer cmdBufferGen(vkBackend.getDevice(), vkBackend.getGraphicsQueueIdx());

    auto cmdBuffer = cmdBufferGen.createCommandBuffer();
    ImGui_ImplVulkan_CreateFontsTexture(cmdBuffer);
    cmdBufferGen.flushCommandBuffer(cmdBuffer);

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

//--------------------------------------------------------------------------------------------------
// Destroy ImGUI
//
static void destroyImGUI(const vk::Device& device)
{
    if (ImGui::GetCurrentContext() != nullptr) {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        device.destroyDescriptorPool(g_imguiDescPool);
    }
}

//--------------------------------------------------------------------------------------------------
// Render UI
//
static void renderUI()
{
    ImGui::Text("Hello, world %d", 123);
    if (ImGui::Button("Save"))
    {
        // do stuff  
    }

    float value = 2.f;
    ImGui::SliderFloat("Test Slider", &value, 0, 100);
}

///////////////////////////////////////////////////////////////////////////
// Application 
///////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
// Application
//
void application() 
{
    glfwSetErrorCallback(onErrorCallback);

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow(g_winWidth, g_winHeight, "Vulkan", nullptr, nullptr);

    glfwSetScrollCallback(window, onScrollCallback);
    glfwSetKeyCallback(window, onKeyCallback);
    glfwSetCursorPosCallback(window, onMouseMoveCallback);
    glfwSetMouseButtonCallback(window, onMouseButtonCallback);
    glfwSetFramebufferSizeCallback(window, onResizeCallback);

    // Setup Camera
    CameraView.setWindowSize(g_winWidth, g_winHeight);
    CameraView.setLookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    // check Vulkan Support
    if (!glfwVulkanSupported())
        throw std::runtime_error("GLFW; Vulkan not supported");

    // Create Vulkan Base
    app::ContextCreateInfo contextInfo = {};
    contextInfo.addInstanceExtension(VK_KHR_SURFACE_EXTENSION_NAME);
    contextInfo.addInstanceExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    contextInfo.addInstanceExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    contextInfo.addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    contextInfo.addDeviceExtension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);

    app::VulkanBackend vkBackend;
    vkBackend.setupVulkan(contextInfo, window);

    // Imgui
    //setupImGUI(vkBackend, window);

    // Vulkan
    ExampleVulkan vkExample;
    vkExample.init(vkBackend.getDevice(), vkBackend.getPhysicalDevice(), 
                   vkBackend.getInstance(), vkBackend.getGraphicsQueueIdx(), 
                   vkBackend.getPresentQueueIdx(), vkBackend.getSize());

    vkExample.loadModel("../media/scenes/cube_multi.obj");

    vkExample.createOffscreenRender();
    vkExample.createDescriptorSetLayout();
    vkExample.createGraphicsPipeline(vkBackend.getRenderPass());
    vkExample.createUniformBuffer();
    vkExample.createSceneDescriptionBuffer();
    vkExample.updateDescriptorSet();

    vkExample.createPostDescriptor();
    vkExample.createPostPipeline(vkBackend.getRenderPass());
    vkExample.updatePostDescriptorSet();
    glm::vec4 clearColor = glm::vec4(1, 1, 1, 1.00f);
    
    // Main Loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        if (g_resizeRequest) {
            vkBackend.onWindowResize(g_winWidth, g_winHeight);
            vkExample.resize(vkBackend.getSize());
            g_resizeRequest = false;
        }


        // ImGui Frame
        /*
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            renderUI();

            ImGui::Render();
        }
        */

        // Render the scene
        //vkBackend.prepareFrame();
        //auto                     currentFrame = vkBackend.getCurrentFrame();
        //const vk::CommandBuffer& cmdBuffer    = vkBackend.getCommandBuffers()[currentFrame];

        //cmdBuffer.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

        

        // Begin render pass

        // rendering scene

        // rendering UI
        //ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);

        //cmdBuffer.end();
        //vkBackend.submitFrame();
    }

    // Cleanup
    vkBackend.getDevice().waitIdle();
    //destroyImGUI(vkBackend.getDevice());

    vkExample.destroy();
    vkBackend.destroy();

    glfwDestroyWindow(window);
    glfwTerminate();
}

//--------------------------------------------------------------------------------------------------
//  Main / Entry Point
//
int main(int argc, char* argv[]) 
{
    (void)(argc), (void)(argv);

    try {
        application();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}