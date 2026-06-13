#include <glad/gl.h>
#include <knot/resources.h>
#include <knot/utility/mesh_helper.h>

namespace knot {

std::shared_ptr<Mesh> createCube() {
    auto mesh = std::make_shared<Mesh>();
    mesh->vertices = {// Front (+Z)
                      {{-0.5f, -0.5f, 0.5f},
                       {0.0f, 0.0f},
                       {0.0f, 0.0f, 1.0f},
                       {1.0f, 0.0f, 0.0f}},
                      {{0.5f, -0.5f, 0.5f},
                       {1.0f, 0.0f},
                       {0.0f, 0.0f, 1.0f},
                       {1.0f, 0.0f, 0.0f}},
                      {{0.5f, 0.5f, 0.5f},
                       {1.0f, 1.0f},
                       {0.0f, 0.0f, 1.0f},
                       {1.0f, 0.0f, 0.0f}},
                      {{-0.5f, 0.5f, 0.5f},
                       {0.0f, 1.0f},
                       {0.0f, 0.0f, 1.0f},
                       {1.0f, 0.0f, 0.0f}},
                      // Back (-Z)
                      {{0.5f, -0.5f, -0.5f},
                       {0.0f, 0.0f},
                       {0.0f, 0.0f, -1.0f},
                       {-1.0f, 0.0f, 0.0f}},
                      {{-0.5f, -0.5f, -0.5f},
                       {1.0f, 0.0f},
                       {0.0f, 0.0f, -1.0f},
                       {-1.0f, 0.0f, 0.0f}},
                      {{-0.5f, 0.5f, -0.5f},
                       {1.0f, 1.0f},
                       {0.0f, 0.0f, -1.0f},
                       {-1.0f, 0.0f, 0.0f}},
                      {{0.5f, 0.5f, -0.5f},
                       {0.0f, 1.0f},
                       {0.0f, 0.0f, -1.0f},
                       {-1.0f, 0.0f, 0.0f}},
                      // Top (+Y)
                      {{-0.5f, 0.5f, 0.5f},
                       {0.0f, 0.0f},
                       {0.0f, 1.0f, 0.0f},
                       {1.0f, 0.0f, 0.0f}},
                      {{0.5f, 0.5f, 0.5f},
                       {1.0f, 0.0f},
                       {0.0f, 1.0f, 0.0f},
                       {1.0f, 0.0f, 0.0f}},
                      {{0.5f, 0.5f, -0.5f},
                       {1.0f, 1.0f},
                       {0.0f, 1.0f, 0.0f},
                       {1.0f, 0.0f, 0.0f}},
                      {{-0.5f, 0.5f, -0.5f},
                       {0.0f, 1.0f},
                       {0.0f, 1.0f, 0.0f},
                       {1.0f, 0.0f, 0.0f}},
                      // Bottom (-Y)
                      {{-0.5f, -0.5f, -0.5f},
                       {0.0f, 0.0f},
                       {0.0f, -1.0f, 0.0f},
                       {1.0f, 0.0f, 0.0f}},
                      {{0.5f, -0.5f, -0.5f},
                       {1.0f, 0.0f},
                       {0.0f, -1.0f, 0.0f},
                       {1.0f, 0.0f, 0.0f}},
                      {{0.5f, -0.5f, 0.5f},
                       {1.0f, 1.0f},
                       {0.0f, -1.0f, 0.0f},
                       {1.0f, 0.0f, 0.0f}},
                      {{-0.5f, -0.5f, 0.5f},
                       {0.0f, 1.0f},
                       {0.0f, -1.0f, 0.0f},
                       {1.0f, 0.0f, 0.0f}},
                      // Left (-X)
                      {{-0.5f, -0.5f, -0.5f},
                       {0.0f, 0.0f},
                       {-1.0f, 0.0f, 0.0f},
                       {0.0f, 0.0f, 1.0f}},
                      {{-0.5f, -0.5f, 0.5f},
                       {1.0f, 0.0f},
                       {-1.0f, 0.0f, 0.0f},
                       {0.0f, 0.0f, 1.0f}},
                      {{-0.5f, 0.5f, 0.5f},
                       {1.0f, 1.0f},
                       {-1.0f, 0.0f, 0.0f},
                       {0.0f, 0.0f, 1.0f}},
                      {{-0.5f, 0.5f, -0.5f},
                       {0.0f, 1.0f},
                       {-1.0f, 0.0f, 0.0f},
                       {0.0f, 0.0f, 1.0f}},
                      // Right (+X)
                      {{0.5f, -0.5f, 0.5f},
                       {0.0f, 0.0f},
                       {1.0f, 0.0f, 0.0f},
                       {0.0f, 0.0f, -1.0f}},
                      {{0.5f, -0.5f, -0.5f},
                       {1.0f, 0.0f},
                       {1.0f, 0.0f, 0.0f},
                       {0.0f, 0.0f, -1.0f}},
                      {{0.5f, 0.5f, -0.5f},
                       {1.0f, 1.0f},
                       {1.0f, 0.0f, 0.0f},
                       {0.0f, 0.0f, -1.0f}},
                      {{0.5f, 0.5f, 0.5f},
                       {0.0f, 1.0f},
                       {1.0f, 0.0f, 0.0f},
                       {0.0f, 0.0f, -1.0f}}};

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

            mesh->vertices.push_back({pos, uv, normal, {0, 0, 0}});
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
    mesh->vertices = {
        {{-hw, -hh, 0.0f},
         {0.0f, 0.0f},
         {0.0f, 0.0f, 1.0f},
         {1.0f, 0.0f, 0.0f}},
        {{hw, -hh, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
        {{hw, hh, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
        {{-hw, hh, 0.0f},
         {0.0f, 1.0f},
         {0.0f, 0.0f, 1.0f},
         {1.0f, 0.0f, 0.0f}}};
    mesh->indices = {0, 1, 2, 2, 3, 0};
    mesh->setup();
    return mesh;
}

std::shared_ptr<Mesh> createRegularPolygon(int sectors, float radius) {
    if (sectors < 3) {
        return nullptr;
    }

    auto mesh = std::make_shared<Mesh>();
    const float PI = 3.14159265359f;

    // 1. 중심점 추가 (Fan 구조를 위해 필요)
    mesh->vertices.push_back({{0.0f, 0.0f, 0.0f},
                              {0.5f, 0.5f},
                              {0.0f, 0.0f, 1.0f},
                              {1.0f, 0.0f, 0.0f}});

    // 2. 외곽 정점들 추가
    for (int i = 0; i < sectors; ++i) {
        float angle = i * (2.0f * PI / sectors);
        float x = cosf(angle) * radius;
        float y = sinf(angle) * radius;

        mesh->vertices.push_back(
            {{x, y, 0.0f},
             {0.5f + 0.5f * cosf(angle), 0.5f + 0.5f * sinf(angle)},
             {0.0f, 0.0f, 1.0f},
             {1.0f, 0.0f, 0.0f}});
    }

    // 3. 인덱스 생성 (중심점 0번과 연결하는 삼각형들)
    for (int i = 1; i <= sectors; ++i) {
        mesh->indices.push_back(0);
        mesh->indices.push_back(i);
        mesh->indices.push_back((i == sectors) ? 1 : i + 1);
    }

    mesh->setup();
    return mesh;
}

std::shared_ptr<Mesh>
createMeshFromVertices(const std::vector<glm::vec3> &positions) {
    if (positions.size() < 3) {
        return nullptr;
    }

    auto mesh = std::make_shared<Mesh>();

    glm::vec3 center(0.0f);
    for (const auto &pos : positions) {
        center += pos;
    }
    center /= static_cast<float>(positions.size());

    // 1. 정점 데이터 입력 (중심점 + 외곽 정점들)
    mesh->vertices.push_back(
        {center, {0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}});
    for (const auto &pos : positions) {
        mesh->vertices.push_back(
            {pos, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}});
    }

    // 2. 인덱스 자동 생성 (Triangle Fan)
    int count = (int)positions.size();
    for (int i = 1; i <= count; ++i) {
        mesh->indices.push_back(0); // 중심점
        mesh->indices.push_back(i);
        mesh->indices.push_back(
            (i == count) ? 1 : i + 1); // 마지막이면 다시 1번으로
    }

    mesh->setup();
    return mesh;
}
} // namespace knot