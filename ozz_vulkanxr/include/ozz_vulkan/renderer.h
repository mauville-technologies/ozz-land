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
#include <unordered_map>

namespace OZZ {
    enum class EyeTarget {
        LEFT,
        RIGHT,
        BOTH,
    };

    struct FrameCommandBufferCache {
        explicit FrameCommandBufferCache(VkDevice vkDevice) {
            // Create fences
            VkFenceCreateInfo fenceCreateInfo {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
            if (vkCreateFence(vkDevice, &fenceCreateInfo, nullptr, &leftEyeFence) != VK_SUCCESS ||
                vkCreateFence(vkDevice, &fenceCreateInfo, nullptr, &rightEyeFence) != VK_SUCCESS) {
                spdlog::error("Failed to create fences");
            }

            CommandBuffers[EyeTarget::LEFT] = {};
            CommandBuffers[EyeTarget::RIGHT] = {};
            CommandBuffers[EyeTarget::BOTH] = {};
        }

        void Claim() {
            Available = false;
        }

        void CheckAndClearCaches(VkDevice vkDevice, VkCommandPool commandPool, uint64_t timeout = 0) {
            VkFence fences[] = {leftEyeFence, rightEyeFence};

            if (vkWaitForFences(vkDevice, 2, fences, VK_TRUE, timeout) == VK_SUCCESS) {
                // Delete command buffers
                vkFreeCommandBuffers(vkDevice, commandPool, CommandBuffers[OZZ::EyeTarget::LEFT].size(),
                                     CommandBuffers[OZZ::EyeTarget::LEFT].data());
                vkFreeCommandBuffers(vkDevice, commandPool, CommandBuffers[OZZ::EyeTarget::RIGHT].size(),
                                     CommandBuffers[OZZ::EyeTarget::RIGHT].data());
                vkFreeCommandBuffers(vkDevice, commandPool, CommandBuffers[OZZ::EyeTarget::BOTH].size(),
                                     CommandBuffers[OZZ::EyeTarget::BOTH].data());
                CommandBuffers[OZZ::EyeTarget::LEFT].clear();
                CommandBuffers[OZZ::EyeTarget::RIGHT].clear();
                CommandBuffers[OZZ::EyeTarget::BOTH].clear();

                // Reset fences
                vkResetFences(vkDevice, 2, fences);

                Available = true;
                spdlog::trace("Cleared command buffer cache");
            }
        }

        const auto& GetFence(EyeTarget target) const {
            if (target == EyeTarget::LEFT) {
                return leftEyeFence;
            } else {
                return rightEyeFence;
            }
        }

        const auto& GetCommandBuffers(EyeTarget target) const {
            return CommandBuffers.at(target);
        }

        void PushCommandBuffer(VkCommandBuffer commandBuffer, EyeTarget target) {
            CommandBuffers.at(target).push_back(commandBuffer);
        }

        bool Available {true};
    private:
        std::unordered_map<EyeTarget, std::vector<VkCommandBuffer>> CommandBuffers;
        VkFence leftEyeFence {VK_NULL_HANDLE};
        VkFence rightEyeFence {VK_NULL_HANDLE};
    };

    class Renderer {
    public:
        Renderer();
        ~Renderer();

        void Init();
        void Update();
        void Submit(VkCommandBuffer commandBuffer, EyeTarget target);
        void RenderFrame();
        void Cleanup();

        VkCommandBuffer GetCommandBufferForSubmission();
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
            return &frameCommandBufferCache.back();
        }

    };
} // namespace OZZ