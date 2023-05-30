//
// Created by ozzadar on 29/05/23.
//
#pragma once
#include <glm/glm.hpp>
#include "transform_component.h"

class CameraObject {
public:
    CameraObject() = default;
    ~CameraObject() = default;

    void Translate(glm::vec3 translation) {
        _transformComponent.Translate(translation);
    }

    void Rotate(glm::quat rotation) {
        _transformComponent.Rotate(rotation);
    }

    [[nodiscard]] glm::mat4 GetViewMatrix() const {
        return glm::inverse(_transformComponent.GetModelMatrix());
    }

    [[nodiscard]] glm::vec3 GetPosition() const {
        return _transformComponent.GetTranslation();
    }

    [[nodiscard]] glm::quat GetRotation() const {
        return _transformComponent.GetRotation();
    }
private:
    TransformComponent _transformComponent;
};
