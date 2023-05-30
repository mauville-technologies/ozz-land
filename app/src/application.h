//
// Created by ozzadar on 08/05/23.
//

#pragma once

#include <ozz_vulkan/renderer.h>
#include <memory>
#include "cube.h"
#include "camera_object.h"

class Application {
public:
    Application();
    ~Application();

    void Run();
    void Stop();

private:
    void update();
    void renderFrame();
    void renderEye(OZZ::EyeTarget eye, const OZZ::EyePoseInfo& eyePoseInfo);

private:
    std::unique_ptr<OZZ::Renderer> _renderer;
    bool _isRunning {false};

    std::unique_ptr<Cube> _cube;
    std::unique_ptr<Cube> _cube2;
    std::unique_ptr<CameraObject> _cameraObject;

    uint64_t _frameCount {0};
};
