#include <ozz_vulkan/graphics_includes.h>

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
