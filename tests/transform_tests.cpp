#include "test_support.h"
#include <knot/resources.h>
#include <knot/mesh.h>

namespace {
void identityAndDirections() {
    knot::Transform transform;
    CHECK(near(project(transform.getWorldMatrix(), {2, 3, 4}), {2, 3, 4}));
    CHECK(near(transform.getFront(), {0, 0, -1}));
    CHECK(near(transform.getRight(), {1, 0, 0}));
    CHECK(near(transform.getUp(), {0, 1, 0}));
}

void transformOrderAndPivot() {
    knot::Transform transform;
    transform.position = {10, 20, 30};
    transform.scale = {2, 3, 4};
    transform.rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 0, 1));
    CHECK(near(project(transform.getWorldMatrix(), {1, 1, 1}), {7, 22, 34}));
    transform.pivot = {1, 1, 1};
    CHECK(near(project(transform.getWorldMatrix(), transform.pivot), {11, 21, 31}));
    CHECK(near(project(transform.getWorldMatrix(), {2, 1, 1}), {11, 23, 31}));
}

void rotatedBasis() {
    knot::Transform transform;
    transform.rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 1, 0));
    transform.scale = {-2, 0, 3};
    CHECK(near(transform.getFront(), {-1, 0, 0}));
    CHECK(near(transform.getRight(), {0, 0, -1}));
    CHECK(near(transform.getUp(), {0, 1, 0}));
}

std::shared_ptr<knot::Mesh> mesh(std::initializer_list<glm::vec3> positions) {
    auto result = std::make_shared<knot::Mesh>();
    for (const auto& position : positions) {
        knot::Vertex vertex{};
        vertex.Position = position;
        result->vertices.push_back(vertex);
    }
    return result; // CPU data only: never call Mesh::setup().
}

void boundsAcrossSubmeshes() {
    knot::Model model(std::vector<knot::SubMesh>{{mesh({{-2, -1, 0}}), nullptr}, {nullptr, nullptr},
                                              {mesh({{4, 3, 0}}), nullptr}, {mesh({}), nullptr}});
    CHECK(near(model.boundsCenter, {1, 1, 0}));
    CHECK(near(model.boundsRadius, std::sqrt(13.0f)));
    CHECK(model.setMesh(mesh({{-4, -1, 0}}), 0));
    CHECK(near(model.boundsCenter, {0, 1, 0}));
    CHECK(near(model.boundsRadius, std::sqrt(20.0f)));
    CHECK(!model.setMesh(nullptr, 99));
    CHECK(model.getMesh(99) == nullptr);
    CHECK(model.getMaterial(99) == nullptr);
    CHECK(!model.setMaterial(nullptr, 99));
}

void emptyBoundsAndObjectAccess() {
    knot::Model model(mesh({{5, 6, 7}}), nullptr);
    CHECK(near(model.boundsCenter, {5, 6, 7}));
    CHECK(near(model.boundsRadius, 0));
    CHECK(model.setMesh(nullptr));
    CHECK(near(model.boundsCenter, {0, 0, 0}));
    CHECK(near(model.boundsRadius, 0));
    knot::Object object;
    CHECK(object.getMesh() == nullptr);
    CHECK(object.getMaterial() == nullptr);
    CHECK(!object.setMesh(nullptr));
    CHECK(!object.setMaterial(nullptr));
    object.model = std::make_shared<knot::Model>(mesh({{1, 2, 3}}), nullptr);
    const auto replacement = mesh({{3, 2, 1}});
    CHECK(object.setMesh(replacement));
    CHECK(object.getMesh() == replacement);
    CHECK(near(object.model->boundsCenter, {3, 2, 1}));
}
} // namespace

int main() {
    return runTests({{"identity and directions", identityAndDirections}, {"transform order and pivot", transformOrderAndPivot},
                     {"rotated basis", rotatedBasis}, {"bounds across submeshes", boundsAcrossSubmeshes},
                     {"empty bounds and object access", emptyBoundsAndObjectAccess}});
}
