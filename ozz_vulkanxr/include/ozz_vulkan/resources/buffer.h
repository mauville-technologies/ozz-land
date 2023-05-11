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
        VkBuffer _buffer;
        VmaAllocation _allocation;
        VkDeviceSize _size;
        VmaAllocator _allocator;
    };
}