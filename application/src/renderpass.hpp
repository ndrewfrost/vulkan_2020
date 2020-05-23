/*
 *
 * Andrew Frost
 * renderpass.hpp
 * 2020
 *
 */

#pragma once

#include <vulkan/vulkan.hpp>

namespace app
{
namespace util
{

inline vk::RenderPass createRenderPass(
    const                          vk::Device& device,
    const std::vector<vk::Format>& colorAttachmentFormats,
    vk::Format                     depthAttachmentFormat,
    uint32_t                       subpassCount = 1,
    bool                           clearColor = true,
    bool                           clearDepth = true,
    vk::ImageLayout                initialLayout = vk::ImageLayout::eUndefined,
    vk::ImageLayout                finalLayout = vk::ImageLayout::ePresentSrcKHR)
{
    std::vector<vk::AttachmentDescription> allAttachments;
    std::vector<vk::AttachmentReference>   colorAttachmentRefs;

    bool hasDepth = depthAttachmentFormat != vk::Format::eUndefined;

    for (const auto& format : colorAttachmentFormats) {
        vk::AttachmentDescription colorAttachment = {};
        colorAttachment.format        = format;
        colorAttachment.loadOp        = clearColor ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare;
        colorAttachment.initialLayout = initialLayout;
        colorAttachment.finalLayout   = finalLayout;

        vk::AttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = static_cast<uint32_t>(allAttachments.size());
        colorAttachmentRef.layout     = vk::ImageLayout::eColorAttachmentOptimal;

        allAttachments.push_back(colorAttachment);
        colorAttachmentRefs.push_back(colorAttachmentRef);
    }

    vk::AttachmentReference depthAttachmentRef;

    if (hasDepth) {
        vk::AttachmentDescription depthAttachment = {};
        depthAttachment.format        = depthAttachmentFormat;
        depthAttachment.loadOp        = clearDepth ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
        depthAttachment.initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        depthAttachment.finalLayout   = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        depthAttachmentRef.attachment = static_cast<uint32_t>(allAttachments.size());
        depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        allAttachments.push_back(depthAttachment);
    }

    std::vector<vk::SubpassDescription> subpasses;
    std::vector<vk::SubpassDependency>  subpassDependencies;

    for (uint32_t i = 0; i < subpassCount; i++) {
        vk::SubpassDescription subpass = {};
        subpass.pipelineBindPoint       = vk::PipelineBindPoint::eGraphics;
        subpass.colorAttachmentCount    = static_cast<uint32_t>(colorAttachmentRefs.size());
        subpass.pColorAttachments       = colorAttachmentRefs.data();
        subpass.pDepthStencilAttachment = hasDepth ? &depthAttachmentRef : VK_NULL_HANDLE;

        vk::SubpassDependency dependency = {};
        dependency.srcSubpass    = i == 0 ? (VK_SUBPASS_EXTERNAL) : (i - 1);
        dependency.dstSubpass    = i;
        dependency.srcStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.srcAccessMask = vk::AccessFlags();
        dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        ;

        subpasses.push_back(subpass);
        subpassDependencies.push_back(dependency);
    }

    vk::RenderPassCreateInfo renderPassInfo;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(allAttachments.size());
    renderPassInfo.pAttachments = allAttachments.data();
    renderPassInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
    renderPassInfo.pSubpasses = subpasses.data();
    renderPassInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
    renderPassInfo.pDependencies = subpassDependencies.data();

    vk::RenderPass renderPass;
    try {
        renderPass = device.createRenderPass(renderPassInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create RenderPass!");
    }
    return renderPass;
}

} // namespace util
} // namespace app