//
// Created by ozzadar on 05/05/23.
//
#pragma once

#include <ozz_vulkan/internal/graphics_includes.h>
#include <filesystem>

namespace OZZ {
    struct ShaderConfiguration {
        VkFormat SwapchainColorFormat;

        std::filesystem::path VertexShaderPath;
        std::filesystem::path FragmentShaderPath;
    };

    class Shader {
    public:
        Shader(VkDevice device, ShaderConfiguration  config);
       ~Shader();

       void Bind(VkCommandBuffer commandBuffer);

       [[nodiscard]] const ShaderConfiguration& GetConfiguration() const { return _config; }
    private:
        void createPipeline();

    private:
        VkDevice _device;
        const ShaderConfiguration _config;
        VkPipeline _pipeline;
        VkPipelineLayout _pipelineLayout;
    };

} // OZZ

