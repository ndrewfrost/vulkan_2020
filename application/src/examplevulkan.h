/*
 *
 * Andrew Frost
 * examplevulkan.h
 * 2020
 *
 */

#pragma once

class ExampleVulkan
{
public:

    void init(const vk::Device& device, 
              const vk::PhysicalDevice& physicalDevice,
              uint32_t graphicsFamilyIdx,
              uint32_t presentFamilyIdx,
              const vk::Extent2D& size);

}; // class ExampleVulkan