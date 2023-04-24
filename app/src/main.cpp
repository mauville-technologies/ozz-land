//
// Created by Paul Mauviel on 2023-04-06.
//
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <spdlog/spdlog.h>
#include <ozz_vulkan/renderer.h>
#include <vector>

#define XR_USE_GRAPHICS_API_VULKAN
#define VMA_IMPLEMENTATION

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <vk_mem_alloc.h>

// Goals:
// Initialize OpenXR, and clear color the HMD using Vulkan

// Vulkan Entities
VkInstance vkInstance{VK_NULL_HANDLE};
VkPhysicalDevice vkPhysicalDevice{VK_NULL_HANDLE};
VkDevice vkDevice {VK_NULL_HANDLE};
VkDebugUtilsMessengerEXT vkDebugMessenger{VK_NULL_HANDLE};
VkQueue vkQueue;
VmaAllocator vmaAllocator;

uint32_t vkQueueFamilyIndex;

XrInstance xrInstance{XR_NULL_HANDLE};
XrSystemId xrSystemId{XR_NULL_SYSTEM_ID};
XrSession xrSession {XR_NULL_HANDLE};
XrSpace xrApplicationSpace {XR_NULL_HANDLE};

// Vulkan Functions
VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *createInfo,
                                        const VkAllocationCallbacks *allocator, VkDebugUtilsMessengerEXT *messenger) {
    auto pfnVkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                                                        "vkCreateDebugUtilsMessengerEXT");
    return pfnVkCreateDebugUtilsMessengerEXT(instance, createInfo, allocator, messenger);
}

void vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger,
                                     const VkAllocationCallbacks *allocator) {
    auto pfnVkDestroyDebugUtilsMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                                                       "vkDestroyDebugUtilsMessengerEXT");
    pfnVkDestroyDebugUtilsMessenger(instance, messenger, allocator);
}

unsigned int vulkanDebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
                                const VkDebugUtilsMessengerCallbackDataEXT *callbackData, void *userData) {
    spdlog::info("Vulkan Debug Message Severity: {}", severity);
    spdlog::info("Vulkan Debug Message: {}", callbackData->pMessage);
    return VK_TRUE;
}

// XR FUNCTIONS
XrResult xrGetVulkanGraphicsRequirements2KHR(XrInstance instance, XrSystemId systemId,
                                             XrGraphicsRequirementsVulkan2KHR *graphicsRequirements) {
    PFN_xrGetVulkanGraphicsRequirements2KHR pfnXrGetVulkanGraphicsRequirements2KHR = nullptr;
    xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsRequirements2KHR",
                          reinterpret_cast<PFN_xrVoidFunction *>(&pfnXrGetVulkanGraphicsRequirements2KHR));
    return pfnXrGetVulkanGraphicsRequirements2KHR(instance, systemId, graphicsRequirements);
}

XrResult xrCreateVulkanInstanceKHR(XrInstance instance, const XrVulkanInstanceCreateInfoKHR *createInfo,
                                   VkInstance *vulkanInstance, VkResult &vulkanResult) {
    PFN_xrCreateVulkanInstanceKHR pfnXrCreateVulkanInstanceKHR = nullptr;
    xrGetInstanceProcAddr(instance, "xrCreateVulkanInstanceKHR",
                          reinterpret_cast<PFN_xrVoidFunction *>(&pfnXrCreateVulkanInstanceKHR));
    return pfnXrCreateVulkanInstanceKHR(instance, createInfo, vulkanInstance, &vulkanResult);
}

XrResult xrGetVulkanGraphicsDevice2KHR(XrInstance instance, const XrVulkanGraphicsDeviceGetInfoKHR *getInfo,
                                       VkPhysicalDevice *vulkanPhysicalDevice) {
    PFN_xrGetVulkanGraphicsDevice2KHR pfnXrGetVulkanGraphicsDevice2KHR = nullptr;
    xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsDevice2KHR",
                          reinterpret_cast<PFN_xrVoidFunction *>(&pfnXrGetVulkanGraphicsDevice2KHR));
    return pfnXrGetVulkanGraphicsDevice2KHR(instance, getInfo, vulkanPhysicalDevice);
}

XrResult xrCreateVulkanDeviceKHR(XrInstance instance, const XrVulkanDeviceCreateInfoKHR *createInfo,
                                 VkDevice *vulkanDevice, VkResult *vulkanResult) {
    PFN_xrCreateVulkanDeviceKHR pfnXrCreateVulkanDeviceKHR = nullptr;
    xrGetInstanceProcAddr(instance, "xrCreateVulkanDeviceKHR",
                          reinterpret_cast<PFN_xrVoidFunction *>(&pfnXrCreateVulkanDeviceKHR));
    return pfnXrCreateVulkanDeviceKHR(instance, createInfo, vulkanDevice, vulkanResult);
}

