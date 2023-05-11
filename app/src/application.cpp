//
// Created by ozzadar on 08/05/23.
//

#include "application.h"

Application::Application() {
// Initialize _renderer
    _renderer = std::make_unique<OZZ::Renderer>();
    _renderer->Init();

    OZZ::ShaderConfiguration config {
            .VertexShaderPath = "assets/shaders/simple.vert.spv",
            .FragmentShaderPath = "assets/shaders/simple.frag.spv"

    };

    _shader = _renderer->CreateShader(config);

    _vertexBuffer = _renderer->CreateVertexBuffer({
        {{0.0f, -0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}},
        {{-0.5f, 0.5f, 0.0f},  {1.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f},   {0.0f, 1.0f, 1.0f}}
    });
}

Application::~Application() {
    _renderer->WaitIdle();
    _shader.reset(nullptr);
    _vertexBuffer.reset(nullptr);
    _renderer.reset(nullptr);
}

void Application::Run() {
    _isRunning = true;
    while (_isRunning) {
        if (_renderer->Update()) {
            Stop();
            continue;
        }

        _renderer->BeginFrame();
        // Record command buffers

        auto commandBuffer = _renderer->RequestCommandBuffer(OZZ::EyeTarget::BOTH);

        VkCommandBufferInheritanceInfo inheritanceInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO};
        inheritanceInfo.renderPass = _shader->GetConfiguration().RenderPass;
        inheritanceInfo.subpass = _shader->GetConfiguration().Subpass;
        inheritanceInfo.pNext = nullptr;

        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = &inheritanceInfo;
        beginInfo.pNext = nullptr;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        auto [width, height] = _renderer->GetSwapchainSize();
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

        _shader->Bind(commandBuffer);
        _vertexBuffer->Bind(commandBuffer);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        vkEndCommandBuffer(commandBuffer);

        _renderer->RenderFrame();
        _renderer->EndFrame();
    }

    spdlog::info("Application stopped");
}

void Application::Stop() {
    _isRunning = false;
    spdlog::info("Stopping application");
}
