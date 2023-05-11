//
// Created by ozzadar on 08/05/23.
//

#pragma once

#include <ozz_vulkan/renderer.h>
#include <memory>

class Application {
public:
    Application();
    ~Application();

    void Run();
    void Stop();

private:
    std::unique_ptr<OZZ::Renderer> _renderer;
    bool _isRunning {false};


    // Temporary
    std::unique_ptr<OZZ::Shader> _shader;
    std::unique_ptr<OZZ::VertexBuffer> _vertexBuffer;
};