void shutdown() {

    spdlog::info("Shutting down application.");

    // Destroy Vulkan Instance if exists
    if (vkInstance != VK_NULL_HANDLE) {
        vkDestroyInstance(vkInstance, nullptr);
        vkInstance = VK_NULL_HANDLE;
        spdlog::trace("Destroyed Vulkan Instance.");
    }

    // Destroy OpenXR Instance if exists
    if (xrInstance != XR_NULL_HANDLE) {
        xrDestroyInstance(xrInstance);
        xrInstance = XR_NULL_HANDLE;
        spdlog::trace("Destroyed OpenXR Instance.");
    }
}

int main(int argc, char *argv[]) {
    spdlog::set_level(spdlog::level::trace);

    spdlog::info("Initializing Application.");

    // Create OpenXR Instance

    spdlog::trace("Creating OpenXR Instance.");

    // create union of extensions required by platform and graphics plugins
    std::vector<const char *> extensions;

    // graphics extensions
    extensions.emplace_back(XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME);

    // Create XR instance
    XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
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
    spdlog::trace("Initializing OpenXR System.");

    XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    result = xrGetSystem(xrInstance, &systemInfo, &xrSystemId);

    if (XR_FAILED(result)) {
        spdlog::error("Failed to get OpenXR system {}", result);
        return 1;
    }

    spdlog::trace("Using system {}", xrSystemId);

    // Initialize vulkan instance
    spdlog::trace("Initializing Vulkan Instance.");

    XrGraphicsRequirementsVulkan2KHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR};
    result = xrGetVulkanGraphicsRequirements2KHR(xrInstance, xrSystemId, &graphicsRequirements);

    if (XR_FAILED(result)) {
        spdlog::error("Failed to get OpenXR Vulkan Graphics Requirements {}", result);
        return 1;
    }

    spdlog::trace("Vulkan Min API Version {}", graphicsRequirements.minApiVersionSupported);
    spdlog::trace("Vulkan Max API Version {}", graphicsRequirements.maxApiVersionSupported);

    // Get validation layers
    std::vector<const char *> validationLayers;
#if !defined(NDEBUG)
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    spdlog::trace("Available Vulkan Validation Layers:");
    for (const auto &layer: availableLayers) {
        spdlog::trace("{}", layer.layerName);
    }

    std::vector<const char *> desiredLayers;
    desiredLayers.push_back("VK_LAYER_KHRONOS_validation");
    desiredLayers.push_back("VK_LAYER_LUNARG_standard_validation");

    // enable only one validation layer from the list above. Prefer KHRONOS over LUNARG
    for (const auto &desiredLayer: desiredLayers) {
        for (const auto &availableLayer: availableLayers) {
            if (strcmp(desiredLayer, availableLayer.layerName) == 0) {
                validationLayers.push_back(desiredLayer);
                break;
            }
        }
    }

    // print selected validation layers
    spdlog::trace("Selected Vulkan Validation Layers:");
    for (const auto &layer: validationLayers) {
        spdlog::trace("{}", layer);
    }
