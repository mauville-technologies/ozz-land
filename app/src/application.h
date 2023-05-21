//
// Created by ozzadar on 08/05/23.
//

#pragma once

#include <ozz_vulkan/renderer.h>
#include <memory>
#include "cube.h"

class Application {
public:
    Application();
    ~Application();

    void Run();
    void Stop();

private:
    void update();
    void renderFrame();
    void renderEye(OZZ::EyeTarget eye);

private:
    std::unique_ptr<OZZ::Renderer> _renderer;
    bool _isRunning {false};

    std::unique_ptr<Cube> _cube;

};
