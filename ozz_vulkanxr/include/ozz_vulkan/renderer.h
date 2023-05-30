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
#include "ozz_vulkan/resources/buffer.h"

#include <memory>
#include <unordered_map>
#include <tuple>
#include <optional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace OZZ {
    struct FieldOfView {
        float AngleDown;
        float AngleLeft;
        float AngleRight;
        float AngleUp;
    };

    struct EyePoseInfo {
        FieldOfView FOV;
        glm::quat Orientation;
        glm::vec3 Position;

        [[nodiscard]] glm::mat4 GetProjectionMatrix() const {

            auto projection = glm::mat4{1.f};

            const float nearZ = 0.1f;
            const float farZ = 100.0f;

            const float tanAngleLeft = tanf(FOV.AngleLeft);
            const float tanAngleRight = tanf(FOV.AngleRight);
            const float tanAngleUp = tanf(FOV.AngleUp);
            const float tanAngleDown = tanf(FOV.AngleDown);

            const float tanAngleWidth = tanAngleRight - tanAngleLeft;

            const float tanAngleHeight = tanAngleDown - tanAngleUp;

            projection[0][0] = 2.0f / tanAngleWidth;
            projection[1][0] = 0.0f;
            projection[2][0] = (tanAngleRight + tanAngleLeft) / tanAngleWidth;
            projection[3][0] = 0.0f;

            projection[0][1] = 0.0f;
            projection[1][1] = 2.0f / tanAngleHeight;
            projection[2][1] = (tanAngleUp + tanAngleDown) / tanAngleHeight;
            projection[3][1] = 0.0f;

            projection[0][2] = 0.0f;
            projection[1][2] = 0.0f;
            projection[2][2] = -farZ / (farZ - nearZ);
            projection[3][2] = -(farZ * nearZ) / (farZ - nearZ);

            projection[0][3] = 0.0f;
            projection[1][3] = 0.0f;
            projection[2][3] = -1.0f;
            projection[3][3] = 0.0f;

            return projection;
        }
    };

    struct FrameInfo {
       int64_t PredictedDisplayTime {0};
    };

    class Renderer {
    public:
        Renderer();
        ~Renderer();

        // Lifecycle functions
        void Init();
        bool Update();
        std::optional<FrameInfo> BeginFrame();
        VkCommandBuffer RequestCommandBuffer(EyeTarget target);
        void RenderFrame(const FrameInfo& frameInfo);
        void EndFrame();
        void WaitIdle();
        void Cleanup();

        [[nodiscard]] std::optional<std::tuple<EyePoseInfo, EyePoseInfo>> GetEyePoseInfo(int64_t predictedDisplayTime) const;

        // Resource functions
        std::unique_ptr<Shader> CreateShader(ShaderConfiguration& config);
        std::unique_ptr<VertexBuffer> CreateVertexBuffer(const std::vector<Vertex>& vertices);
        std::unique_ptr<IndexBuffer> CreateIndexBuffer(const std::vector<uint32_t>& indices);

        [[nodiscard]] std::tuple<int, int> GetSwapchainSize() const { return std::make_tuple(swapchains[0].width, swapchains[0].height); }
        [[nodiscard]] VkFormat GetSwapchainFormat() const { return static_cast<VkFormat>(swapchainColorFormat); }
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
        void createCommandPool();
        void createFrameData();

        void renderEye(Swapchain* swapchain, const std::vector<std::unique_ptr<SwapchainImage>>& images,
                                 VkQueue queue, EyeTarget eye);

        bool processXREvents();

        VkCommandBuffer getCommandBufferForSubmission();
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                            VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                            void* pUserData);

        [[nodiscard]] static XrPosef getPoseFromEyePose(const EyePoseInfo& eyeInfo) {
            return {
                .orientation = {
                        .x = eyeInfo.Orientation.x,
                        .y = eyeInfo.Orientation.y,
                        .z = eyeInfo.Orientation.z,
                        .w = eyeInfo.Orientation.w,
                },
                .position = {
                        .x = eyeInfo.Position.x,
                        .y = eyeInfo.Position.y,
                        .z = eyeInfo.Position.z
                }
            };
        };

        static XrFovf getFovFromEyeFov(const EyePoseInfo& eyeInfo) {
            return {
                    .angleLeft = eyeInfo.FOV.AngleLeft,
                    .angleRight = eyeInfo.FOV.AngleRight,
                    .angleUp = eyeInfo.FOV.AngleUp,
                    .angleDown = eyeInfo.FOV.AngleDown,
            };
        };
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

        VkCommandPool commandPool;

        uint32_t vkQueueFamilyIndex;

        bool xrSessionInitialized{false};
        XrInstance xrInstance{XR_NULL_HANDLE};
        XrSystemId xrSystemId{XR_NULL_SYSTEM_ID};
        XrSession xrSession{XR_NULL_HANDLE};
        XrSpace xrApplicationSpace{XR_NULL_HANDLE};
        std::vector<XrViewConfigurationView> viewConfigurationViews;
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
        std::vector<std::shared_ptr<FrameCommandBufferCache>> frameCommandBufferCache {};
        FrameCommandBufferCache* currentFrameBufferCache {nullptr};

        FrameCommandBufferCache* getAvailableFrameBufferCache(VkDevice vkDevice) {
            for (auto& cache : frameCommandBufferCache) {
                if (cache->Available) {
                    cache->Claim();
                    return cache.get();
                }
            }
            spdlog::trace("No available cache found");

            // No available cache found, create a new one
            auto& newCache = frameCommandBufferCache.emplace_back(
                    std::make_shared<FrameCommandBufferCache>(vkDevice, commandPool));
            newCache->Claim();

            if (frameCommandBufferCache.size() > 5) {
                spdlog::warn("Frame command buffer cache is getting large, performance could be suffering.");
            }
            return newCache.get();
        }

        bool _pauseValidation { false };

    };
} // namespace OZZ