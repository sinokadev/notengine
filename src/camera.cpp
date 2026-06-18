#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <knot/camera.h>

namespace knot {
Camera::Camera(glm::vec3 position, glm::vec3 worldUp, float yaw, float pitch, unsigned int objectId)
    : Object(objectId), worldUp(worldUp), yaw(yaw), pitch(pitch), fov(45.0f) {
    this->position = position;
    this->front = glm::vec3(0.0f, 0.0f, -1.0f);
    updateCameraVector();
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

void Camera::updateCameraVector() {
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    front = glm::normalize(newFront);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::lookAtTarget(glm::vec3 targetPos) {
    const glm::vec3 direction = glm::normalize(targetPos - position);

    pitch = glm::degrees(asin(direction.y));
    yaw = glm::degrees(atan2(direction.z, direction.x));

    updateCameraVector();
}

MovingCamera::MovingCamera(glm::vec3 startPos, unsigned int objectId)
    : Camera(startPos, glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f, objectId), speed(2.5f), sensitivity(0.1f) {
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
