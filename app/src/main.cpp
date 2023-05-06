//
// Created by Paul Mauviel on 2023-04-06.
//

#include <ozz_vulkan/renderer.h>
#include "ozz_vulkan/internal/utils.h"

bool isRunning {true};
std::unique_ptr<OZZ::Renderer> renderer;

VkPipeline createPipeline();

int main(int argc, char** argv) {
    // Initialize renderer
    renderer = std::make_unique<OZZ::Renderer>();
    renderer->Init();

    OZZ::ShaderConfiguration config {
        .VertexShaderPath = "assets/shaders/simple.vert.spv",
        .FragmentShaderPath = "assets/shaders/simple.frag.spv"
    };

    auto shader = renderer->CreateShader(config);

    while (isRunning) {
        renderer->Update();
        renderer->BeginFrame();
        // Record command buffers


        auto commandBuffer = renderer->RequestCommandBuffer(OZZ::EyeTarget::BOTH);

        VkCommandBufferInheritanceInfo inheritanceInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO};
        inheritanceInfo.renderPass = shader->GetConfiguration().RenderPass;
        inheritanceInfo.subpass = shader->GetConfiguration().Subpass;
        inheritanceInfo.pNext = nullptr;

        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        beginInfo.pInheritanceInfo = &inheritanceInfo;
        beginInfo.pNext = nullptr;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        auto [width, height] = renderer->GetSwapchainSize();
        VkViewport viewport = {
                0, 0,
                (float) width, (float) height,
                0, 1
        };

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor = {
                {0,                           0},
                {(uint32_t) width, (uint32_t) height}
        };

        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        shader->Bind(commandBuffer);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        vkEndCommandBuffer(commandBuffer);

        renderer->RenderFrame();
        renderer->EndFrame();
    }
}


