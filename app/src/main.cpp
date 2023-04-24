//
// Created by Paul Mauviel on 2023-04-06.
//
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <spdlog/spdlog.h>
#include <ozz_vulkan/renderer.h>
#include <vector>

#define XR_USE_GRAPHICS_API_VULKAN

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

// Goals:
// Initialize OpenXR, and clear color the HMD using Vulkan

// Vulkan Entities
VkInstance vkInstance { VK_NULL_HANDLE };

XrInstance xrInstance { XR_NULL_HANDLE };
XrSystemId xrSystemId { XR_NULL_SYSTEM_ID };

XrResult xrGetVulkanGraphicsRequirements2KHR(XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsVulkan2KHR* graphicsRequirements) {
    PFN_xrGetVulkanGraphicsRequirements2KHR pfnXrGetVulkanGraphicsRequirements2KHR = nullptr;
    xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsRequirements2KHR",
                          reinterpret_cast<PFN_xrVoidFunction*>(&pfnXrGetVulkanGraphicsRequirements2KHR));
    return pfnXrGetVulkanGraphicsRequirements2KHR(instance, systemId, graphicsRequirements);
}

XrResult xrCreateVulkanInstanceKHR(XrInstance instance, const XrVulkanInstanceCreateInfoKHR* createInfo, VkInstance* vulkanInstance, VkResult& vulkanResult) {
    PFN_xrCreateVulkanInstanceKHR pfnXrCreateVulkanInstanceKHR = nullptr;
    xrGetInstanceProcAddr(instance, "xrCreateVulkanInstanceKHR",
                          reinterpret_cast<PFN_xrVoidFunction*>(&pfnXrCreateVulkanInstanceKHR));
    return pfnXrCreateVulkanInstanceKHR(instance, createInfo, vulkanInstance, &vulkanResult);
}

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::trace);

    spdlog::info("Initializing Application.");

//    OZZ_VULKAN::Renderer myRenderer {};
//    myRenderer.Init();

    // Create OpenXR Instance

    // create union of extensions required by platform and graphics plugins
    std::vector<const char*> extensions;

    // graphics extensions
    extensions.emplace_back(XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME);

    // Create XR instance
    XrInstanceCreateInfo createInfo { XR_TYPE_INSTANCE_CREATE_INFO };
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.enabledExtensionNames = extensions.data();
    strcpy(createInfo.applicationInfo.applicationName, "Hello OpenXR");
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

    XrResult result = xrCreateInstance(&createInfo, &xrInstance);
    if (XR_FAILED(result)) {
        spdlog::error("Failed to create OpenXR instance {}", result);
        return 1;
    }

    // Initialize System
    XrSystemGetInfo systemInfo { XR_TYPE_SYSTEM_GET_INFO };
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    result = xrGetSystem(xrInstance, &systemInfo, &xrSystemId);

    if (XR_FAILED(result)) {
        spdlog::error("Failed to get OpenXR system {}", result);
        return 1;
    }

    spdlog::trace("Using system {}", xrSystemId);

    // Initialize Vulkan Device
    XrGraphicsRequirementsVulkan2KHR graphicsRequirements { XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR };
    result = xrGetVulkanGraphicsRequirements2KHR(xrInstance, xrSystemId, &graphicsRequirements);

    if (XR_FAILED(result)) {
        spdlog::error("Failed to get OpenXR Vulkan Graphics Requirements {}", result);
        return 1;
    }

    spdlog::trace("Vulkan Min API Version {}", graphicsRequirements.minApiVersionSupported);
    spdlog::trace("Vulkan Max API Version {}", graphicsRequirements.maxApiVersionSupported);

    // Get validation layers
    std::vector<const char*> validationLayers;
#if !defined(NDEBUG)
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    spdlog::trace("Available Vulkan Validation Layers:");
    for (const auto& layer : availableLayers) {
        spdlog::trace("{}", layer.layerName);
    }

    std::vector<const char*> desiredLayers;
    desiredLayers.push_back("VK_LAYER_KHRONOS_validation");
    desiredLayers.push_back("VK_LAYER_LUNARG_standard_validation");

    // enable only one validation layer from the list above. Prefer KHRONOS over LUNARG
    for (const auto& desiredLayer : desiredLayers) {
        for (const auto& availableLayer : availableLayers) {
            if (strcmp(desiredLayer, availableLayer.layerName) == 0) {
                validationLayers.push_back(desiredLayer);
                break;
            }
        }
    }

    // print selected validation layers
    spdlog::trace("Selected Vulkan Validation Layers:");
    for (const auto& layer : validationLayers) {
        spdlog::trace("{}", layer);
    }
#endif

    // get vulkan instance extensions
    std::vector<const char*> vulkanInstanceExtensions;

    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

    auto isExtSupported = [&](const char* extName) -> bool {
        for (const auto& availableExtension : availableExtensions) {
            if (strcmp(extName, availableExtension.extensionName) == 0) {
                return true;
            }
        }
        return false;
    };

    if (isExtSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
        vulkanInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // Create vulkan instance
    VkApplicationInfo appInfo { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.pApplicationName = "Hello OpenXR";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "OZZ_LAND";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0 , 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instanceCreateInfo { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(vulkanInstanceExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = vulkanInstanceExtensions.empty() ? nullptr : vulkanInstanceExtensions.data();
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = validationLayers.empty() ? nullptr : validationLayers.data();

    XrVulkanInstanceCreateInfoKHR xrVulkanInstanceCreateInfoKhr { XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR };
    xrVulkanInstanceCreateInfoKhr.systemId = xrSystemId;
    xrVulkanInstanceCreateInfoKhr.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
    xrVulkanInstanceCreateInfoKhr.vulkanCreateInfo = &instanceCreateInfo;
    xrVulkanInstanceCreateInfoKhr.vulkanAllocator = nullptr;

    VkResult vkResult;
    result = xrCreateVulkanInstanceKHR(xrInstance, &xrVulkanInstanceCreateInfoKhr, &vkInstance, vkResult);

    if (XR_FAILED(result)) {
        spdlog::error("Failed to create OpenXR Vulkan Instance {}", result);
        return 1;
    } else {
        spdlog::trace("Created OpenXR Vulkan Instance");
    }

    if (vkResult != VK_SUCCESS) {
        spdlog::error("Failed to create Vulkan Instance {}", vkResult);
        return 1;
    } else {
        spdlog::trace("Created Vulkan Instance");
    }
}
