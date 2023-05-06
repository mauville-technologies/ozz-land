//
// Created by Paul Mauviel on 2023-04-06.
//

#include <ozz_vulkan/renderer.h>
#include "ozz_vulkan/internal/xr_utils.h"
#include "ozz_vulkan/internal/vk_utils.h"

namespace OZZ {

    Renderer::Renderer() {
    }

    Renderer::~Renderer() {
        Cleanup();
    }

    void Renderer::Init() {
        spdlog::set_level(spdlog::level::trace);

        spdlog::info("Initializing Renderer.");
        initXrInstance();
        initGetXrSystem();
        initVulkanInstance();
        initVulkanDebugMessenger();
        initVulkanDevice();
        initVulkanMemoryAllocator();
        initXrSession();
        initXrReferenceSpaces();
        initXrSwapchains();
        createRenderPass();
        createCommandPool();
        createFrameData();

        OZZ::ShaderConfiguration config {
                .VertexShaderPath = "assets/shaders/simple.vert.spv",
                .FragmentShaderPath = "assets/shaders/simple.frag.spv"
        };
        shader = CreateShader(config);
    }

    void Renderer::Update() {
        processXREvents();
    }

    void Renderer::BeginFrame() {
        // Get available frame cache
        currentFrameBufferCache = getAvailableFrameBufferCache(vkDevice);
    }

    VkCommandBuffer Renderer::RequestCommandBuffer(EyeTarget target) {
        if (!currentFrameBufferCache) {
            spdlog::warn("No selected frame buffer cache. Have you began the frame?");
            return VK_NULL_HANDLE;
        }
        auto newBuffer = getCommandBufferForSubmission();
        currentFrameBufferCache->PushCommandBuffer(newBuffer, target);

        return newBuffer;
    }

