//
// Created by Paul Mauviel on 2023-04-06.
//

#include <ozz_vulkan/renderer.h>

bool isRunning {true};
std::unique_ptr<OZZ::Renderer> renderer;

int main(int argc, char** argv) {
    // Initialize renderer
    renderer = std::make_unique<OZZ::Renderer>();
    renderer->Init();

    while (isRunning) {
        renderer->Update();
        renderer->RenderFrame();
    }
}
