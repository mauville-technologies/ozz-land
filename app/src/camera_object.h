//
// Created by ozzadar on 29/05/23.
//
#pragma once
#include <glm/glm.hpp>
#include "transform_component.h"
#include "ozz_vulkan/renderer.h"

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
        // build headpose matrix
        auto headPoseMatrix = glm::mat4{1.f};
        headPoseMatrix = glm::translate(headPoseMatrix, _headPose.Position);
        headPoseMatrix = glm::translate(headPoseMatrix, _transformComponent.GetTranslation());
        headPoseMatrix *= glm::toMat4(_transformComponent.GetRotation() * _headPose.Orientation);

        // build view matrix
        return glm::inverse(headPoseMatrix);
    }

    [[nodiscard]] glm::vec3 GetPosition() const {
        return _transformComponent.GetTranslation();
    }

    [[nodiscard]] glm::quat GetRotation() const {
        return _transformComponent.GetRotation();
    }

    void SetHeadPose(const OZZ::HeadPoseInfo& headPoseInfo) {
        _headPose = headPoseInfo;
    }
private:
    TransformComponent _transformComponent;
    OZZ::HeadPoseInfo _headPose;
};
