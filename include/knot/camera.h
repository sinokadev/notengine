#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <knot/resources.h>

namespace knot {

struct FrustumPlane {
    glm::vec3 normal{0.0f};
    float distance = 0.0f;

    float signedDistance(const glm::vec3& point) const {
        return glm::dot(normal, point) + distance;
    }
};

struct Frustum {
    FrustumPlane planes[6];

    enum Plane { Left = 0, Right, Bottom, Top, Near, Far };

    bool intersectsSphere(const glm::vec3& center, float radius) const {
        for (const auto& plane : planes) {
            if (plane.signedDistance(center) < -radius)
                return false;
        }

        return true;
    }

    bool intersectsAABB(const glm::vec3& min, const glm::vec3& max) const {
        for (const auto& plane : planes) {
            glm::vec3 p(plane.normal.x >= 0.0f ? max.x : min.x, plane.normal.y >= 0.0f ? max.y : min.y, plane.normal.z >= 0.0f ? max.z : min.z);

            if (plane.signedDistance(p) < 0.0f)
                return false;
        }

        return true;
    }
};

class Camera : public Transform {
public:
    static constexpr float kNearPlane = 0.1f;
    static constexpr float kFarPlane = 100.0f;

    glm::vec3 worldUp{0.0f, 1.0f, 0.0f};

    float yaw = -90.0f;
    float pitch = 0.0f;
    float fov = 45.0f;
    float nearPlane = kNearPlane;
    float farPlane = kFarPlane;

    Camera(glm::vec3 startPos = glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = -90.0f, float pitch = 0.0f,
           float fov = 45.0f, float nearPlane = kNearPlane, float farPlane = kFarPlane);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

    void updateCameraVector();
    void lookAtTarget(glm::vec3 targetPos);

    const Frustum& getFrustum(float aspectRatio) const;

private:
    mutable Frustum frustum;
};

class MovingCamera : public Camera {
public:
    float speed = 5.0f;
    float sensitivity = 0.1f;

    MovingCamera(glm::vec3 startPos = glm::vec3(0.0f, 0.0f, 5.0f));

    void move(glm::vec3 direction, float deltaTime);

    void rotate(float xOffset, float yOffset, bool constrainPitch = true);
};

} // namespace knot