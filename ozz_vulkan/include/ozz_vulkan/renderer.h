//
// Created by Paul Mauviel on 2023-04-06.
//

#pragma once
#include <vulkan/vulkan.h>

namespace OZZ_VULKAN {
    class Renderer {
    public:
        Renderer() = default;
        ~Renderer();

        void Init();
    private:
        void initInstance();
        void destroyInstance();

    private:
        VkInstance _instance { VK_NULL_HANDLE };
    };
}
