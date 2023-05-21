//
// Created by ozzadar on 08/05/23.
//

#include "application.h"
#include "ozz_vulkan/brushes/shapes.h"


Application::Application() {
// Initialize _renderer
    _renderer = std::make_unique<OZZ::Renderer>();
    _renderer->Init();

    // Create cube
    _cube = std::make_unique<Cube>(_renderer.get());
}

Application::~Application() {
    _renderer->WaitIdle();
    _cube.reset(nullptr);
    _renderer.reset(nullptr);
}

void Application::Run() {
    _isRunning = true;
    while (_isRunning) {
        if (_renderer->Update()) {
            Stop();
            continue;
        }
        update();
        renderFrame();
    }

    spdlog::info("Application stopped");
}

void Application::Stop() {
    _isRunning = false;
    spdlog::info("Stopping application");
}

void Application::update() {
    _cube->Update(0);
}

void Application::renderFrame() {
    _renderer->BeginFrame();
    renderEye(OZZ::EyeTarget::Left);
    renderEye(OZZ::EyeTarget::Right);
    _renderer->RenderFrame();
    _renderer->EndFrame();
}

void Application::renderEye(OZZ::EyeTarget eye) {
    VkCommandBufferInheritanceRenderingInfo renderingInheritance { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO };
    renderingInheritance.colorAttachmentCount = 1;
    auto swapchainFormat = _renderer->GetSwapchainFormat();
    renderingInheritance.pColorAttachmentFormats = &swapchainFormat;
    renderingInheritance.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkCommandBufferInheritanceInfo inheritanceInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO};
    inheritanceInfo.pNext = &renderingInheritance;

    // Record command buffers
    auto [width, height] = _renderer->GetSwapchainSize();
    VkViewport viewport = {
            0, 0,
            (float) width, (float) height,
            0, 1
    };

    VkRect2D scissor = {
            {0,                           0},
            {(uint32_t) width, (uint32_t) height}
    };

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo = &inheritanceInfo;
    beginInfo.pNext = nullptr;

    auto commandBuffer = _renderer->RequestCommandBuffer(eye);
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    _cube->Draw(commandBuffer, glm::mat4(1.0f), glm::mat4(1.0f));

    vkEndCommandBuffer(commandBuffer);
}
