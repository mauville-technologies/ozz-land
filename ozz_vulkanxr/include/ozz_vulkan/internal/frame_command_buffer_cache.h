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

}

