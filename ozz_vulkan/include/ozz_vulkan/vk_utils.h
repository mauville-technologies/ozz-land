//
// Created by ozzadar on 02/05/23.
//

#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <spdlog/spdlog.h>

VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code) {
   VkShaderModuleCreateInfo createInfo { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        spdlog::error("Failed to create shader module");
    }

    return shaderModule;
}