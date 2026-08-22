#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <knot/resources.h>

namespace knot {

/**
 * @brief Represents a plane of a view frustum.
 */
struct FrustumPlane {
    glm::vec3 normal{0.0f};
    float distance = 0.0f;

    /**
     * @brief Calculates the signed distance from the plane to a point.
     *
     * @param point The point to calculate the distance to.
     * @return The signed distance from the plane to the point.
     */
    float signedDistance(const glm::vec3& point) const {
        return glm::dot(normal, point) + distance;
    }
};

/**
 * @brief Represents a camera view frustum.
 *
 * Contains the six clipping planes used for frustum culling.
 */
struct Frustum {
    FrustumPlane planes[6];

    enum Plane {
        Left = 0,
        Right,
        Bottom,
        Top,
        Near,
        Far
    };

    /**
     * @brief Tests whether a sphere intersects the frustum.
     *
     * @param center The center of the sphere.
     * @param radius The radius of the sphere.
     * @return true if the sphere intersects or is inside the frustum.
     */
    bool intersectsSphere(const glm::vec3& center, float radius) const;

    /**
     * @brief Tests whether an axis-aligned bounding box intersects the frustum.
     *
     * @param min The minimum corner of the AABB.
     * @param max The maximum corner of the AABB.
     * @return true if the AABB intersects or is inside the frustum.
     */
    bool intersectsAABB(const glm::vec3& min, const glm::vec3& max) const;
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

    /**
     * @brief Gets the view matrix.
     *
     * The view matrix transforms world-space coordinates
     * into view-space coordinates.
     *
     * @return The view matrix.
     */
    glm::mat4 getViewMatrix() const;
    /**
     * @brief Gets the perspective projection matrix.
     *
     * Creates a perspective projection matrix using the camera's
     * field of view, the specified aspect ratio, and the near and far clipping planes.
     *
     * @param aspectRatio The viewport width-to-height ratio.
     * @return The perspective projection matrix.
     */
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

    /**
     * @brief Updates the camera rotation from its orientation vectors.
     *
     * Calculates the camera's front, right, and up vectors from the current
     * yaw and pitch angles, then updates the camera rotation.
     */
    void Camera::updateCameraVector();

    /**
     * @brief Rotates the camera to look at a target position.
     *
     * Calculates the yaw and pitch angles required for the camera to face
     * the specified target position, then updates the camera rotation.
     *
     * @param targetPos The position for the camera to look at.
     */
    void Camera::lookAtTarget(glm::vec3 targetPos);

    /**
     * @brief Gets the camera's view frustum.
     *
     * Calculates the six clipping planes of the camera's view frustum from
     * the projection and view matrices. The resulting planes are normalized
     * and can be used for frustum culling.
     *
     * @param aspectRatio The viewport width-to-height ratio.
     * @return The camera's view frustum.
     */
    const Frustum& getFrustum(float aspectRatio) const;

private:
    mutable Frustum frustum;
};

/**
 * @brief A camera with movement and rotation controls.
 */
class MovingCamera : public Camera {
public:
    /**
     * @brief Constructs a moving camera.
     *
     * @param startPos The initial camera position.
     * @param worldUp The world's up direction.
     * @param yaw The initial yaw angle in degrees.
     * @param pitch The initial pitch angle in degrees.
     * @param fov The camera's field of view in degrees.
     * @param nearPlane The near clipping plane distance.
     * @param farPlane The far clipping plane distance.
     */
    MovingCamera(
        glm::vec3 startPos = glm::vec3(0.0f, 0.0f, 5.0f),
        glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f),
        float yaw = -90.0f,
        float pitch = 0.0f,
        float fov = 45.0f,
        float nearPlane = 0.1f,
        float farPlane = 100.0f
    );

    /**
     * @brief Moves the camera.
     *
     * @param direction The movement direction.
     * @param deltaTime The elapsed time since the previous frame.
     */
    void move(glm::vec3 direction, float deltaTime);

    /**
     * @brief Rotates the camera.
     *
     * @param xOffset The horizontal rotation offset.
     * @param yOffset The vertical rotation offset.
     * @param constrainPitch Whether to constrain the pitch angle.
     */
    void rotate(float xOffset, float yOffset, bool constrainPitch = true);

    float speed;
    float sensitivity;
};

} // namespace knot