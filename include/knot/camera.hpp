#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace knot {

class Camera {
  public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 world_up;

    float yaw;
    float pitch;
    float fov;

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
           glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = -90.0f,
           float pitch = 0.0f)
        : position(position), world_up(world_up), yaw(yaw), pitch(pitch),
          front(glm::vec3(0.0f, 0.0f, -1.0f)), fov(45.0f) {
        update_camera_vector();
    }

    glm::mat4 get_view_matrix() const {
        return glm::lookAt(position, position + front, up);
    }

    void update_camera_vector() {
        glm::vec3 new_front;
        new_front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        new_front.y = sin(glm::radians(pitch));
        new_front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        front = glm::normalize(new_front);
        right = glm::normalize(glm::cross(front, world_up));
        up = glm::normalize(glm::cross(right, front));
    }

    void look_at_target(glm::vec3 target_pos) {
        const glm::vec3 direction = glm::normalize(target_pos - position);

        pitch = glm::degrees(asin(direction.y));
        yaw = glm::degrees(atan2(direction.z, direction.x));

        update_camera_vector();
    }
};

class MovingCamera : public Camera {
  public:
    float speed;
    float sensitivity;

    MovingCamera(glm::vec3 start_pos = glm::vec3(0.0f, 0.0f, 5.0f))
        : Camera(start_pos), speed(2.5f), sensitivity(0.1f) {}

    void move(glm::vec3 direction, float deltaTime) {
        position += direction * (speed * deltaTime);
    }

    void rotate(float xOffset, float yOffset, bool constrainPitch = true) {
        yaw += xOffset * sensitivity;
        pitch += yOffset * sensitivity;

        if (constrainPitch) {
            pitch = glm::clamp(pitch, -89.0f, 89.0f);
        }

        update_camera_vector();
    }
};

} // namespace knot
