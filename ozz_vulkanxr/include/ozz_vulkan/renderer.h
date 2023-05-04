//
// Created by ozzadar on 04/05/23.
//

#pragma once

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#define EYE_COUNT 2

#include <ozz_vulkan/graphics_includes.h>
#include <ozz_vulkan/swapchain_image.h>

#include <memory>
#include <vk_mem_alloc.h>

namespace OZZ {
    class Renderer {
    public:
        Renderer();
        ~Renderer();

        void Init();
        void Update();
        void RenderFrame();
        void Cleanup();

    private:
        void initXrInstance();
        void initGetXrSystem();
        void initVulkanInstance();
        void initVulkanDebugMessenger();
        void initVulkanDevice();
        void initVulkanMemoryAllocator();
        void initXrSession();
        void initXrReferenceSpaces();
        void initXrSwapchains();
        void createRenderPass();
        void createCommandPool();
        void createFrameData();

        void renderEye(Swapchain* swapchain, const std::vector<std::unique_ptr<SwapchainImage>>& images,
                                 XrView view, VkDevice device, VkQueue queue, VkRenderPass renderPass,
                                 VkPipelineLayout pipelineLayout, VkPipeline pipeline);

        void processXREvents();

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                            VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                            void* pUserData);
    private:
        // Vulkan Entities
        VkInstance vkInstance{VK_NULL_HANDLE};
        VkPhysicalDevice vkPhysicalDevice{VK_NULL_HANDLE};
        VkDevice vkDevice{VK_NULL_HANDLE};
        VkDebugUtilsMessengerEXT vkDebugMessenger{VK_NULL_HANDLE};
        VkQueue vkQueue;
        VmaAllocator vmaAllocator;
        std::vector<XrSwapchainImageVulkan2KHR> swapchainImages[EYE_COUNT];
        std::vector<std::vector<std::unique_ptr<SwapchainImage>>> wrappedSwapchainImages{};

        VkRenderPass renderPass;
        VkCommandPool commandPool;

        uint32_t vkQueueFamilyIndex;

        bool xrSessionInitialized{false};
        XrInstance xrInstance{XR_NULL_HANDLE};
        XrSystemId xrSystemId{XR_NULL_SYSTEM_ID};
        XrSession xrSession{XR_NULL_HANDLE};
        XrSpace xrApplicationSpace{XR_NULL_HANDLE};
        std::vector<XrViewConfigurationView> viewConfigurationViews;
        std::vector<XrView> views;
        int64_t swapchainColorFormat{-1};
        std::vector<Swapchain> swapchains;
    };
} // namespace OZZ