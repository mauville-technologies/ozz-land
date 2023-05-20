//
// Created by ozzadar on 10/05/23.
//

#pragma once
#include <ozz_vulkan/resources/types.h>
#include <ozz_vulkan/internal/graphics_includes.h>
#include <vector>

namespace OZZ {

    class VertexBuffer {
    public:
        VertexBuffer(VmaAllocator allocator, const std::vector<Vertex>& vertices);
        ~VertexBuffer();

        void Bind(VkCommandBuffer commandBuffer);

        [[nodiscard]] VkBuffer GetBuffer() const { return _buffer; }
        [[nodiscard]] VkDeviceSize GetSize() const { return _size; }
    private:
        VkBuffer _buffer { VK_NULL_HANDLE };
        VmaAllocation _allocation { VK_NULL_HANDLE };
        VkDeviceSize _size { 0 };
        VmaAllocator _allocator { VK_NULL_HANDLE };
    };

    class IndexBuffer {
    public:
        IndexBuffer(VmaAllocator allocator, const std::vector<uint32_t>& indices);
        ~IndexBuffer();

        void Bind(VkCommandBuffer commandBuffer);

        [[nodiscard]] VkBuffer GetBuffer() const { return _buffer; }
        [[nodiscard]] VkDeviceSize GetSize() const { return _size; }
        [[nodiscard]] uint32_t GetIndexCount() const { return _size / sizeof(uint32_t); }

    private:
        VkBuffer _buffer { VK_NULL_HANDLE };
        VmaAllocation _allocation { VK_NULL_HANDLE };
        VkDeviceSize _size { 0 };
        VmaAllocator _allocator { VK_NULL_HANDLE };
    };
}