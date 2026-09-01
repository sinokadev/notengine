// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <knot/resources.h>

namespace knot {

/**
 * @brief Represents a plane of a view frustum.
 */
struct FrustumPlane {
    /** @brief Unit-length plane normal pointing into the frustum. */
    glm::vec3 normal{0.0f};

    /** @brief Plane distance in the equation dot(normal, point) + distance. */
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
    /** @brief Clipping planes indexed by Plane. */
    FrustumPlane planes[6];

    /** @brief Indices of the six view-frustum clipping planes. */
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
    bool intersectsSphere(
        const glm::vec3& center,
        float radius
    ) const;

    /**
     * @brief Tests whether an axis-aligned bounding box intersects the frustum.
     *
     * @param min The minimum corner of the AABB.
     * @param max The maximum corner of the AABB.
     * @return true if the AABB intersects or is inside the frustum.
     */
    bool intersectsAABB(
        const glm::vec3& min,
        const glm::vec3& max
    ) const;
};

/**
 * @brief Base class for cameras.
 *
 * Provides the common interface and clipping-plane configuration shared
 * by all camera types.
 */
class Camera : public Transform {
public:
    /** @brief Default near clipping distance. */
    static constexpr float kNearPlane = 0.1f;

    /** @brief Default far clipping distance. */
    static constexpr float kFarPlane = 100.0f;

    /**
     * @brief Creates a camera at the specified position.
     *
     * @param startPos The initial camera position.
     * @param nearPlane The near clipping plane distance.
     * @param farPlane The far clipping plane distance.
     */
    Camera(
        glm::vec3 startPos = glm::vec3(0.0f, 0.0f, 3.0f),
        float nearPlane = kNearPlane,
        float farPlane = kFarPlane
    );

    virtual ~Camera() = default;

    /**
     * @brief Gets the view matrix.
     *
     * @return The view matrix.
     */
    virtual glm::mat4 getViewMatrix() const = 0;

    /**
     * @brief Gets the projection matrix.
     *
     * @param aspectRatio The viewport width-to-height ratio.
     * @return The projection matrix.
     */
    virtual glm::mat4 getProjectionMatrix(float aspectRatio) const = 0;

    /**
     * @brief Rotates the camera to look at a target position.
     *
     * Updates the camera's quaternion rotation so that its front
     * direction points toward the specified target position.
     *
     * @param targetPos The position for the camera to look at.
     */
    void lookAtTarget(glm::vec3 targetPos);

    /**
     * @brief Gets the camera's view frustum.
     *
     * Calculates the six clipping planes from the camera's view and
     * projection matrices.
     *
     * @param aspectRatio The viewport width-to-height ratio.
     * @return The camera's view frustum.
     */
    const Frustum& getFrustum(float aspectRatio) const;

    /** @brief Distance to the near clipping plane. */
    float nearPlane = kNearPlane;

    /** @brief Distance to the far clipping plane. */
    float farPlane = kFarPlane;

protected:
    /** @brief Cached view frustum used for frustum culling. */
    mutable Frustum frustum;
};

/**
 * @brief Perspective camera defined by a transform.
 */
class PerspectiveCamera : public Camera {
public:
    /**
     * @brief Creates a perspective camera.
     *
     * @param startPos The initial camera position.
     * @param fov The vertical field of view in degrees.
     * @param nearPlane The near clipping plane distance.
     * @param farPlane The far clipping plane distance.
     */
    PerspectiveCamera(
        glm::vec3 startPos = glm::vec3(0.0f, 0.0f, 3.0f),
        float fov = 45.0f,
        float nearPlane = kNearPlane,
        float farPlane = kFarPlane
    );

    /**
     * @brief Gets the perspective view matrix.
     *
     * @return The view matrix.
     */
    glm::mat4 getViewMatrix() const override;

    /**
     * @brief Gets the perspective projection matrix.
     *
     * @param aspectRatio The viewport width-to-height ratio.
     * @return The projection matrix.
     */
    glm::mat4 getProjectionMatrix(float aspectRatio) const override;

