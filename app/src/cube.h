//
// Created by ozzadar on 20/05/23.
//

#pragma once
#include <ozz_vulkan/renderer.h>
#include <glm/gtx/quaternion.hpp>

class Cube {
public:
    Cube(OZZ::Renderer* renderer);
    ~Cube();

    void Update(float deltaTime);
    void Draw(VkCommandBuffer commandBuffer, glm::mat4 view, glm::mat4 projection);

    void Rotate(float degrees, glm::vec3 axis) {
        _rotation = glm::rotate(_rotation, glm::radians(degrees), axis);
        updateModelMatrix();
    }

private:
    void createVertexBuffer(OZZ::Renderer* renderer);
    void createIndexBuffer(OZZ::Renderer* renderer);
    void createShader(OZZ::Renderer* renderer);

    void updateModelMatrix();
private:
    std::unique_ptr<OZZ::Shader> _shader;
    std::unique_ptr<OZZ::VertexBuffer> _vertexBuffer;
    std::unique_ptr<OZZ::IndexBuffer> _indexBuffer;

    glm::mat4 _modelMatrix { 1.0f };
    glm::quat _rotation { 1.0f, 0.0f, 0.0f, 0.0f };
};
