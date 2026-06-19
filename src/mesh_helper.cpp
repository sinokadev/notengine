#include <glad/gl.h>
#include <knot/resources.h>
#include <knot/utility/mesh_helper.h>
#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

namespace knot {

// 탄젠트 유도를 위한 헬퍼 함수 (삼각형 UV 기준)
void calculateMeshTangents(std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
    for (auto& v : vertices) {
        v.Tangent = glm::vec3(0.0f);
    }

    for (size_t i = 0; i < indices.size(); i += 3) {
        Vertex& v0 = vertices[indices[i]];
        Vertex& v1 = vertices[indices[i + 1]];
        Vertex& v2 = vertices[indices[i + 2]];

        glm::vec3 edge1 = v1.Position - v0.Position;
        glm::vec3 edge2 = v2.Position - v0.Position;
        glm::vec2 deltaUV1 = v1.TexCoords - v0.TexCoords;
        glm::vec2 deltaUV2 = v2.TexCoords - v0.TexCoords;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        glm::vec3 tangent;
        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent = glm::normalize(tangent);

        v0.Tangent += tangent;
        v1.Tangent += tangent;
        v2.Tangent += tangent;
    }

    for (auto& v : vertices) {
        v.Tangent = glm::normalize(v.Tangent - v.Normal * glm::dot(v.Normal, v.Tangent));
    }
}

std::shared_ptr<Mesh> createCube() {
    auto mesh = std::make_shared<Mesh>();
    mesh->vertices = {// Front (+Z)
                      {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                      {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                      {{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                      {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                      // Back (-Z)
                      {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}},
                      {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}},
                      {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}},
                      {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}},
                      // Top (+Y)
                      {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                      {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                      {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                      {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                      // Bottom (-Y)
                      {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                      {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                      {{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                      {{-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                      // Left (-X)
                      {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                      {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                      {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                      {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                      // Right (+X)
                      {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
                      {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
                      {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
                      {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}};

    mesh->indices = {
        0,  1,  2,  2,  3,  0,  // Front
        4,  5,  6,  6,  7,  4,  // Back
        8,  9,  10, 10, 11, 8,  // Top
        12, 13, 14, 14, 15, 12, // Bottom
        16, 17, 18, 18, 19, 16, // Left
        20, 21, 22, 22, 23, 20  // Right
    };

    mesh->setup();
    return mesh;
}
std::shared_ptr<Mesh> createSphere(int sectors, int stacks) {
    auto mesh = std::make_shared<Mesh>();
    const float PI = 3.14159265359f;

    for (int i = 0; i <= stacks; ++i) {
        float stackAngle = PI / 2 - i * (PI / stacks);
        float xy = cosf(stackAngle);
        float z = sinf(stackAngle);

        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = j * (2.0f * PI / sectors);
            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);

            glm::vec3 pos(x * 0.5f, y * 0.5f, z * 0.5f);
            glm::vec3 normal(x, y, z);
            glm::vec2 uv((float)j / sectors, (float)i / stacks);

            glm::vec3 tangent(-xy * sinf(sectorAngle), xy * cosf(sectorAngle), 0.0f);

            if (glm::length(tangent) < 0.0001f) {
                tangent = glm::vec3(1.0f, 0.0f, 0.0f);
            } else {
                tangent = glm::normalize(tangent);
            }

            mesh->vertices.push_back({pos, uv, normal, tangent});
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            int first = (i * (sectors + 1)) + j;
            int second = first + sectors + 1;
            
            mesh->indices.push_back(first);
            mesh->indices.push_back(second);
            mesh->indices.push_back(first + 1);
            
            mesh->indices.push_back(second);
            mesh->indices.push_back(second + 1);
            mesh->indices.push_back(first + 1);
        }
    }
    
    mesh->setup();
    return mesh;
}

std::shared_ptr<Mesh> createPlane(float width, float height) {
    auto mesh = std::make_shared<Mesh>();
    float hw = width * 0.5f, hh = height * 0.5f;
    
    mesh->vertices = {{{-hw, -hh, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                      {{hw, -hh, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                      {{hw, hh, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                      {{-hw, hh, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}}};
    mesh->indices = {0, 1, 2, 2, 3, 0};
    mesh->setup();
    return mesh;
}

std::shared_ptr<Mesh> createRegularPolygon(int sectors, float radius) {
    if (sectors < 3) return nullptr;

    auto mesh = std::make_shared<Mesh>();
    const float PI = 3.14159265359f;

    mesh->vertices.push_back({{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}});

    for (int i = 0; i < sectors; ++i) {
        float angle = i * (2.0f * PI / sectors);
        float x = cosf(angle) * radius;
        float y = sinf(angle) * radius;

        mesh->vertices.push_back({{x, y, 0.0f}, {0.5f + 0.5f * cosf(angle), 0.5f + 0.5f * sinf(angle)}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}});
    }

    for (int i = 1; i <= sectors; ++i) {
        mesh->indices.push_back(0);
        mesh->indices.push_back((i == sectors) ? 1 : i + 1);
        mesh->indices.push_back(i);
    }

    mesh->setup();
    return mesh;
}

std::shared_ptr<Mesh> createMeshFromVertices(const std::vector<glm::vec3>& positions) {
    if (positions.size() < 3) return nullptr;

    auto mesh = std::make_shared<Mesh>();

    glm::vec3 center(0.0f);
    for (const auto& pos : positions) center += pos;
    center /= static_cast<float>(positions.size());

    mesh->vertices.push_back({center, {0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}});
    
    float maxDist = 0.001f;
    for (const auto& pos : positions) {
        maxDist = glm::max(maxDist, glm::distance(center, pos));
    }

    for (const auto& pos : positions) {
        glm::vec3 dir = pos - center;
        glm::vec2 uv = glm::vec2(dir.x, dir.y) / (maxDist * 2.0f) + glm::vec2(0.5f);

        mesh->vertices.push_back({pos, uv, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}});
    }

    int count = (int)positions.size();
    for (int i = 1; i <= count; ++i) {
        mesh->indices.push_back(0);
        mesh->indices.push_back((i == count) ? 1 : i + 1);
        mesh->indices.push_back(i);
    }

    calculateMeshTangents(mesh->vertices, mesh->indices);

    mesh->setup();
    return mesh;
}

std::shared_ptr<Mesh> loadModelOBJ(const std::string& filePath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str())) {
        std::cerr << "[Error] Failed to load the OBJ File: " << warn << err << std::endl;
        return nullptr;
    }

    auto mesh = std::make_shared<Mesh>();

    struct IndexTuple {
        int v_idx, vt_idx, vn_idx;
        bool operator<(const IndexTuple& other) const {
            if (v_idx != other.v_idx) return v_idx < other.v_idx;
            if (vt_idx != other.vt_idx) return vt_idx < other.vt_idx;
            return vn_idx < other.vn_idx;
        }
    };
    std::map<IndexTuple, unsigned int> uniqueVertices;

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            IndexTuple idxTuple = { index.vertex_index, index.texcoord_index, index.normal_index };

            if (uniqueVertices.count(idxTuple) == 0) {
                Vertex vertex{};

                vertex.Position = glm::vec3(
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                );

                if (index.texcoord_index >= 0) {
                    vertex.TexCoords = glm::vec2(
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                    );
                } else {
                    vertex.TexCoords = glm::vec2(0.0f);
                }

                if (index.normal_index >= 0) {
                    vertex.Normal = glm::vec3(
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]
                    );
                } else {
                    vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
                }

                uniqueVertices[idxTuple] = static_cast<unsigned int>(mesh->vertices.size());
                mesh->vertices.push_back(vertex);
            }

            mesh->indices.push_back(uniqueVertices[idxTuple]);
        }
    }

    if (!mesh->indices.empty()) {
        calculateMeshTangents(mesh->vertices, mesh->indices);
    }

    mesh->indexCount = static_cast<unsigned int>(mesh->indices.size());
    mesh->setup();

    return mesh;
}
} // namespace knot