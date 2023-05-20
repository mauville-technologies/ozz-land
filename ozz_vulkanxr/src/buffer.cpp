//
// Created by ozzadar on 10/05/23.
//

#include <cstring>
#include <spdlog/spdlog.h>
#include "ozz_vulkan/resources/buffer.h"

OZZ::VertexBuffer::VertexBuffer(VmaAllocator allocator, const std::vector<Vertex> &vertices) : _allocator(allocator) {
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = sizeof(Vertex) * vertices.size();
    bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    VmaAllocationCreateInfo allocationCreateInfo{};
    allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    vmaCreateBuffer(allocator, &bufferCreateInfo, &allocationCreateInfo, &_buffer, &_allocation, nullptr);
    _size = bufferCreateInfo.size;

    void* data;
    vmaMapMemory(allocator, _allocation, &data);
    memcpy(data, vertices.data(), _size);
    vmaUnmapMemory(allocator, _allocation);
}

OZZ::VertexBuffer::~VertexBuffer() {
    if (_buffer != VK_NULL_HANDLE) {
        spdlog::trace("Destroying vertex buffer");
        vmaDestroyBuffer(_allocator, _buffer, _allocation);
        _buffer = VK_NULL_HANDLE;
    }
}

void OZZ::VertexBuffer::Bind(VkCommandBuffer commandBuffer) {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_buffer, &offset);
}

OZZ::IndexBuffer::IndexBuffer(VmaAllocator allocator, const std::vector<uint32_t> &indices): _allocator(allocator) {
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = sizeof(uint32_t) * indices.size();
    bufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    VmaAllocationCreateInfo allocationCreateInfo{};
    allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    vmaCreateBuffer(allocator, &bufferCreateInfo, &allocationCreateInfo, &_buffer, &_allocation, nullptr);
    _size = bufferCreateInfo.size;


    void* data;
    vmaMapMemory(allocator, _allocation, &data);
    memcpy(data, indices.data(), _size);
    vmaUnmapMemory(allocator, _allocation);
}

OZZ::IndexBuffer::~IndexBuffer() {
    if (_buffer != VK_NULL_HANDLE) {
        spdlog::trace("Destroying index buffer");
        vmaDestroyBuffer(_allocator, _buffer, _allocation);
        _buffer = VK_NULL_HANDLE;
    }
}

void OZZ::IndexBuffer::Bind(VkCommandBuffer commandBuffer) {
    vkCmdBindIndexBuffer(commandBuffer, _buffer, 0, VK_INDEX_TYPE_UINT32);
}
