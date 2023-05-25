//
// Created by ozzadar on 08/05/23.
//
#include <glm/glm.hpp>

#include "application.h"
#include "ozz_vulkan/brushes/shapes.h"


Application::Application() {
// Initialize _renderer
    _renderer = std::make_unique<OZZ::Renderer>();
    _renderer->Init();

    // Create cube
    _cube = std::make_unique<Cube>(_renderer.get());
    _cube2 = std::make_unique<Cube>(_renderer.get());
    _cube2->Translate(glm::vec3(1.f, 0.0f, -5.0f));
    _cube2->Rotate(90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
}

Application::~Application() {
    _renderer->WaitIdle();
    _cube.reset(nullptr);
    _cube2.reset(nullptr);
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
    _cube2->Update(0);
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
    renderingInheritance.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
    renderingInheritance.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkCommandBufferInheritanceInfo inheritanceInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO};
    inheritanceInfo.pNext = &renderingInheritance;

    // Record command buffers

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT ;
    beginInfo.pInheritanceInfo = &inheritanceInfo;
    beginInfo.pNext = nullptr;

    auto commandBuffer = _renderer->RequestCommandBuffer(eye);
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

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

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    auto projection = glm::perspectiveRH_ZO(glm::radians(45.0f), (float) width / (float) height, 0.1f, 15.0f);
    projection[1][1] *= -1;

    auto view = glm::lookAt(glm::vec3(0.0f, 0.0f, 10.0f), // Camera is at (0,0,3), in World Space
                            glm::vec3(0.0f, 0.0f, 0.0f), // and looks at the origin
                            glm::vec3(0.0f, 1.0f, 0.0f)  // Head is up (set to 0,-1,0 to look upside-down)
    );

    _cube->Draw(commandBuffer, view, projection);
    _cube2->Draw(commandBuffer, view, projection);

    vkEndCommandBuffer(commandBuffer);
}
