//
// Created by ozzadar on 05/05/23.
//
#pragma once

#include <ozz_vulkan/internal/graphics_includes.h>
#include <ozz_vulkan/resources/push_constants.h>
#include <filesystem>

namespace OZZ {
    struct ShaderConfiguration {
        VkFormat SwapchainColorFormat;

        std::filesystem::path VertexShaderPath;
        std::filesystem::path FragmentShaderPath;

        std::vector<PushConstantDefinition> PushConstants;
    };

    class Shader {
    public:
        Shader(VkDevice device, ShaderConfiguration  config);
       ~Shader();

       void Bind(VkCommandBuffer commandBuffer);

       template <typename T>
       void YeetPushConstants(VkCommandBuffer commandBuffer, T constants, VkShaderStageFlags shaderFlags, uint32_t offset = 0) {
          vkCmdPushConstants(commandBuffer, _pipelineLayout, shaderFlags, offset, sizeof(T), &constants);
       }

       [[nodiscard]] const ShaderConfiguration& GetConfiguration() const { return _config; }
    private:
        void recreatePipeline();
        void destroyPipeline();
        void createPipeline();

    private:
        VkDevice _device;
        const ShaderConfiguration _config;
        VkPipeline _pipeline;
        VkPipelineLayout _pipelineLayout;
    };

} // OZZ

