#include "test_support.h"
#include <knot/camera.h>

namespace {
void perspectiveProjection() {
    knot::PerspectiveCamera camera({0, 0, 0}, 90, 1, 10);
    const auto projection = camera.getProjectionMatrix(2);
    CHECK(near(project(projection, {0, 0, -1}), {0, 0, -1}));
    CHECK(near(project(projection, {0, 0, -10}), {0, 0, 1}));
    CHECK(near(project(projection, {4, 2, -2}), {1, 1, 1.0f / 9.0f}));
}

void orthographicProjection() {
    knot::OrthographicCamera camera({0, 0, 0}, 1, 11, 4);
    const auto projection = camera.getProjectionMatrix(2);
    CHECK(near(project(projection, {-4, -2, -1}), {-1, -1, -1}));
    CHECK(near(project(projection, {4, 2, -11}), {1, 1, 1}));
    CHECK(near(project(projection, {0, 0, -6}), {0, 0, 0}));
}

void viewAndLookAt() {
    knot::PerspectiveCamera camera({3, 2, 5});
    CHECK(near(project(camera.getViewMatrix(), camera.position), {0, 0, 0}));
    camera.lookAtTarget({8, 2, 5});
    CHECK(near(camera.getFront(), {1, 0, 0}));
    CHECK(near(project(camera.getViewMatrix(), {8, 2, 5}), {0, 0, -5}));
    camera.lookAtTarget(camera.position);
    CHECK(near(camera.getFront(), {1, 0, 0}));
    knot::OrthographicCamera orthographic(camera.position);
    orthographic.rotation = camera.rotation;
    CHECK(near(project(orthographic.getViewMatrix(), {8, 2, 5}), {0, 0, -5}));
}

void movementAndRotation() {
    knot::MovingCamera camera({0, 0, 0});
    camera.speed = 0.002f;
    camera.move({1, 0, -1}, 0.5f);
    CHECK(near(camera.position, {1, 0, -1}));
    camera.move({1, 1, 1}, 0);
    CHECK(near(camera.position, {1, 0, -1}));
    camera.sensitivity = 1;
    camera.rotate(90, 0);
    CHECK(near(camera.getFront(), {1, 0, 0}));
    CHECK(near(project(camera.getViewMatrix(), camera.position + camera.getFront()), {0, 0, -1}));
    camera.rotate(0, 1000);
    CHECK(near(camera.getFront().y, std::sin(glm::radians(89.0f))));
    CHECK(near(glm::length(camera.getRight()), 1));
    CHECK(near(glm::length(camera.getUp()), 1));
    CHECK(near(glm::dot(camera.getFront(), camera.getUp()), 0));
    camera.rotate(0, -2000);
    CHECK(near(camera.getFront().y, -std::sin(glm::radians(89.0f))));
}

knot::Frustum unitBox() {
    knot::Frustum frustum;
    frustum.planes[knot::Frustum::Left] = {{1, 0, 0}, 1};
    frustum.planes[knot::Frustum::Right] = {{-1, 0, 0}, 1};
    frustum.planes[knot::Frustum::Bottom] = {{0, 1, 0}, 1};
    frustum.planes[knot::Frustum::Top] = {{0, -1, 0}, 1};
    frustum.planes[knot::Frustum::Near] = {{0, 0, 1}, 1};
    frustum.planes[knot::Frustum::Far] = {{0, 0, -1}, 1};
    return frustum;
}

void frustumBoundaryIntersections() {
    const auto frustum = unitBox();
    CHECK(near(frustum.planes[0].signedDistance({0, 0, 0}), 1));
    CHECK(frustum.intersectsSphere({0, 0, 0}, 0));
    CHECK(frustum.intersectsAABB({-2, -2, -2}, {2, 2, 2}));
    for (int axis = 0; axis < 3; ++axis) {
        for (float sign : {-1.0f, 1.0f}) {
            glm::vec3 center(0);
            center[axis] = sign * 1.5f;
            CHECK(frustum.intersectsSphere(center, 0.5f));
            CHECK(frustum.intersectsAABB(center - glm::vec3(0.5f), center + glm::vec3(0.5f)));
            center[axis] = sign * 1.51f;
            CHECK(!frustum.intersectsSphere(center, 0.5f));
            CHECK(!frustum.intersectsAABB(center - glm::vec3(0.5f), center + glm::vec3(0.5f)));
        }
    }
}

void extractedFrustumUpdates() {
    knot::PerspectiveCamera camera({0, 0, 0}, 90, 1, 10);
    auto frustum = camera.getFrustum(1);
    for (const auto& plane : frustum.planes)
        CHECK(near(glm::length(plane.normal), 1));
    CHECK(frustum.intersectsSphere({0, 0, -5}, 0.1f));
    CHECK(!frustum.intersectsSphere({0, 0, 1}, 0.1f));
    CHECK(!frustum.intersectsSphere({0, 0, -11}, 0.1f));
    CHECK(!frustum.intersectsSphere({7, 0, -5}, 0.1f));
    CHECK(camera.getFrustum(2).intersectsSphere({7, 0, -5}, 0.1f));
    camera.position = {20, 0, 0};
    CHECK(!camera.getFrustum(1).intersectsSphere({0, 0, -5}, 0.1f));
    CHECK(camera.getFrustum(1).intersectsSphere({20, 0, -5}, 0.1f));
    camera.lookAtTarget({25, 0, 0});
    CHECK(camera.getFrustum(1).intersectsSphere({25, 0, 0}, 0.1f));
    CHECK(!camera.getFrustum(1).intersectsSphere({20, 0, -5}, 0.1f));
}
} // namespace

int main() {
    return runTests({{"perspective projection", perspectiveProjection}, {"orthographic projection", orthographicProjection},
                     {"view and look-at", viewAndLookAt}, {"movement and rotation", movementAndRotation},
                     {"frustum boundary intersections", frustumBoundaryIntersections}, {"extracted frustum updates", extractedFrustumUpdates}});
}
