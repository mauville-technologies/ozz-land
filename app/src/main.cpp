//
// Created by Paul Mauviel on 2023-04-06.
//
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#define EYE_COUNT 2

#include <spdlog/spdlog.h>
#include <ozz_vulkan/renderer.h>
#include <vector>
#include <memory>

#define VMA_IMPLEMENTATION
#include "graphics_includes.h"
#include <vk_mem_alloc.h>

#include "swapchain_image.h"
#include "utils.h"
#include "ozz_vulkan/vk_utils.h"


// Program arguments
bool isRunning {true};

// Goals:
// Initialize OpenXR, and clear color the HMD using Vulkan

// Vulkan Entities
VkInstance vkInstance{VK_NULL_HANDLE};
VkPhysicalDevice vkPhysicalDevice{VK_NULL_HANDLE};
VkDevice vkDevice {VK_NULL_HANDLE};
VkDebugUtilsMessengerEXT vkDebugMessenger{VK_NULL_HANDLE};
VkQueue vkQueue;
VmaAllocator vmaAllocator;
std::vector<XrSwapchainImageVulkan2KHR> swapchainImages[EYE_COUNT];
std::vector<std::vector<SwapchainImage*>> wrappedSwapchainImages {};

VkRenderPass renderPass;
VkPipelineLayout pipelineLayout;
VkPipeline pipeline;
VkCommandPool commandPool;

/*
 * Stuff I need to add
 *
 * RenderPass
 * Command Pool
 * Descriptor Pool
 * Descriptor Set Layout
 *
 */
uint32_t vkQueueFamilyIndex;

bool xrSessionInitialized{false};
XrInstance xrInstance{XR_NULL_HANDLE};
XrSystemId xrSystemId{XR_NULL_SYSTEM_ID};
XrSession xrSession {XR_NULL_HANDLE};
XrSpace xrApplicationSpace {XR_NULL_HANDLE};
std::vector<XrViewConfigurationView> viewConfigurationViews;
std::vector<XrView> views;
int64_t swapchainColorFormat{-1};
std::vector<Swapchain> swapchains;

int64_t SelectColorSwapchainFormat(const std::vector<int64_t>& runtimeFormats) {
    // List of supported color swapchain formats.
    constexpr int64_t SupportedColorSwapchainFormats[] = {VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB,
                                                      VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM};

    auto swapchainFormatIt =
        std::find_first_of(runtimeFormats.begin(), runtimeFormats.end(), std::begin(SupportedColorSwapchainFormats),
                           std::end(SupportedColorSwapchainFormats));
    if (swapchainFormatIt == runtimeFormats.end()) {
        spdlog::error("No runtime swapchain format supported for color swapchain");
    }

    return *swapchainFormatIt;
}

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

void mainLoop();
void createRenderPass();
void createPipeline();