    void Renderer::RenderFrame() {
        // Clean and check framebuffer caches
        for (auto& cache : frameCommandBufferCache) {
            cache.CheckAndClearCaches(vkDevice, commandPool);
        }

        if (!xrSessionInitialized) return;

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
                    static_cast<EyeTarget>(eye)
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

    void Renderer::EndFrame() {
        // No more frame buffer cache
        currentFrameBufferCache = nullptr;
    }

    void Renderer::renderEye(Swapchain* swapchain, const std::vector<std::unique_ptr<SwapchainImage>>& images,
                   XrView view, VkDevice device, VkQueue queue, VkRenderPass renderPass, EyeTarget eye) {

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
        clearValue.color = {0.2f, 0.2f, 0.2f, 1.0f};

        VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = image->framebuffer;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = { static_cast<uint32_t>(swapchain->width), static_cast<uint32_t>(swapchain->height) };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearValue;

        vkCmdBeginRenderPass(image->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);


        // execute current frame buffer cache for current eye
        if (!currentFrameBufferCache) {
            spdlog::error("No frame buffer cache");
        }

        auto& eyeBuffers = currentFrameBufferCache->GetCommandBuffers(eye);
        auto& bothBuffers = currentFrameBufferCache->GetCommandBuffers(EyeTarget::BOTH);

//        vkCmdExecuteCommands(image->commandBuffer, static_cast<uint32_t>(eyeBuffers.size()), eyeBuffers.data());
        vkCmdExecuteCommands(image->commandBuffer, static_cast<uint32_t>(bothBuffers.size()), bothBuffers.data());

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

        // Get the appropriate fence for the current eye
        auto fence = currentFrameBufferCache->GetFence(eye);

        if (vkQueueSubmit(queue, 1, &submitInfo, fence) != VK_SUCCESS) {
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

    void Renderer::Cleanup() {

        spdlog::info("Shutting down renderer.");

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
            OZZ::vkDestroyDebugUtilsMessengerEXT(vkInstance, vkDebugMessenger, nullptr);
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


    std::unique_ptr<Shader> Renderer::CreateShader(ShaderConfiguration &config) {
        config.RenderPass = renderPass;
        return std::make_unique<Shader>(vkDevice, config);
    }

    void Renderer::initXrInstance() {
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
        }
    }

    void Renderer::initGetXrSystem() {
        // Initialize System
        spdlog::trace("Initializing OpenXR System.");

        XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
        systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        auto result = xrGetSystem(xrInstance, &systemInfo, &xrSystemId);

        if (XR_FAILED(result)) {
            spdlog::error("Failed to get OpenXR system {}", result);
        } else {
            spdlog::trace("Got OpenXR system {}", xrSystemId);
        }
    }

    void Renderer::initVulkanInstance() {
        // Initialize vulkan instance
        spdlog::trace("Initializing Vulkan Instance.");

        XrGraphicsRequirementsVulkan2KHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR};
        auto result = xrGetVulkanGraphicsRequirements2KHR(xrInstance, xrSystemId, &graphicsRequirements);

        if (XR_FAILED(result)) {
            spdlog::error("Failed to get OpenXR Vulkan Graphics Requirements {}", result);
            return;
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
        } else {
            spdlog::trace("Created OpenXR Vulkan Instance");
        }

        if (vkResult != VK_SUCCESS) {
            spdlog::error("Failed to create Vulkan Instance {}", vkResult);
        } else {
            spdlog::trace("Created Vulkan Instance");
        }
    }

    void Renderer::initVulkanDebugMessenger() {
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
        debugUtilsMessengerCreateInfoExt.pfnUserCallback = &Renderer::debugCallback;
        debugUtilsMessengerCreateInfoExt.pUserData = nullptr;

        // Create debug utils messenger

        auto vkResult = OZZ::vkCreateDebugUtilsMessengerEXT(vkInstance, &debugUtilsMessengerCreateInfoExt, nullptr,
                                                  &vkDebugMessenger);

        if (vkResult != VK_SUCCESS) {
            spdlog::error("Failed to create Vulkan Debug Messenger {}", vkResult);
            return;
        } else {
            spdlog::trace("Created Vulkan Debug Messenger");
        }
    }

    void Renderer::initVulkanDevice() {
        // Get Vulkan Physical Device
        spdlog::trace("Getting Vulkan Physical Device");

        XrVulkanGraphicsDeviceGetInfoKHR deviceGetInfoKHR{XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR};
        deviceGetInfoKHR.systemId = xrSystemId;
        deviceGetInfoKHR.vulkanInstance = vkInstance;

        auto result = xrGetVulkanGraphicsDevice2KHR(xrInstance, &deviceGetInfoKHR, &vkPhysicalDevice);

        if (XR_FAILED(result)) {
            spdlog::error("Failed to get OpenXR Vulkan Graphics Device {}", result);
            return;
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

        VkResult vkResult;
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
    }

    void Renderer::initVulkanMemoryAllocator() {
        // initialize vma allocator
        VmaAllocatorCreateInfo vmaAllocatorCreateInfo{};
        vmaAllocatorCreateInfo.physicalDevice = vkPhysicalDevice;
        vmaAllocatorCreateInfo.device = vkDevice;
        vmaAllocatorCreateInfo.instance = vkInstance;

        auto vkResult = vmaCreateAllocator(&vmaAllocatorCreateInfo, &vmaAllocator);

        // check if successful
        if (vkResult != VK_SUCCESS) {
            spdlog::error("Failed to create VMA Allocator {}", vkResult);
        } else {
            spdlog::trace("Created VMA Allocator");
        }
    }

    void Renderer::initXrSession() {
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
        auto result = xrCreateSession(xrInstance, &sessionCreateInfo, &xrSession);

        if (XR_FAILED(result)) {
            spdlog::error("Failed to create OpenXR Session {}", result);
            return;
        } else {
            spdlog::trace("Created OpenXR Session");
        }
    }

    void Renderer::initXrReferenceSpaces() {
        // Create Reference Space
        spdlog::trace("Creating XR Reference Space");

        XrReferenceSpaceCreateInfo referenceSpaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
        referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        referenceSpaceCreateInfo.poseInReferenceSpace = { {0, 0, 0, 1 }, { 0, 0, 0 } };

        auto result = xrCreateReferenceSpace(xrSession, &referenceSpaceCreateInfo, &xrApplicationSpace);

        if (XR_FAILED(result)) {
            spdlog::error("Failed to create OpenXR Reference Space {}", result);
            return;
        } else {
            spdlog::trace("Created OpenXR Reference Space");
        }
    }

    void Renderer::initXrSwapchains() {
        XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES};
        auto result = xrGetSystemProperties(xrInstance, xrSystemId, &systemProperties);

        if (XR_FAILED(result)) {
            spdlog::error("Failed to get OpenXR System Properties {}", result);
            return;
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
            return;
        }

        viewConfigurationViews.resize(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
        result = xrEnumerateViewConfigurationViews(xrInstance, xrSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, viewCount, &viewCount, viewConfigurationViews.data());

        if (XR_FAILED(result)) {
            spdlog::error("Failed to get OpenXR View Configuration Views {}", result);
            return;
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
                return;
            }
            std::vector<int64_t > swapchainFormats(swapchainFormatCount);
            result = xrEnumerateSwapchainFormats(xrSession, swapchainFormatCount, &swapchainFormatCount, swapchainFormats.data());

            if (XR_FAILED(result)) {
                spdlog::error("Failed to get OpenXR Swapchain Formats {}", result);
                return;
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
                    return;
                } else {
                    spdlog::trace("Created OpenXR Swapchain");
                }

                swapchains.push_back(swapchain);

                // Get swapchain images
                uint32_t swapchainImageCount;
                result = xrEnumerateSwapchainImages(swapchain.handle, 0, &swapchainImageCount, nullptr);

                if (XR_FAILED(result)) {
                    spdlog::error("Failed to get OpenXR Swapchain Image count {}", result);
                    return;
                }

                swapchainImages[i].resize(swapchainImageCount, {XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR});
                result = xrEnumerateSwapchainImages(swapchain.handle,
                                                    swapchainImageCount, &swapchainImageCount, reinterpret_cast<XrSwapchainImageBaseHeader *> (swapchainImages[i].data()));

                if (XR_FAILED(result)) {
                    spdlog::error("Failed to get OpenXR Swapchain Images {}", result);
                    return;
                } else {
                    spdlog::trace("Got OpenXR Swapchain Images");
                }
            }
        }
    }

    void Renderer::createRenderPass() {
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

    void Renderer::createCommandPool() {
        VkCommandPoolCreateInfo commandPoolCreateInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolCreateInfo.queueFamilyIndex = vkQueueFamilyIndex;

        auto vkResult = vkCreateCommandPool(vkDevice, &commandPoolCreateInfo, nullptr, &commandPool);

        if (vkResult != VK_SUCCESS) {
            spdlog::error("Failed to create Vulkan Command Pool {}", vkResult);
            return;
        } else {
            spdlog::trace("Created Vulkan Command Pool");
        }
    }

    void Renderer::createFrameData() {
        wrappedSwapchainImages.resize(EYE_COUNT);
        for (auto eye = 0; eye < EYE_COUNT; eye++) {
            wrappedSwapchainImages[eye] = std::vector<std::unique_ptr<SwapchainImage>> {swapchainImages[eye].size() };
            for (auto i = 0; i < swapchainImages[eye].size(); i++) {
                wrappedSwapchainImages[eye][i] = std::make_unique<SwapchainImage> (
                        vkDevice,
                        &swapchains[eye],
                        swapchainImages[eye][i],
                        renderPass,
                        commandPool
                );
            }
        }
    }

    void Renderer::processXREvents() {
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
                            break;
                        case XR_SESSION_STATE_EXITING:
                            spdlog::info("Session state changed to exiting");
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

    VkCommandBuffer Renderer::getCommandBufferForSubmission() {
        // create a new secondary command buffer
        VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        auto result = vkAllocateCommandBuffers(vkDevice, &allocInfo, &commandBuffer);

        if (result != VK_SUCCESS) {
            spdlog::error("Failed to allocate command buffer {}", result);
            return nullptr;
        }

        return commandBuffer;
    }

    VkBool32 Renderer::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                     VkDebugUtilsMessageTypeFlagsEXT messageType,
                                     const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
        spdlog::info("Vulkan Debug Message Severity: {}", messageSeverity);
        spdlog::info("Vulkan Debug Message: {}", pCallbackData->pMessage);
        return VK_TRUE;
    }



}