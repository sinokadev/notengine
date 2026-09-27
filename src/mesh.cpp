#include <knot/mesh.h>
#include "vulkan_internal.h"
#include <stdexcept>
namespace knot {
Mesh::~Mesh() {
    auto& c = vk::context();
    auto it = c.meshes.find(gpuId);
    if (it != c.meshes.end()) {
        vkDeviceWaitIdle(c.device);
        c.destroy(it->second.vertices);
        c.destroy(it->second.indices);
        c.meshes.erase(it);
    }
}
void Mesh::setup() {
    if (vertices.empty() || indices.empty()) {
        indexCount = 0;
        return;
    }
    for (auto i : indices)
        if (i >= vertices.size())
            throw std::invalid_argument("Mesh index outside vertex array");
    auto& c = vk::context();
    if (!c.device)
        throw std::runtime_error("Initialize Renderer before uploading meshes");
    vk::Geometry next{};
    try {
        next.vertices = c.buffer(vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices.data());
        next.indices = c.buffer(indices.size() * sizeof(unsigned), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indices.data());
    } catch (...) {
        c.destroy(next.vertices);
        c.destroy(next.indices);
        throw;
    }
    auto it = c.meshes.find(gpuId);
    if (it != c.meshes.end()) {
        vkDeviceWaitIdle(c.device);
        c.destroy(it->second.vertices);
        c.destroy(it->second.indices);
        c.meshes.erase(it);
    }
    gpuId = c.nextId++;
    c.meshes.emplace(gpuId, next);
    indexCount = static_cast<unsigned>(indices.size());
}
} // namespace knot