void shutdown() {

    spdlog::info("Shutting down application.");

    // Destroy OpenXR Swapchains
    for (auto& swapchain : swapchains) {
        xrDestroySwapchain(swapchain.handle);
        swapchain.handle = XR_NULL_HANDLE;
        spdlog::trace("Destroyed OpenXR Swapchain.");
    }

    swapchains.clear();

    // Clear OpenXR System Id
    xrSystemId = XR_NULL_SYSTEM_ID;

    // Destroy OpenXR Application Space if exists
    if (xrApplicationSpace != XR_NULL_HANDLE) {
        xrDestroySpace(xrApplicationSpace);
        xrApplicationSpace = XR_NULL_HANDLE;
        spdlog::trace("Destroyed OpenXR Application Space.");
    }

    // Destroy OpenXR Session if exists
    if (xrSession != XR_NULL_HANDLE) {
        xrDestroySession(xrSession);
        xrSession = XR_NULL_HANDLE;
        spdlog::trace("Destroyed OpenXR Session.");
    }

    // Destroy vma allocator if exists
    if (vmaAllocator != VK_NULL_HANDLE) {
        vmaDestroyAllocator(vmaAllocator);
        vmaAllocator = VK_NULL_HANDLE;
        spdlog::trace("Destroyed VMA Allocator.");
    }

    // Destroy Vulkan Device if exists
    if (vkDevice != VK_NULL_HANDLE) {
        vkDestroyDevice(vkDevice, nullptr);
        vkDevice = VK_NULL_HANDLE;
        spdlog::trace("Destroyed Vulkan Device.");
    }

    // destroy debug messenger if exists
    if (vkDebugMessenger != VK_NULL_HANDLE) {
        spdlog::trace("Destroying Vulkan Debug Messenger.");
        vkDestroyDebugUtilsMessengerEXT(vkInstance, vkDebugMessenger, nullptr);
        vkDebugMessenger = VK_NULL_HANDLE;
    }

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
    XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES};
    result = xrGetSystemProperties(xrInstance, xrSystemId, &systemProperties);

    if (XR_FAILED(result)) {
        spdlog::error("Failed to get OpenXR System Properties {}", result);
        return 1;
    } else {
        spdlog::trace("Got OpenXR System Properties");
    }

    // Log system properties
    spdlog::info("System Properties:");
    spdlog::info("System ID: {}", systemProperties.systemId);
    spdlog::info("Vendor ID: {}", systemProperties.vendorId);
    spdlog::info("Graphics Properties:");
    spdlog::info("Max Swapchain Image Height: {}", systemProperties.graphicsProperties.maxSwapchainImageHeight);
    spdlog::info("Max Swapchain Image Width: {}", systemProperties.graphicsProperties.maxSwapchainImageWidth);
    spdlog::info("Max Layer Count: {}", systemProperties.graphicsProperties.maxLayerCount);
    spdlog::info("System tracking properties:");
    spdlog::info("Orientation Tracking: {}", systemProperties.trackingProperties.orientationTracking);
    spdlog::info("Position Tracking: {}", systemProperties.trackingProperties.positionTracking);

    // Get view configuration views
    uint32_t viewCount;
    result = xrEnumerateViewConfigurationViews(xrInstance, xrSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, nullptr);

    if (XR_FAILED(result)) {
        spdlog::error("Failed to get OpenXR View Configuration count {}", result);
        return 1;
    }

    viewConfigurationViews.resize(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
    result = xrEnumerateViewConfigurationViews(xrInstance, xrSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, viewCount, &viewCount, viewConfigurationViews.data());

    if (XR_FAILED(result)) {
        spdlog::error("Failed to get OpenXR View Configuration Views {}", result);
        return 1;
    } else {
        spdlog::trace("Got OpenXR View Configuration Views");
    }

    views.resize(viewCount, {XR_TYPE_VIEW});

    // Create swapchains
    if (viewCount > 0) {
        uint32_t swapchainFormatCount;
        result = xrEnumerateSwapchainFormats(xrSession, 0, &swapchainFormatCount, nullptr);

        if (XR_FAILED(result)) {
            spdlog::error("Failed to get OpenXR Swapchain Format count {}", result);
            return 1;
        }
        std::vector<int64_t > swapchainFormats(swapchainFormatCount);
        result = xrEnumerateSwapchainFormats(xrSession, swapchainFormatCount, &swapchainFormatCount, swapchainFormats.data());

        if (XR_FAILED(result)) {
            spdlog::error("Failed to get OpenXR Swapchain Formats {}", result);
            return 1;
        } else {
            spdlog::trace("Got OpenXR Swapchain Formats");
        }

        swapchainColorFormat = SelectColorSwapchainFormat(swapchainFormats);

        // Print swapchain formats and selected one
        spdlog::info("Swapchain Formats:");
        for (auto format : swapchainFormats) {
            spdlog::info("{}", format);
        }
        spdlog::info("Selected Swapchain Format: {}", swapchainColorFormat);

        // Create a swapchain for each view
        for (uint32_t i = 0; i < viewCount; i++) {
            XrSwapchainCreateInfo swapchainCreateInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
            swapchainCreateInfo.arraySize = 1;
            swapchainCreateInfo.format = swapchainColorFormat;
            swapchainCreateInfo.width = viewConfigurationViews[i].recommendedImageRectWidth;
            swapchainCreateInfo.height = viewConfigurationViews[i].recommendedImageRectHeight;
            swapchainCreateInfo.mipCount = 1;
            swapchainCreateInfo.faceCount = 1;
            swapchainCreateInfo.sampleCount = viewConfigurationViews[i].recommendedSwapchainSampleCount;
            swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

            Swapchain swapchain;
            swapchain.width = static_cast<int> (swapchainCreateInfo.width);
            swapchain.height = static_cast<int> (swapchainCreateInfo.height);
            swapchain.format = swapchainCreateInfo.format;

            result = xrCreateSwapchain(xrSession, &swapchainCreateInfo, &swapchain.handle);

            if (XR_FAILED(result)) {
                spdlog::error("Failed to create OpenXR Swapchain {}", result);
                return 1;
            } else {
                spdlog::trace("Created OpenXR Swapchain");
            }

            swapchains.push_back(swapchain);

            // Get swapchain images
            uint32_t swapchainImageCount;
            result = xrEnumerateSwapchainImages(swapchain.handle, 0, &swapchainImageCount, nullptr);

            if (XR_FAILED(result)) {
                spdlog::error("Failed to get OpenXR Swapchain Image count {}", result);
                return 1;
            }

            swapchainImages[i].resize(swapchainImageCount, {XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR});
            result = xrEnumerateSwapchainImages(swapchain.handle,
                                                swapchainImageCount, &swapchainImageCount, reinterpret_cast<XrSwapchainImageBaseHeader *> (swapchainImages[i].data()));

            if (XR_FAILED(result)) {
                spdlog::error("Failed to get OpenXR Swapchain Images {}", result);
                return 1;
            } else {
                spdlog::trace("Got OpenXR Swapchain Images");
            }
        }
    }

    createRenderPass();
    // Create Pipeline
    createPipeline();

    VkCommandPoolCreateInfo commandPoolCreateInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = vkQueueFamilyIndex;

    vkResult = vkCreateCommandPool(vkDevice, &commandPoolCreateInfo, nullptr, &commandPool);

    if (vkResult != VK_SUCCESS) {
        spdlog::error("Failed to create Vulkan Command Pool {}", vkResult);
        return 1;
    } else {
        spdlog::trace("Created Vulkan Command Pool");
    }

    wrappedSwapchainImages.resize(EYE_COUNT);
    for (auto eye = 0; eye < EYE_COUNT; eye++) {
        wrappedSwapchainImages[eye] = std::vector<SwapchainImage*> {swapchainImages[eye].size() };
        for (auto i = 0; i < swapchainImages[eye].size(); i++) {
            wrappedSwapchainImages[eye][i] = new SwapchainImage (
                vkDevice,
                &swapchains[eye],
                swapchainImages[eye][i],
                renderPass,
                commandPool
            );
        }
    }
    mainLoop();

    spdlog::info("Exiting");
    shutdown();

    spdlog::shutdown();
}

