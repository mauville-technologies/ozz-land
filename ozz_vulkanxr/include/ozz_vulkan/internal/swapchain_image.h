//
// Created by ozzadar on 02/05/23.
//

#pragma once

#include "graphics_includes.h"
#include <spdlog/spdlog.h>

struct Swapchain {
    XrSwapchain handle{XR_NULL_HANDLE};
    int32_t width{0};
    int32_t height{0};
    int64_t format{0};
};

struct SwapchainImage {
    SwapchainImage() = default;
    ~SwapchainImage() = default;
    // Copy constructor
    SwapchainImage(const SwapchainImage& other) {
        imageView = other.imageView;
        image = other.image;
        framebuffer = other.framebuffer;
        commandBuffer = other.commandBuffer;
    }

    // Move constructor
    SwapchainImage(SwapchainImage&& other) noexcept {
        imageView = other.imageView;
        image = other.image;
        framebuffer = other.framebuffer;
        commandBuffer = other.commandBuffer;
    }

    SwapchainImage(VkDevice device, const Swapchain *swapchain, XrSwapchainImageVulkan2KHR image,
        VkRenderPass renderPass, VkCommandPool commandPool): image(image) {

        VkImageViewCreateInfo imageViewCreateInfo { .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        imageViewCreateInfo.image = image.image;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = static_cast<VkFormat>(swapchain->format);
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS) {
            spdlog::error("Failed to create image view");
        }

        VkFramebufferCreateInfo framebufferCreateInfo { .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebufferCreateInfo.renderPass = renderPass;
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.pAttachments = &imageView;
        framebufferCreateInfo.width = swapchain->width;
        framebufferCreateInfo.height = swapchain->height;
        framebufferCreateInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffer) != VK_SUCCESS) {
            spdlog::error("Failed to create framebuffer");
        }

        VkCommandBufferAllocateInfo commandBufferAllocateInfo { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        commandBufferAllocateInfo.commandPool = commandPool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer) != VK_SUCCESS) {
            spdlog::error("Failed to allocate command buffer");
        } else {
            spdlog::trace("Allocated command buffer");
        }
    }

    VkImageView imageView { VK_NULL_HANDLE };
    XrSwapchainImageVulkan2KHR image;
    VkFramebuffer framebuffer { VK_NULL_HANDLE };
    VkCommandBuffer commandBuffer { VK_NULL_HANDLE };
};