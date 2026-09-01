// camera.cpp
// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#include <knot/camera.h>

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace knot {

bool Frustum::intersectsSphere(
    const glm::vec3& center,
    float radius
) const {
    for (const FrustumPlane& plane : planes) {
        if (plane.signedDistance(center) < -radius)
            return false;
    }

    return true;
}

bool Frustum::intersectsAABB(
    const glm::vec3& min,
    const glm::vec3& max
) const {
    for (const FrustumPlane& plane : planes) {
        glm::vec3 positive = min;

        if (plane.normal.x >= 0.0f)
            positive.x = max.x;

        if (plane.normal.y >= 0.0f)
            positive.y = max.y;

        if (plane.normal.z >= 0.0f)
            positive.z = max.z;

        if (plane.signedDistance(positive) < 0.0f)
            return false;
    }

    return true;
}

Camera::Camera(
    glm::vec3 startPos,
    float nearPlane,
    float farPlane
)
    : nearPlane(nearPlane),
      farPlane(farPlane) {
    position = startPos;
}

void Camera::lookAtTarget(glm::vec3 targetPos) {
    const glm::vec3 delta = targetPos - position;

    if (glm::length(delta) <= 0.0f)
        return;

    const glm::vec3 direction = glm::normalize(delta);

    rotation = glm::quatLookAt(
        direction,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
}

const Frustum& Camera::getFrustum(float aspectRatio) const {
    const glm::mat4 projection = getProjectionMatrix(aspectRatio);
    const glm::mat4 view = getViewMatrix();
    const glm::mat4 clip = projection * view;

    // Left
    frustum.planes[Frustum::Left].normal.x =
        clip[0][3] + clip[0][0];
    frustum.planes[Frustum::Left].normal.y =
        clip[1][3] + clip[1][0];
    frustum.planes[Frustum::Left].normal.z =
        clip[2][3] + clip[2][0];
    frustum.planes[Frustum::Left].distance =
        clip[3][3] + clip[3][0];

    // Right
    frustum.planes[Frustum::Right].normal.x =
        clip[0][3] - clip[0][0];
    frustum.planes[Frustum::Right].normal.y =
        clip[1][3] - clip[1][0];
    frustum.planes[Frustum::Right].normal.z =
        clip[2][3] - clip[2][0];
    frustum.planes[Frustum::Right].distance =
        clip[3][3] - clip[3][0];

    // Bottom
    frustum.planes[Frustum::Bottom].normal.x =
        clip[0][3] + clip[0][1];
    frustum.planes[Frustum::Bottom].normal.y =
        clip[1][3] + clip[1][1];
    frustum.planes[Frustum::Bottom].normal.z =
        clip[2][3] + clip[2][1];
    frustum.planes[Frustum::Bottom].distance =
        clip[3][3] + clip[3][1];

    // Top
    frustum.planes[Frustum::Top].normal.x =
        clip[0][3] - clip[0][1];
    frustum.planes[Frustum::Top].normal.y =
        clip[1][3] - clip[1][1];
    frustum.planes[Frustum::Top].normal.z =
        clip[2][3] - clip[2][1];
    frustum.planes[Frustum::Top].distance =
        clip[3][3] - clip[3][1];

    // Near
    frustum.planes[Frustum::Near].normal.x =
        clip[0][3] + clip[0][2];
    frustum.planes[Frustum::Near].normal.y =
        clip[1][3] + clip[1][2];
    frustum.planes[Frustum::Near].normal.z =
        clip[2][3] + clip[2][2];
    frustum.planes[Frustum::Near].distance =
        clip[3][3] + clip[3][2];

    // Far
    frustum.planes[Frustum::Far].normal.x =
        clip[0][3] - clip[0][2];
    frustum.planes[Frustum::Far].normal.y =
        clip[1][3] - clip[1][2];
    frustum.planes[Frustum::Far].normal.z =
        clip[2][3] - clip[2][2];
    frustum.planes[Frustum::Far].distance =
        clip[3][3] - clip[3][2];

    for (FrustumPlane& plane : frustum.planes) {
        const float length = glm::length(plane.normal);

        if (length > 0.0f) {
            plane.normal /= length;
            plane.distance /= length;
        }
    }

    return frustum;
}

PerspectiveCamera::PerspectiveCamera(
    glm::vec3 startPos,
    float fov,
    float nearPlane,
    float farPlane
)
    : Camera(startPos, nearPlane, farPlane),
      fov(fov) {
}

glm::mat4 PerspectiveCamera::getViewMatrix() const {
    return glm::lookAt(
        position,
        position + getFront(),
        getUp()
    );
}

glm::mat4 PerspectiveCamera::getProjectionMatrix(
    float aspectRatio
) const {
    return glm::perspective(
        glm::radians(fov),
        aspectRatio,
        nearPlane,
        farPlane
    );
}

MovingCamera::MovingCamera(
    glm::vec3 startPos,
    float fov,
    float nearPlane,
    float farPlane
)
    : PerspectiveCamera(startPos, fov, nearPlane, farPlane),
      speed(5.0f),
      sensitivity(0.1f) {
}
void MovingCamera::move(
    glm::vec3 direction,
    float deltaTime
) {
    position += direction * (speed * deltaTime);
}

void MovingCamera::rotate(float xOffset, float yOffset) {
    yaw += xOffset * sensitivity;
    pitch += yOffset * sensitivity;

    pitch = glm::clamp(pitch, -89.0f, 89.0f);
}

glm::vec3 MovingCamera::getFront() const {
    const float yawRadians = glm::radians(yaw);
    const float pitchRadians = glm::radians(pitch);

    glm::vec3 front;

    front.x = std::cos(yawRadians) * std::cos(pitchRadians);
    front.y = std::sin(pitchRadians);
    front.z = std::sin(yawRadians) * std::cos(pitchRadians);

    return glm::normalize(front);
}

glm::vec3 MovingCamera::getRight() const {
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

    return glm::normalize(
        glm::cross(getFront(), worldUp)
    );
}

glm::vec3 MovingCamera::getUp() const {
    const glm::vec3 front = getFront();
    const glm::vec3 right = getRight();

    return glm::normalize(
        glm::cross(right, front)
    );
}

glm::mat4 MovingCamera::getViewMatrix() const {
    return glm::lookAt(
        position,
        position + getFront(),
        getUp()
    );
}

OrthographicCamera::OrthographicCamera(
    glm::vec3 startPos,
    float nearPlane,
    float farPlane,
    float size
)
    : Camera(startPos, nearPlane, farPlane),
      size(size) {
}

glm::mat4 OrthographicCamera::getViewMatrix() const {
    return glm::lookAt(
        position,
        position + getFront(),
        getUp()
    );
}

glm::mat4 OrthographicCamera::getProjectionMatrix(
    float aspectRatio
) const {
    const float halfHeight = size * 0.5f;
    const float halfWidth = halfHeight * aspectRatio;

    return glm::ortho(
        -halfWidth,
        halfWidth,
        -halfHeight,
        halfHeight,
        nearPlane,
        farPlane
    );
}

} // namespace knot