void processEvents() {
    XrEventDataBuffer eventDataBuffer{XR_TYPE_EVENT_DATA_BUFFER};

    XrResult result = xrPollEvent(xrInstance, &eventDataBuffer);

    if (result == XR_EVENT_UNAVAILABLE) {
        return;
    } else if (XR_FAILED(result)) {
        spdlog::error("Failed to poll OpenXR event {}", result);
        return;
    } else {
        spdlog::trace("Got OpenXR event");
        switch (eventDataBuffer.type) {
            case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
                spdlog::info("Instance loss pending");
                isRunning = false;
                break;
            }
            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                auto event = *reinterpret_cast<const XrEventDataSessionStateChanged *> (&eventDataBuffer);
                switch(event.state){
                    case XR_SESSION_STATE_UNKNOWN:
                    case XR_SESSION_STATE_MAX_ENUM:
                        spdlog::info("Session state changed to unknown");
                        break;
                    case XR_SESSION_STATE_IDLE:
                        spdlog::info("Session state changed to idle");
                        xrSessionInitialized = false;
                        break;
                    case XR_SESSION_STATE_READY: {
                        spdlog::info("Session state changed to ready");
                        // begin openxr session
                        XrSessionBeginInfo sessionBeginInfo{XR_TYPE_SESSION_BEGIN_INFO};
                        sessionBeginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

                        result = xrBeginSession(xrSession, &sessionBeginInfo);
                        if (XR_FAILED(result)) {
                            spdlog::error("Failed to begin OpenXR session {}", result);
                            return;
                        } else {
                            spdlog::trace("Began OpenXR session");
                        }
                        xrSessionInitialized = true;
                        break;
                    }
                    case XR_SESSION_STATE_SYNCHRONIZED:
                    case XR_SESSION_STATE_VISIBLE:
                    case XR_SESSION_STATE_FOCUSED:
                        xrSessionInitialized = true;
                        break;
                    case XR_SESSION_STATE_STOPPING:
                        spdlog::info("Session state changed to stopping");

                        result = xrEndSession(xrSession);
                        if (XR_FAILED(result)) {
                            spdlog::error("Failed to end OpenXR session {}", result);
                            return;
                        } else {
                            spdlog::trace("Ended OpenXR session");
                        }
                        break;
                    case XR_SESSION_STATE_LOSS_PENDING:
                        spdlog::info("Session state changed to loss pending");
                        isRunning = false;
                        break;
                    case XR_SESSION_STATE_EXITING:
                        spdlog::info("Session state changed to exiting");
                        isRunning = false;
                        break;
                }

                break;
            }
            case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING: {
                spdlog::info("Reference space change pending");
                break;
            }
            case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: {
                spdlog::info("Interaction profile changed");
                break;
            }
            default: {
                spdlog::warn("Unhandled event type {}", eventDataBuffer.type);
                break;
            }
        }
    }

}
void createRenderPass() {
   VkAttachmentDescription colorAttachment{};
    colorAttachment.format = static_cast<VkFormat>(swapchainColorFormat);
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear the framebuffer before rendering
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store the framebuffer after rendering
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // We don't care about stencil
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // We don't care about stencil
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // We don't care about initial layout
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // We want to present the image

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // We want to use the image as a color attachment

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // We want to use the pipeline for graphics
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkResult result = vkCreateRenderPass(vkDevice, &renderPassInfo, nullptr, &renderPass);
    if (result != VK_SUCCESS) {
        spdlog::error("Failed to create render pass {}", result);
        return;
    } else {
        spdlog::trace("Created render pass");
    }
}