#endif

    // get vulkan instance extensions
    std::vector<const char *> vulkanInstanceExtensions;

    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

    auto isExtSupported = [&](const char *extName) -> bool {
        for (const auto &availableExtension: availableExtensions) {
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
    VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = "Hello OpenXR";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "OZZ_LAND";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instanceCreateInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(vulkanInstanceExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = vulkanInstanceExtensions.empty() ? nullptr
                                                                                  : vulkanInstanceExtensions.data();
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = validationLayers.empty() ? nullptr : validationLayers.data();

    XrVulkanInstanceCreateInfoKHR xrVulkanInstanceCreateInfoKhr{XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR};
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

    // Create vulkan debug messenger
    spdlog::trace("Creating Vulkan Debug Messenger");

    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoExt{
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debugUtilsMessengerCreateInfoExt.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugUtilsMessengerCreateInfoExt.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugUtilsMessengerCreateInfoExt.pfnUserCallback = vulkanDebugMessage;
    debugUtilsMessengerCreateInfoExt.pUserData = nullptr;

    // Create debug utils messenger
    vkResult = vkCreateDebugUtilsMessengerEXT(vkInstance, &debugUtilsMessengerCreateInfoExt, nullptr,
                                              &vkDebugMessenger);

    if (vkResult != VK_SUCCESS) {
        spdlog::error("Failed to create Vulkan Debug Messenger {}", vkResult);
        return 1;
    } else {
        spdlog::trace("Created Vulkan Debug Messenger");
    }

    // Get Vulkan Physical Device
    spdlog::trace("Getting Vulkan Physical Device");

    XrVulkanGraphicsDeviceGetInfoKHR deviceGetInfoKHR{XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR};
    deviceGetInfoKHR.systemId = xrSystemId;
    deviceGetInfoKHR.vulkanInstance = vkInstance;

    result = xrGetVulkanGraphicsDevice2KHR(xrInstance, &deviceGetInfoKHR, &vkPhysicalDevice);

    if (XR_FAILED(result)) {
        spdlog::error("Failed to get OpenXR Vulkan Graphics Device {}", result);
        return 1;
    } else {
        spdlog::trace("Got OpenXR Vulkan Graphics Device");
    }

    // Get the vulkan graphics queue
    spdlog::trace("Getting Vulkan Graphics Queue");

    VkDeviceQueueCreateInfo deviceQueueCreateInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    float queuePriorities = 0;
    deviceQueueCreateInfo.queueCount = 1;
    deviceQueueCreateInfo.pQueuePriorities = &queuePriorities;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, queueFamilyProperties.data());

    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        // Only need graphics and not presentation queue
        if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0u) {
            vkQueueFamilyIndex = deviceQueueCreateInfo.queueFamilyIndex = i;
            break;
        }
    }

    // Get device extensions and features
    spdlog::trace("Getting Vulkan Device Extensions and Features");
    std::vector<const char *> deviceExtensions = {};
    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo deviceCreateInfo { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
    deviceCreateInfo.enabledLayerCount = 0;
    deviceCreateInfo.ppEnabledLayerNames = nullptr;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.empty() ? nullptr : deviceExtensions.data();
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    // create xr vulkan device
    spdlog::trace("Creating XR Vulkan Device");

    XrVulkanDeviceCreateInfoKHR xrVulkanDeviceCreateInfoKhr { XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR };
    xrVulkanDeviceCreateInfoKhr.systemId = xrSystemId;
    xrVulkanDeviceCreateInfoKhr.vulkanPhysicalDevice = vkPhysicalDevice;
    xrVulkanDeviceCreateInfoKhr.vulkanCreateInfo = &deviceCreateInfo;
    xrVulkanDeviceCreateInfoKhr.vulkanAllocator = nullptr;
    xrVulkanDeviceCreateInfoKhr.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;

    result = xrCreateVulkanDeviceKHR(xrInstance, &xrVulkanDeviceCreateInfoKhr, &vkDevice, &vkResult);

    if (XR_FAILED(result)) {
        spdlog::error("Failed to create OpenXR Vulkan Device {}", result);
    } else {
        spdlog::trace("Created OpenXR Vulkan Device");
    }

    if (vkResult != VK_SUCCESS) {
        spdlog::error("Failed to create Vulkan Device {}", vkResult);
    } else {
        spdlog::trace("Created Vulkan Device");
    }

    // Get the vulkan graphics queue
    vkGetDeviceQueue(vkDevice, vkQueueFamilyIndex, 0, &vkQueue);

    // initialize vma allocator
    VmaAllocatorCreateInfo vmaAllocatorCreateInfo{};
    vmaAllocatorCreateInfo.physicalDevice = vkPhysicalDevice;
    vmaAllocatorCreateInfo.device = vkDevice;
    vmaAllocatorCreateInfo.instance = vkInstance;

    vkResult = vmaCreateAllocator(&vmaAllocatorCreateInfo, &vmaAllocator);

    // check if successful
    if (vkResult != VK_SUCCESS) {
        spdlog::error("Failed to create VMA Allocator {}", vkResult);
    } else {
        spdlog::trace("Created VMA Allocator");
    }

    // Create XR Session
    spdlog::trace("Creating XR Session");

    XrGraphicsBindingVulkan2KHR graphicsBinding { XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR };
    graphicsBinding.instance = vkInstance;
    graphicsBinding.physicalDevice = vkPhysicalDevice;
    graphicsBinding.device = vkDevice;
    graphicsBinding.queueFamilyIndex = vkQueueFamilyIndex;
    graphicsBinding.queueIndex = 0;

    XrSessionCreateInfo sessionCreateInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionCreateInfo.systemId = xrSystemId;
    sessionCreateInfo.next = &graphicsBinding;
    result = xrCreateSession(xrInstance, &sessionCreateInfo, &xrSession);

    if (XR_FAILED(result)) {
        spdlog::error("Failed to create OpenXR Session {}", result);
        return 1;
    } else {
        spdlog::trace("Created OpenXR Session");
    }

    // Create Reference Space
    spdlog::trace("Creating XR Reference Space");

    XrReferenceSpaceCreateInfo referenceSpaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    referenceSpaceCreateInfo.poseInReferenceSpace = { {0, 0, 0, 1 }, { 0, 0, 0 } };

    result = xrCreateReferenceSpace(xrSession, &referenceSpaceCreateInfo, &xrApplicationSpace);

    if (XR_FAILED(result)) {
        spdlog::error("Failed to create OpenXR Reference Space {}", result);
        return 1;
    } else {
        spdlog::trace("Created OpenXR Reference Space");
    }

    // TODO: Next step: Create swapchains. Ref: hello_xr main.cpp L331 & openxr_program.cpp L572

    shutdown();
}

