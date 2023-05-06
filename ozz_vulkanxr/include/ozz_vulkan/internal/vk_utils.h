//
// Created by ozzadar on 02/05/23.
//

#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <spdlog/spdlog.h>

namespace OZZ {
    static VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &code) {
        VkShaderModuleCreateInfo createInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shaderModule;
        if (auto err = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            spdlog::error("Failed to create shader module: {}", err);
        }

        return shaderModule;
    }

    static int64_t SelectColorSwapchainFormat(const std::vector<int64_t> &runtimeFormats) {
        // List of supported color swapchain formats.
        constexpr int64_t SupportedColorSwapchainFormats[] = {VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB,
                                                              VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM};

        auto swapchainFormatIt =
                std::find_first_of(runtimeFormats.begin(), runtimeFormats.end(),
                                   std::begin(SupportedColorSwapchainFormats),
                                   std::end(SupportedColorSwapchainFormats));
        if (swapchainFormatIt == runtimeFormats.end()) {
            spdlog::error("No runtime swapchain format supported for color swapchain");
        }

        return *swapchainFormatIt;
    }

    static VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *createInfo,
                                            const VkAllocationCallbacks *allocator,
                                            VkDebugUtilsMessengerEXT *messenger) {
        auto pfnVkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                                                            "vkCreateDebugUtilsMessengerEXT");
        return pfnVkCreateDebugUtilsMessengerEXT(instance, createInfo, allocator, messenger);
    }

    static void vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger,
                                         const VkAllocationCallbacks *allocator) {
        auto pfnVkDestroyDebugUtilsMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                                                           "vkDestroyDebugUtilsMessengerEXT");
        pfnVkDestroyDebugUtilsMessenger(instance, messenger, allocator);
    }
}
