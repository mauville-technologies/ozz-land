//
// Created by ozzadar on 04/05/23.
//

#pragma once

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#define EYE_COUNT 2

#include "ozz_vulkan/internal/graphics_includes.h"
#include "ozz_vulkan/internal/swapchain_image.h"
#include "ozz_vulkan/internal/xr_types.h"
#include "ozz_vulkan/resources/shader.h"

#include "ozz_vulkan/internal/frame_command_buffer_cache.h"

#include <memory>
#include <vk_mem_alloc.h>
#include <unordered_map>
#include <tuple>

namespace OZZ {
    class Renderer {
    public:
        Renderer();
        ~Renderer();

        // Lifecycle functions
        void Init();
        void Update();
        void BeginFrame();
        VkCommandBuffer RequestCommandBuffer(EyeTarget target);
        void RenderFrame();
        void EndFrame();
        void Cleanup();

        std::unique_ptr<Shader> CreateShader(ShaderConfiguration& config);
        std::tuple<int, int> GetSwapchainSize() const { return std::make_tuple(swapchains[0].width, swapchains[0].height); }
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
                                 XrView view, VkDevice device, VkQueue queue, VkRenderPass renderPass, EyeTarget eye);

        void processXREvents();

        VkCommandBuffer getCommandBufferForSubmission();
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

        /*
         * The Framebuffer cache conatains secondary command buffers for each eye, or both eyes
         *
         * This cache is used to allow for recording commands outside of the main renderer
         * and then running them to the appropriate renderpass when needed.
         *
         * Order:
         * Eye Specific
         * - Left
         * - Right
         * Both
         * - Both
         */
        std::vector<FrameCommandBufferCache> frameCommandBufferCache {};
        FrameCommandBufferCache* currentFrameBufferCache {nullptr};

        FrameCommandBufferCache* getAvailableFrameBufferCache(VkDevice vkDevice) {
            for (auto& cache : frameCommandBufferCache) {
                if (cache.Available) {
                    cache.Claim();
                    return &cache;
                }
            }
            spdlog::trace("No available cache found");

            // No available cache found, create a new one
            frameCommandBufferCache.emplace_back(vkDevice);
            frameCommandBufferCache.back().Claim();

            if (frameCommandBufferCache.size() > 5) {
                spdlog::warn("Frame command buffer cache is getting large, performance could be suffering.");
            }
            return &frameCommandBufferCache.back();
        }

        std::unique_ptr<Shader> shader;
    };
} // namespace OZZ