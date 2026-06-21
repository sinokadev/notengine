// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <knot/resources.h>

namespace knot {

class Camera : public Object {
public:
    glm::vec3 worldUp;

    float yaw;
    float pitch;
    float fov;

    Camera(glm::vec3 startPos = glm::vec3(0.0f, 0.0f, 3.0f), 
           glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f), 
           float yaw = -90.0f, float pitch = 0.0f);

    glm::mat4 getViewMatrix() const;
    void updateCameraVector();
    void lookAtTarget(glm::vec3 targetPos);
};

class MovingCamera : public Camera {
public:
    float speed;
    float sensitivity;

    MovingCamera(glm::vec3 startPos = glm::vec3(0.0f, 0.0f, 5.0f));

    void move(glm::vec3 direction, float deltaTime) override;
    void rotate(float xOffset, float yOffset, bool constrainPitch = true) override;
};

} // namespace knot
