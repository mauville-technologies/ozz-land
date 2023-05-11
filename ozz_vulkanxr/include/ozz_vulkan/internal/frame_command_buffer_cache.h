//
// Created by ozzadar on 05/05/23.
//

#pragma once

#include "graphics_includes.h"
#include <spdlog/spdlog.h>
#include <vector>
#include <unordered_map>
#include "xr_types.h"

namespace OZZ {
    struct FrameCommandBufferCache {
        explicit FrameCommandBufferCache(VkDevice vkDevice,
                                         VkCommandPool commandPool) : vkDevice(vkDevice), commandPool(commandPool) {
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

        ~FrameCommandBufferCache() {
            spdlog::trace("Destroying FrameCommandBufferCache");
            if (leftEyeFence != VK_NULL_HANDLE) {
                vkDestroyFence(vkDevice, leftEyeFence, nullptr);
                leftEyeFence = VK_NULL_HANDLE;
            }

            if (rightEyeFence != VK_NULL_HANDLE) {
                vkDestroyFence(vkDevice, rightEyeFence, nullptr);
                rightEyeFence = VK_NULL_HANDLE;
            }

            clearCommandBuffers();
        }

        void Claim() {
            Available = false;
        }

        void CheckAndClearCaches(uint64_t timeout = 0) {
            VkFence fences[] = {leftEyeFence, rightEyeFence};

            if (vkWaitForFences(vkDevice, 2, fences, VK_TRUE, timeout) == VK_SUCCESS) {
                // Clear command buffers
                clearCommandBuffers();

                // Reset fences
                vkResetFences(vkDevice, 2, fences);

                Available = true;
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
        void clearCommandBuffers() {
            if (!CommandBuffers[OZZ::EyeTarget::LEFT].empty())
                vkFreeCommandBuffers(vkDevice, commandPool, CommandBuffers[OZZ::EyeTarget::LEFT].size(),
                                     CommandBuffers[OZZ::EyeTarget::LEFT].data());

            if (!CommandBuffers[OZZ::EyeTarget::RIGHT].empty())
                vkFreeCommandBuffers(vkDevice, commandPool, CommandBuffers[OZZ::EyeTarget::RIGHT].size(),
                                     CommandBuffers[OZZ::EyeTarget::RIGHT].data());

            if (!CommandBuffers[OZZ::EyeTarget::BOTH].empty())
                vkFreeCommandBuffers(vkDevice, commandPool, CommandBuffers[OZZ::EyeTarget::BOTH].size(),
                                     CommandBuffers[OZZ::EyeTarget::BOTH].data());

            CommandBuffers[OZZ::EyeTarget::LEFT].clear();
            CommandBuffers[OZZ::EyeTarget::RIGHT].clear();
            CommandBuffers[OZZ::EyeTarget::BOTH].clear();
        }
    private:
        std::unordered_map<EyeTarget, std::vector<VkCommandBuffer>> CommandBuffers;
        VkFence leftEyeFence {VK_NULL_HANDLE};
        VkFence rightEyeFence {VK_NULL_HANDLE};

        VkDevice vkDevice {VK_NULL_HANDLE};
        VkCommandPool commandPool {VK_NULL_HANDLE};
    };

}

