#include <knot/mesh.h>
#include <iostream>

namespace knot {
Mesh::~Mesh() {
    if (sg_isvalid()) {
        sg_destroy_buffer(vertexBuffer);
        sg_destroy_buffer(indexBuffer);
    }
}

void Mesh::setup() {
    if (!sg_isvalid() || vertices.empty() || indices.empty()) {
        std::cerr << "[Error] Mesh::setup requires geometry and an initialized renderer\n";
        return;
    }
    glm::vec3 boundsMin = vertices.front().Position;
    glm::vec3 boundsMax = boundsMin;
    for (const auto& vertex : vertices) {
        boundsMin = glm::min(boundsMin, vertex.Position);
        boundsMax = glm::max(boundsMax, vertex.Position);
    }
    boundsCenter = (boundsMin + boundsMax) * 0.5f;

    sg_buffer_desc desc{};
    desc.data = {vertices.data(), vertices.size() * sizeof(Vertex)};
    desc.label = "mesh vertices";
    const auto newVertices = sg_make_buffer(desc);
    desc = {};
    desc.usage.index_buffer = true;
    desc.data = {indices.data(), indices.size() * sizeof(unsigned int)};
    desc.label = "mesh indices";
    const auto newIndices = sg_make_buffer(desc);
    if (sg_query_buffer_state(newVertices) != SG_RESOURCESTATE_VALID || sg_query_buffer_state(newIndices) != SG_RESOURCESTATE_VALID) {
        sg_destroy_buffer(newVertices);
        sg_destroy_buffer(newIndices);
        return;
    }
    sg_destroy_buffer(vertexBuffer);
    sg_destroy_buffer(indexBuffer);
    vertexBuffer = newVertices;
    indexBuffer = newIndices;
    indexCount = static_cast<unsigned int>(indices.size());
}
} // namespace knot
