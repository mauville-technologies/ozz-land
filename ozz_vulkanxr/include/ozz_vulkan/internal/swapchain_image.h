//
// Created by ozzadar on 02/05/23.
//

#pragma once

#include "graphics_includes.h"
#include "vk_utils.h"
#include <spdlog/spdlog.h>

namespace OZZ {
    struct Swapchain {
        XrSwapchain handle{XR_NULL_HANDLE};
        int32_t width{0};
        int32_t height{0};
        int64_t format{0};
    };

    struct SwapchainImage {
        SwapchainImage() = default;

        // Copy constructor
        SwapchainImage(const SwapchainImage &other) {
            imageView = other.imageView;
            image = other.image;
            commandBuffer = other.commandBuffer;
        }

        // Move constructor
        SwapchainImage(SwapchainImage &&other) noexcept {
            imageView = other.imageView;
            image = other.image;
            commandBuffer = other.commandBuffer;
            vkDevice = other.vkDevice;
            commandPool = other.commandPool;
        }

        SwapchainImage(VkDevice device, VmaAllocator allocator, const Swapchain *swapchain,
                       XrSwapchainImageVulkan2KHR image,
                       VkCommandPool commandPool, VkQueue queue) : image(image), vkDevice(device), vmaAllocator(allocator),
                                                    commandPool(commandPool) {

            VkImageViewCreateInfo imageViewCreateInfo{.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            imageViewCreateInfo.image = image.image;
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCreateInfo.format = static_cast<VkFormat>(swapchain->format);
            imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
            imageViewCreateInfo.subresourceRange.levelCount = 1;
            imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            imageViewCreateInfo.subresourceRange.layerCount = 1;

            // Create depth stencil image
            VkImageCreateInfo depthImageCreateInfo{.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
            depthImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            depthImageCreateInfo.format = VK_FORMAT_D32_SFLOAT;
            depthImageCreateInfo.extent.width = swapchain->width;
            depthImageCreateInfo.extent.height = swapchain->height;
            depthImageCreateInfo.extent.depth = 1;
            depthImageCreateInfo.mipLevels = 1;
            depthImageCreateInfo.arrayLayers = 1;
            depthImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            depthImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            depthImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

            VmaAllocationCreateInfo depthImageAllocationCreateInfo{};
            depthImageAllocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            depthImageAllocationCreateInfo.requiredFlags = VkMemoryPropertyFlags {VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

            if (vmaCreateImage(vmaAllocator, &depthImageCreateInfo, &depthImageAllocationCreateInfo,
                               &depthImage, &depthImageAllocation, nullptr) != VK_SUCCESS) {
                spdlog::error("Failed to create depth image");
            }

            VkImageViewCreateInfo depthImageViewCreateInfo{.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            depthImageViewCreateInfo.image = depthImage;
            depthImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            depthImageViewCreateInfo.format = VK_FORMAT_D32_SFLOAT;
            depthImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            depthImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
            depthImageViewCreateInfo.subresourceRange.levelCount = 1;
            depthImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            depthImageViewCreateInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &depthImageViewCreateInfo, nullptr, &depthImageView) != VK_SUCCESS) {
                spdlog::error("Failed to create depth image view");
            }

            if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS) {
                spdlog::error("Failed to create image view");
            }

            // transition depth image layout
            transitionImageLayout(device, commandPool, queue, depthImage, VK_FORMAT_D32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

            VkCommandBufferAllocateInfo commandBufferAllocateInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
            commandBufferAllocateInfo.commandPool = commandPool;
            commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            commandBufferAllocateInfo.commandBufferCount = 1;

            if (vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer) != VK_SUCCESS) {
                spdlog::error("Failed to allocate command buffer");
            } else {
                spdlog::trace("Allocated command buffer");
            }
        }

        ~SwapchainImage() {
            spdlog::trace("Destroying swapchain image");
            vkDestroyImageView(vkDevice, imageView, nullptr);
            vkDestroyImageView(vkDevice, depthImageView, nullptr);

            vmaDestroyImage(vmaAllocator, depthImage, depthImageAllocation);
            vkFreeCommandBuffers(vkDevice, commandPool, 1, &commandBuffer);
            vkDevice = VK_NULL_HANDLE;
            commandPool = VK_NULL_HANDLE;
        }

        VkImageView imageView{VK_NULL_HANDLE};
        XrSwapchainImageVulkan2KHR image;
        VkCommandBuffer commandBuffer{VK_NULL_HANDLE};

        VkCommandPool commandPool{VK_NULL_HANDLE};
        VkDevice vkDevice{VK_NULL_HANDLE};
        VmaAllocator vmaAllocator{VK_NULL_HANDLE};

        VkImage depthImage{VK_NULL_HANDLE};
        VkImageView depthImageView{VK_NULL_HANDLE};
        VmaAllocation depthImageAllocation{VK_NULL_HANDLE};
    };
}