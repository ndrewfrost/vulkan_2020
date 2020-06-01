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

#include "../external/imgui/imgui.h"
#include "../external/imgui/imgui_impl_glfw.h"
#include "../external/imgui/imgui_impl_vulkan.h"

#include <array>
#include <vulkan/vulkan.hpp>
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE


#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include "glm/gtx/transform.hpp"

#include "../vk_helpers/vulkanbackend.hpp"
#include "../vk_helpers/commands.hpp"
#include "../general_helpers/manipulator.h"
#include "../vk_helpers/utilities.hpp"
#include "examplevulkan.hpp"

static int  g_winWidth      = 800;
static int  g_winHeight     = 600;

//-------------------------------------------------------------------------
// GLFW on Error Callback
//
static void onErrorCallback(int error, const char* description)
{
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

//-------------------------------------------------------------------------
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
// Application                                                           //
///////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------
// Application
//
void application() 
{
    glfwSetErrorCallback(onErrorCallback);
    if (!glfwInit()) return;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow(g_winWidth, g_winHeight, "Vulkan", nullptr, nullptr);
    
    // Setup Camera
    CameraManipulator.setWindowSize(g_winWidth, g_winHeight);
    CameraManipulator.setLookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    // check Vulkan Support
    if (!glfwVulkanSupported())
        throw std::runtime_error("GLFW; Vulkan not supported");

    // Create Vulkan Base
    app::ContextCreateInfo contextInfo = {};
    contextInfo.addInstanceExtension(VK_KHR_SURFACE_EXTENSION_NAME);
    contextInfo.addInstanceExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    contextInfo.addInstanceExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    contextInfo.addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    contextInfo.addDeviceExtension(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
    contextInfo.addDeviceExtension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
    contextInfo.addDeviceExtension(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
    contextInfo.addDeviceExtension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
    contextInfo.addDeviceExtension(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);

    // Vulkan
    ExampleVulkan vkExample;
    vkExample.setupVulkan(contextInfo, window);

    // Imgui 
    vkExample.initGUI(window);

    vkExample.loadModel(".../../media/scenes/cube_multi.obj");
    vkExample.createOffscreenRender();
    vkExample.createDescriptorSetLayout();
    vkExample.createGraphicsPipeline();
    vkExample.createUniformBuffer();
    vkExample.createSceneDescriptionBuffer();
    vkExample.updateDescriptorSet();

    vkExample.createPostDescriptor();
    vkExample.createPostPipeline();
    vkExample.updatePostDescriptorSet();
    glm::vec4 clearColor = glm::vec4(1, 1, 1, 1.00f);

    vkExample.setupGlfwCallbacks(window);
    ImGui_ImplGlfw_InitForVulkan(window, true);
    
    // Main Loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        if (vkExample.isMinimized())
            continue;

        // Start ImGUI frame
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // update camera buffer
        vkExample.updateUniformBuffer();

        // Show UI window
        {
            ImGui::ColorEdit3("Clear color", reinterpret_cast<float*>(&clearColor));
            
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            
            renderUI();
            
            ImGui::Render();
        }

        // Start rendering the scene
        vkExample.prepareFrame();

        // Start command buffer of this frame
        auto                     currentFrame = vkExample.getCurrentFrame();
        const vk::CommandBuffer& cmdBuffer    = vkExample.getCommandBuffers()[currentFrame];

        cmdBuffer.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

        // clearing the screen
        vk::ClearValue clearValues[2];
        clearValues[0].setColor(app::util::clearColor(clearColor));
        clearValues[1].setDepthStencil({ 1.0f, 0 });

        // Offscreen render pass
        vk::RenderPassBeginInfo offscreenRenderPassBeginInfo = {};
        offscreenRenderPassBeginInfo.clearValueCount = 2;
        offscreenRenderPassBeginInfo.pClearValues    = clearValues;
        offscreenRenderPassBeginInfo.renderPass      = vkExample.m_offscreenRenderPass;
        offscreenRenderPassBeginInfo.framebuffer     = vkExample.m_offscreenFramebuffer;
        offscreenRenderPassBeginInfo.renderArea      = vk::Rect2D({}, vkExample.getSize());

        // Rendering the scene
        cmdBuffer.beginRenderPass(offscreenRenderPassBeginInfo, vk::SubpassContents::eInline);
        vkExample.rasterize(cmdBuffer);
        cmdBuffer.endRenderPass();

        // 2nd Render Pass : tone mapper, UI
        vk::RenderPassBeginInfo postRenderPassBeginInfo = {};
        postRenderPassBeginInfo.clearValueCount = 2;
        postRenderPassBeginInfo.pClearValues    = clearValues;
        postRenderPassBeginInfo.renderPass      = vkExample.getRenderPass();
        postRenderPassBeginInfo.framebuffer     = vkExample.getFramebuffers()[currentFrame];
        postRenderPassBeginInfo.renderArea      = vk::Rect2D({}, vkExample.getSize());

        cmdBuffer.beginRenderPass(postRenderPassBeginInfo, vk::SubpassContents::eInline);

        // Rendering tonemapper
        vkExample.drawPost(cmdBuffer);

        // Rendering UI
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer); 
        cmdBuffer.endRenderPass();

        // Submit for Display
        cmdBuffer.end();
        vkExample.submitFrame();
    }

    // Cleanup
    vkExample.getDevice().waitIdle();
    vkExample.destroyResources();
    vkExample.destroy();

    glfwDestroyWindow(window);
    glfwTerminate();
}

//-------------------------------------------------------------------------
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