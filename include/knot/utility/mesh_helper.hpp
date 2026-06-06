#pragma once
#include <glad/gl.h>
#include <knot/resources.h>


namespace knot {
    class MeshGen {
    public:
static Mesh createCube() {
            Mesh mesh;
            mesh.vertices = {
                // Front (+Z)
                {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                // Back (-Z)
                {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}},
                {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}},
                {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}},
                {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}},
                // Top (+Y)
                {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                // Bottom (-Y)
                {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                // Left (-X)
                {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                // Right (+X)
                {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
                {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
                {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
                {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}
            };

            mesh.indices = {
                0, 1, 2, 2, 3, 0,       // Front
                4, 5, 6, 6, 7, 4,       // Back
                8, 9, 10, 10, 11, 8,    // Top
                12, 13, 14, 14, 15, 12, // Bottom
                16, 17, 18, 18, 19, 16, // Left
                20, 21, 22, 22, 23, 20  // Right
            };

            setupMesh(mesh);
            return mesh;
        }
static Mesh createSphere(int sectors, int stacks) {
            Mesh mesh;
            const float PI = 3.14159265359f;
            
            for (int i = 0; i <= stacks; ++i) {
                float stackAngle = PI / 2 - i * (PI / stacks); // 위에서 아래로
                float xy = cosf(stackAngle);
                float z = sinf(stackAngle);

                for (int j = 0; j <= sectors; ++j) {
                    float sectorAngle = j * (2.0f * PI / sectors);
                    float x = xy * cosf(sectorAngle);
                    float y = xy * sinf(sectorAngle);

                    glm::vec3 pos(x * 0.5f, y * 0.5f, z * 0.5f);
                    glm::vec3 normal(x, y, z);
                    glm::vec2 uv((float)j / sectors, (float)i / stacks);
                    
                    mesh.vertices.push_back({pos, uv, normal, {0,0,0}});
                }
            }

            for (int i = 0; i < stacks; ++i) {
                for (int j = 0; j < sectors; ++j) {
                    int first = (i * (sectors + 1)) + j;
                    int second = first + sectors + 1;
                    mesh.indices.push_back(first); mesh.indices.push_back(second); mesh.indices.push_back(first + 1);
                    mesh.indices.push_back(second); mesh.indices.push_back(second + 1); mesh.indices.push_back(first + 1);
                }
            }
            setupMesh(mesh);
            return mesh;
        }

        static Mesh createPlane(float width, float height) {
            Mesh mesh;
            float hw = width * 0.5f, hh = height * 0.5f;
            mesh.vertices = {
                {{-hw, -hh, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                {{ hw, -hh, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                {{ hw,  hh, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                {{-hw,  hh, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}}
            };
            mesh.indices = { 0, 1, 2, 2, 3, 0 };
            setupMesh(mesh);
            return mesh;
        }


        static Mesh createRegularPolygon(int sectors, float radius) {
            Mesh mesh;
            const float PI = 3.14159265359f;

            // 1. 중심점 추가 (Fan 구조를 위해 필요)
            mesh.vertices.push_back({{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}});

            // 2. 외곽 정점들 추가
            for (int i = 0; i < sectors; ++i) {
                float angle = i * (2.0f * PI / sectors);
                float x = cosf(angle) * radius;
                float y = sinf(angle) * radius;
                
                mesh.vertices.push_back({{x, y, 0.0f}, 
                                         {0.5f + 0.5f * cosf(angle), 0.5f + 0.5f * sinf(angle)}, 
                                         {0.0f, 0.0f, 1.0f}, 
                                         {1.0f, 0.0f, 0.0f}});
            }

            // 3. 인덱스 생성 (중심점 0번과 연결하는 삼각형들)
            for (int i = 1; i <= sectors; ++i) {
                mesh.indices.push_back(0);
                mesh.indices.push_back(i);
                mesh.indices.push_back((i == sectors) ? 1 : i + 1);
            }

            setupMesh(mesh);
            return mesh;
        }

        static Mesh createMeshFromVertices(const std::vector<glm::vec3>& positions) {
            Mesh mesh;
            
            // 중심점 계산 (모든 정점의 평균)
            glm::vec3 center(0.0f);
            for (const auto& pos : positions) center += pos;
            center /= (float)positions.size();

            // 1. 정점 데이터 입력 (중심점 + 외곽 정점들)
            mesh.vertices.push_back({center, {0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}});
            for (const auto& pos : positions) {
                mesh.vertices.push_back({pos, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}});
            }

            // 2. 인덱스 자동 생성 (Triangle Fan)
            int count = (int)positions.size();
            for (int i = 1; i <= count; ++i) {
                mesh.indices.push_back(0); // 중심점
                mesh.indices.push_back(i);
                mesh.indices.push_back((i == count) ? 1 : i + 1); // 마지막이면 다시 1번으로
            }

            setupMesh(mesh);
            return mesh;
        }
    };
    
    inline void setupMesh(Mesh& mesh) {
        glGenVertexArrays(1, &mesh.vao);
        glGenBuffers(1, &mesh.vbo);
        glGenBuffers(1, &mesh.ebo);

        glBindVertexArray(mesh.vao);

        glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
        glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), mesh.indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
        
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));

        glBindVertexArray(0);
        mesh.indexCount = static_cast<unsigned int>(mesh.indices.size());
    }
}