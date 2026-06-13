#pragma once

#include <glad/gl.h>
#include <knot/resources.h>


namespace knot {
class Mesh;

std::shared_ptr<Mesh> createCube();
std::shared_ptr<Mesh> createSphere(int sectors, int stacks);
std::shared_ptr<Mesh> createPlane(float width, float height);
std::shared_ptr<Mesh> createRegularPolygon(int sectors, float radius);
std::shared_ptr<Mesh> createMeshFromVertices(const std::vector<glm::vec3>& positions);
}