#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <knot/camera.h>

namespace knot {
Camera::Camera(glm::vec3 startPos, glm::vec3 worldUp, float yaw, float pitch, float fov, float nearPlane, float farPlane)
    : worldUp(worldUp), yaw(yaw), pitch(pitch), fov(fov), nearPlane(nearPlane), farPlane(farPlane) {
    this->position = startPos;
    updateCameraVector();
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + getFront(), getUp());
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}

void Camera::updateCameraVector() {
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    glm::vec3 front = glm::normalize(newFront);
    glm::vec3 right = glm::normalize(glm::cross(front, worldUp));
    glm::vec3 up = glm::normalize(glm::cross(right, front));

    rotation = glm::quatLookAt(front, up);
}

void Camera::lookAtTarget(glm::vec3 targetPos) {
    const glm::vec3 direction = glm::normalize(targetPos - position);

    pitch = glm::degrees(asin(glm::clamp(direction.y, -1.0f, 1.0f)));
    yaw = glm::degrees(atan2(direction.z, direction.x));

    updateCameraVector();
}

const Frustum& Camera::getFrustum(float aspectRatio) const {
    glm::mat4 projection = getProjectionMatrix(aspectRatio);
    glm::mat4 view = getViewMatrix();
    glm::mat4 clip = projection * view;

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

    // Normalize
    for (auto& plane : frustum.planes) {
        float length = glm::length(plane.normal);
        if (length > 0.0f) {
            plane.normal /= length;
            plane.distance /= length;
        }
    }

    return frustum;
}

MovingCamera::MovingCamera(glm::vec3 startPos) : Camera(startPos), speed(5.0f), sensitivity(0.1f) {
}

void MovingCamera::move(glm::vec3 direction, float deltaTime) {
    position += direction * (speed * deltaTime);
}

void MovingCamera::rotate(float xOffset, float yOffset, bool constrainPitch) {
    yaw += xOffset * sensitivity;
    pitch += yOffset * sensitivity;

    if (constrainPitch) {
        pitch = glm::clamp(pitch, -89.0f, 89.0f);
    }

    updateCameraVector();
}
} // namespace knot