void createPipeline() {
    auto vertShaderCode = readFile("assets/shaders/simple.vert.spv");
    auto fragShaderCode = readFile("assets/shaders/simple.frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vkDevice, vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(vkDevice, fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(vkDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        spdlog::error("Failed to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        spdlog::error("Failed to create graphics pipeline");
    } else {
        spdlog::trace("Created graphics pipeline");
    }
}

void renderEye(Swapchain* swapchain, const std::vector<SwapchainImage*>& images,
    XrView view, VkDevice device, VkQueue queue, VkRenderPass renderPass,
    VkPipelineLayout pipelineLayout, VkPipeline pipeline) {

    XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    uint32_t swapchainImageIndex;

    XrResult result = xrAcquireSwapchainImage(swapchain->handle, &acquireInfo, &swapchainImageIndex);

    if (result != XR_SUCCESS) {
        spdlog::error("Failed to acquire swapchain image {}", result);
        return;
    }

    XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    waitInfo.timeout = std::numeric_limits<int64_t>::max();

    result = xrWaitSwapchainImage(swapchain->handle, &waitInfo);

    if (result != XR_SUCCESS) {
        spdlog::error("Failed to wait for swapchain image {}", result);
        return;
    }

    auto& image = images[swapchainImageIndex];

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(image->commandBuffer, &beginInfo) != VK_SUCCESS) {
        spdlog::error("Failed to begin command buffer");
        return;
    }

    VkClearValue clearValue{};
    clearValue.color = {1.0f, 0.0f, 0.0f, 1.0f};

    VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = image->framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = { static_cast<uint32_t>(swapchain->width), static_cast<uint32_t>(swapchain->height) };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(image->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {
        0.0f, 0.0f,
        static_cast<float>(swapchain->width), static_cast<float>(swapchain->height),
        0.0f, 1.0f
    };

    VkRect2D scissor = {
        {0, 0},
        {static_cast<uint32_t>(swapchain->width), static_cast<uint32_t>(swapchain->height)}
    };

    vkCmdSetViewport(image->commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(image->commandBuffer, 0, 1, &scissor);

    vkCmdBindPipeline(image->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    vkCmdEndRenderPass(image->commandBuffer);

    if (vkEndCommandBuffer(image->commandBuffer) != VK_SUCCESS) {
        spdlog::error("Failed to record command buffer");
        return;
    }

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &image->commandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;

    if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        spdlog::error("Failed to submit queue");
        return;
    }

    XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    result = xrReleaseSwapchainImage(swapchain->handle, &releaseInfo);

    if (result != XR_SUCCESS) {
        spdlog::error("Failed to release swapchain image {}", result);
        return;
    }
}

void renderFrame() {

    XrFrameWaitInfo frameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};

    XrFrameState frameState{XR_TYPE_FRAME_STATE};
    XrResult result = xrWaitFrame(xrSession, &frameWaitInfo, &frameState);

    if (result != XR_SUCCESS) {
        spdlog::error("Failed to wait for frame {}", result);
        return;
    }

    if (!frameState.shouldRender) {
        return;
    }

    XrFrameBeginInfo frameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};
    result = xrBeginFrame(xrSession, &frameBeginInfo);

    if (result != XR_SUCCESS) {
        spdlog::error("Failed to begin frame {}", result);
        return;
    }

    XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
    viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    viewLocateInfo.displayTime = frameState.predictedDisplayTime;
    viewLocateInfo.space = xrApplicationSpace;

    XrViewState viewState{XR_TYPE_VIEW_STATE};
    uint32_t viewCount = EYE_COUNT;
    std::vector<XrView> views(viewCount, {XR_TYPE_VIEW});

    result = xrLocateViews(xrSession, &viewLocateInfo, &viewState, viewCount, &viewCount, views.data());

    if (result != XR_SUCCESS) {
        spdlog::error("Failed to locate views {}", result);
        return;
    }

    for (auto eye = 0; eye < EYE_COUNT; eye++) {
        renderEye(
                &swapchains[eye],
                wrappedSwapchainImages[eye],
                views[eye],
                vkDevice,
                vkQueue,
                renderPass,
                pipelineLayout,
                pipeline
        );
    }

    XrCompositionLayerProjectionView projectionLayerViews[EYE_COUNT] = {};

    for (auto eye = 0; eye < EYE_COUNT; eye++) {
        projectionLayerViews[eye].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        projectionLayerViews[eye].pose = views[eye].pose;
        projectionLayerViews[eye].fov = views[eye].fov;
        projectionLayerViews[eye].subImage.swapchain = swapchains[eye].handle;
        projectionLayerViews[eye].subImage.imageRect = { { 0, 0 }, { swapchains[eye].width, swapchains[eye].height } };
        projectionLayerViews[eye].subImage.imageArrayIndex = 0;
    }

    XrCompositionLayerProjection projectionLayer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    projectionLayer.space = xrApplicationSpace;
    projectionLayer.viewCount = EYE_COUNT;
    projectionLayer.views = projectionLayerViews;

    auto pLayer = reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projectionLayer);

    XrFrameEndInfo frameEndInfo{XR_TYPE_FRAME_END_INFO};
    frameEndInfo.displayTime = frameState.predictedDisplayTime;
    frameEndInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    frameEndInfo.layerCount = 1;
    frameEndInfo.layers = &pLayer;

    result = xrEndFrame(xrSession, &frameEndInfo);

    if (result != XR_SUCCESS) {
        spdlog::error("Failed to end frame {}", result);
        return;
    }
}

void mainLoop() {
    while (isRunning) {
        processEvents();
        if (xrSessionInitialized)
            renderFrame();
    }
}
