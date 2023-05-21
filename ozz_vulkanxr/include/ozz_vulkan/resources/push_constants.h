//
// Created by ozzadar on 20/05/23.
//

#pragma once
#include <vector>
#include <cstdint>
#include <ozz_vulkan/internal/graphics_includes.h>

namespace OZZ {
    class PushConstantDefinition {
    public:
        PushConstantDefinition(uint32_t size, VkShaderStageFlags flags, uint32_t offset = 0) : _offset(offset),
                                                                                               _size(size),
                                                                                               _stageFlags(flags) {}

        [[nodiscard]] uint32_t GetOffset() const { return _offset; }

        [[nodiscard]] VkShaderStageFlags GetStageFlags() const { return _stageFlags; }

        [[nodiscard]] uint32_t GetSize() const { return _size; }

        [[nodiscard]] VkPushConstantRange GetRange() const { return {_stageFlags, _offset, _size}; }

    private:
        uint32_t _offset;
        uint32_t _size;
        VkShaderStageFlags _stageFlags;
    };
}