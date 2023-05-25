//
// Created by ozzadar on 20/05/23.
//

#include "cube.h"
#include "ozz_vulkan/brushes/shapes.h"

struct ShaderMatrices {
    glm::mat4 Model;
    glm::mat4 View;
    glm::mat4 Projection;
};

Cube::Cube(OZZ::Renderer* renderer) {
    createIndexBuffer(renderer);
    createVertexBuffer(renderer);
    createShader(renderer);
}

Cube::~Cube() {
    _indexBuffer.reset(nullptr);
    _vertexBuffer.reset(nullptr);
    _shader.reset(nullptr);
}

void Cube::Update(float deltaTime) {
   Rotate(0.6f, glm::vec3(0.0f, 1.0f, 0.0f));
}

void Cube::Draw(VkCommandBuffer commandBuffer, glm::mat4 view, glm::mat4 projection) {
    _shader->Bind(commandBuffer);
    _shader->YeetPushConstants<ShaderMatrices>(commandBuffer, ShaderMatrices {
            .Model = _modelMatrix,
            .View = view,
            .Projection = projection
    }, VK_SHADER_STAGE_VERTEX_BIT);

    _indexBuffer->Bind(commandBuffer);
    _vertexBuffer->Bind(commandBuffer);

    vkCmdDrawIndexed(commandBuffer, _indexBuffer->GetIndexCount(), 1, 0, 0, 0);
}

void Cube::createVertexBuffer(OZZ::Renderer* renderer) {
    // convert array to vector
    auto cubeVertices = std::vector<OZZ::Vertex>(OZZ::Brushes::cubeVertices.begin(), OZZ::Brushes::cubeVertices.end());
    _vertexBuffer = renderer->CreateVertexBuffer(cubeVertices);
}

void Cube::createIndexBuffer(OZZ::Renderer* renderer) {
    // convert array to vector
    auto cubeIndices = std::vector<uint32_t>(OZZ::Brushes::cubeIndices.begin(), OZZ::Brushes::cubeIndices.end());
    _indexBuffer = renderer->CreateIndexBuffer(cubeIndices);
}

void Cube::createShader(OZZ::Renderer *renderer) {
    OZZ::ShaderConfiguration config {
            .VertexShaderPath = "assets/shaders/simple.vert.spv",
            .FragmentShaderPath = "assets/shaders/simple.frag.spv",
            .PushConstants = {
                    OZZ::PushConstantDefinition(sizeof(ShaderMatrices), VK_SHADER_STAGE_VERTEX_BIT)
            }
    };

    _shader = renderer->CreateShader(config);
}

void Cube::updateModelMatrix() {
    _modelMatrix = glm::translate(glm::mat4{1.f}, _translation) * glm::mat4_cast(_rotation);
}
