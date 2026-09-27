#include <knot/resources.h>
#include <knot/camera.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <cmath>
#include <limits>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "GeneratedShaders.h"

namespace knot {

unsigned int createSolidColorTexture(glm::vec3 color);

namespace {

#ifdef NOTENGINE_ASSET_ROOT
std::string assetRoot = []() {
    std::string root = NOTENGINE_ASSET_ROOT;
    if (!root.empty() && root.back() != '/') {
        root += '/';
    }
    return root;
}();
#else
std::string assetRoot = "";
#endif

} // namespace

void setAssetRoot(const std::string& root) {
    assetRoot = root;
    if (!assetRoot.empty() && assetRoot.back() != '/') {
        assetRoot += '/';
    }
}

const std::string& getAssetRoot() {
    return assetRoot;
}

ShaderSource::ShaderSource(std::string v, std::string f) : vertexPath(v), fragmentPath(f) {
    // 1. 파일 이름만 추출 (디렉토리 경로 제거)
    std::string vName = std::filesystem::path(v).filename().string();
    std::string fName = std::filesystem::path(f).filename().string();

    // 2. 파일 이름으로 Registry 검색
    if (Shaders::Registry.count(vName)) {
        vertexSourceCode = Shaders::Registry.at(vName);
    } else {
        std::cerr << "[Error] Failed to find shader in Registry: " << vName << std::endl;
    }

    if (Shaders::Registry.count(fName)) {
        fragmentSourceCode = Shaders::Registry.at(fName);
    } else {
        std::cerr << "[Error] Failed to find shader in Registry: " << fName << std::endl;
    }
}
bool ShaderSource::isValid() const {
    return !vertexSourceCode.empty() && !fragmentSourceCode.empty();
}

Shader::Shader(std::shared_ptr<ShaderSource> ss, unsigned int shaderId) : id(shaderId) {
    if (!ss || !ss->isValid()) return;
    const auto vertex = std::filesystem::path(ss->vertexPath).filename().string();
    const auto fragment = std::filesystem::path(ss->fragmentPath).filename().string();
    pbr = fragment == "pbr.frag";
    valid = vertex == "alpha.vert" && (pbr || fragment == "alpha.frag");
    if (!valid) std::cerr << "[Error] Vulkan material shaders must use alpha.vert with alpha.frag or pbr.frag\n";
}

ShaderSource AlphaShader::GetSource() {
    return ShaderSource(getAssetRoot() + "shaders/alpha.vert", getAssetRoot() + "shaders/alpha.frag");
}

ShaderSource PbrShader::GetSource() {
    return ShaderSource(getAssetRoot() + "shaders/alpha.vert", getAssetRoot() + "shaders/pbr.frag");
}

Model::Model(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material) {
    if (mesh || material) {
        subMeshes.emplace_back(std::move(mesh), std::move(material));
    }
    calculateBounds();
}

Model::Model(std::vector<SubMesh> subMeshes) : subMeshes(std::move(subMeshes)) {
    calculateBounds();
}

void Model::calculateBounds() {
    bool hasVertices = false;
    glm::vec3 minPos(std::numeric_limits<float>::max());
    glm::vec3 maxPos(std::numeric_limits<float>::lowest());

    for (const auto& subMesh : subMeshes) {
        if (!subMesh.mesh || subMesh.mesh->vertices.empty())
            continue;

        for (const auto& v : subMesh.mesh->vertices) {
            minPos = glm::min(minPos, v.Position);
            maxPos = glm::max(maxPos, v.Position);
            hasVertices = true;
        }
    }

    if (!hasVertices) {
        boundsCenter = glm::vec3(0.0f);
        boundsRadius = 0.0f;
        return;
    }

    boundsCenter = (minPos + maxPos) * 0.5f;

    float maxDistSq = 0.0f;
    for (const auto& subMesh : subMeshes) {
        if (!subMesh.mesh || subMesh.mesh->vertices.empty())
            continue;

        for (const auto& v : subMesh.mesh->vertices) {
            glm::vec3 diff = v.Position - boundsCenter;
            float distSq = glm::dot(diff, diff);
            if (distSq > maxDistSq) {
                maxDistSq = distSq;
            }
        }
    }
    boundsRadius = std::sqrt(maxDistSq);
}

glm::mat4 Transform::getWorldMatrix() const {
    glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 R = glm::mat4_cast(rotation);
    glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
    glm::mat4 toPivot = glm::translate(glm::mat4(1.0f), pivot);
    glm::mat4 fromPivot = glm::translate(glm::mat4(1.0f), -pivot);

    return T * toPivot * R * S * fromPivot;
}

bool Object::isVisible(const Frustum& frustum) const {
    return isVisible(frustum, getWorldMatrix());
}

bool Object::isVisible(const Frustum& frustum, const glm::mat4& worldMatrix) const {
    if (!model)
        return false;

    if (model->boundsRadius <= 0.0f) {
        model->calculateBounds();
    }

    glm::vec3 center = glm::vec3(worldMatrix * glm::vec4(model->boundsCenter, 1.0f));

    glm::vec3 absScale = glm::abs(scale);
    float maxScale = glm::max(glm::max(absScale.x, absScale.y), absScale.z);

    float radius = model->boundsRadius * maxScale;

    return frustum.intersectsSphere(center, radius);
}

glm::vec3 Transform::getFront() const {
    return glm::normalize(rotation * glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::vec3 Transform::getRight() const {
    return glm::normalize(rotation * glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 Transform::getUp() const {
    return glm::normalize(rotation * glm::vec3(0.0f, 1.0f, 0.0f));
}
} // namespace knot