    /** @brief Vertical field of view in degrees. */
    float fov = 45.0f;
};

/**
 * @brief A perspective camera with movement and FPS-style rotation controls.
 *
 * Unlike other Transform-based objects, MovingCamera does not use
 * Transform::rotation for its orientation. Its orientation is represented
 * by yaw and pitch angles, which are used directly to calculate the view
 * matrix and camera direction vectors.
 *
 * Transform::rotation should not be modified or used to control the
 * orientation of a MovingCamera.
 */
class MovingCamera : public PerspectiveCamera {
public:
    /**
     * @brief Constructs a moving perspective camera.
     *
     * @param startPos The initial camera position.
     * @param fov The camera's field of view in degrees.
     * @param nearPlane The near clipping plane distance.
     * @param farPlane The far clipping plane distance.
     */
    MovingCamera(
        glm::vec3 startPos = glm::vec3(0.0f, 0.0f, 5.0f),
        float fov = 45.0f,
        float nearPlane = kNearPlane,
        float farPlane = kFarPlane
    ); // 솔직히 MovingCamera만 Transform::rotate를 안쓰고 yaw, pitch를 쓴다는거, 진짜 별로인 생각같지만 이렇게 안하면 어떻게 해야할지 모르겠어. 그치만 뭐... 버그가 난다면 누군가 고쳐주겠지.

    /**
     * @brief Moves the camera.
     *
     * @param direction The movement direction.
     * @param deltaTime The elapsed time since the previous frame.
     */
    void move(
        glm::vec3 direction,
        float deltaTime
    );

    /**
     * @brief Rotates the camera.
     *
     * @param xOffset The horizontal rotation offset.
     * @param yOffset The vertical rotation offset.
     */
    void rotate(
        float xOffset,
        float yOffset
    );

    /**
     * @brief Gets the camera's view matrix.
     *
     * Uses yaw and pitch instead of Transform::rotation.
     *
     * @return The view matrix.
     */
    glm::mat4 getViewMatrix() const override;

    /**
     * @brief Gets the camera's forward direction.
     *
     * @return Normalized forward direction.
     */
    glm::vec3 getFront() const;

    /**
     * @brief Gets the camera's right direction.
     *
     * @return Normalized right direction.
     */
    glm::vec3 getRight() const;

    /**
     * @brief Gets the camera's up direction.
     *
     * @return Normalized up direction.
     */
    glm::vec3 getUp() const;

    /** @brief Movement speed in world units per second. */
    float speed;

    /** @brief Multiplier applied to rotation input offsets. */
    float sensitivity;

private:
    /** @brief Horizontal camera rotation in degrees. */
    float yaw = -90.0f;

    /** @brief Vertical camera rotation in degrees. */
    float pitch = 0.0f;
};

/**
 * @brief Orthographic camera defined by a transform.
 *
 * Uses an orthographic projection where objects retain their apparent
 * size regardless of their distance from the camera.
 */
class OrthographicCamera : public Camera {
public:
    /**
     * @brief Creates an orthographic camera.
     *
     * @param startPos The initial camera position.
     * @param nearPlane The near clipping plane distance.
     * @param farPlane The far clipping plane distance.
     * @param size The vertical size of the orthographic view.
     */
    OrthographicCamera(
        glm::vec3 startPos = glm::vec3(0.0f, 0.0f, 3.0f),
        float nearPlane = kNearPlane,
        float farPlane = kFarPlane,
        float size = 10.0f
    );

    /**
     * @brief Gets the orthographic view matrix.
     *
     * @return The view matrix.
     */
    glm::mat4 getViewMatrix() const override;

    /**
     * @brief Gets the orthographic projection matrix.
     *
     * @param aspectRatio The viewport width-to-height ratio.
     * @return The projection matrix.
     */
    glm::mat4 getProjectionMatrix(float aspectRatio) const override;

    /** @brief Vertical size of the orthographic view. */
    float size = 10.0f;
};

} // namespace knot