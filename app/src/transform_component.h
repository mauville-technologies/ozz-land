//
// Created by ozzadar on 29/05/23.
//
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

struct TransformComponent {

    TransformComponent() = default;
    ~TransformComponent() = default;

    void Translate(glm::vec3 translation) {
        Translation += translation;
    }

    void Rotate(glm::quat rotation) {
        Rotation *= rotation;
    }

    void ScaleBy(glm::vec3 scale) {
        Scale *= scale;
    }

    glm::vec3 GetTranslation() const {
        return Translation;
    }

    glm::quat GetRotation() const {
        return Rotation;
    }

    glm::vec3 GetScale() const {
        return Scale;
    }

    [[nodiscard]] glm::mat4 GetModelMatrix() const {
        auto model = glm::mat4{1.f};
        model = glm::translate(model, Translation);
        model *= glm::toMat4(Rotation);
        model = glm::scale(model, Scale);
        return model;
    }
private:
    glm::vec3 Translation{0.f};
    glm::quat Rotation{1.f, 0.f, 0.f, 0.f};
    glm::vec3 Scale{1.f};
};
