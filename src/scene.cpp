#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include <knot/scene.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <knot/mesh.h>
#include <knot/utility.h>

namespace knot {
Scene::Scene() {
    if (glad_glCreateShader != nullptr) {
        resourceManager.init();
    }
}

Scene::~Scene() {
    shutdown();
}

void Scene::clear() {
    objectManager.clear();
    lightManager.clear();

    if (cubeMap != 0) {
        if (glfwGetCurrentContext() != nullptr)
            glDeleteTextures(1, &cubeMap);

        cubeMap = 0;
    }

    if (irradianceMap != 0) {
        if (glfwGetCurrentContext() != nullptr)
            glDeleteTextures(1, &irradianceMap);

        irradianceMap = 0;
    }

    updateCallback = nullptr;
}

void Scene::shutdown() {
    clear();
    resourceManager.shutdown();
}

void Scene::loadHDRMap(const std::string& path) {
    if (cubeMap != 0)
        glDeleteTextures(1, &cubeMap);
    if (irradianceMap != 0)
        glDeleteTextures(1, &irradianceMap);

    unsigned int tempHdrMap = loadHDRTexture(path);

    cubeMap = bakeHDRMapToCubemap(tempHdrMap, 512);

    irradianceMap = bakeCubemapToIrradianceMap(cubeMap, 32);

    glDeleteTextures(1, &tempHdrMap);
}

void Scene::setUpdateCallback(UpdateCallback callback) {
    updateCallback = std::move(callback);
}

ObjectManager& Scene::getObjectManager() {
    return objectManager;
}

LightManager& Scene::getLightManager() {
    return lightManager;
}

ResourceManager& Scene::getResourceManager() {
    return resourceManager;
}

Camera& Scene::getCamera() {
    return *camera;
}

const Camera& Scene::getCamera() const {
    return *camera;
}

void Scene::setCamera(std::shared_ptr<Camera> cam) {
    camera = std::move(cam);
}

void Scene::update(float dt) {
    if (updateCallback) {
        updateCallback(*this, dt);
    }
}

std::string resolveAssetPath(std::string path) {
    const std::string token = "{assetRoot}";

    const auto pos = path.find(token);

    if (pos != std::string::npos) {
        path.replace(pos, token.length(), getAssetRoot());
    }

    return path;
}

bool Scene::loadSeno(const std::string& path) {
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cerr << "[Error] Failed to open Seno scene: "
                  << path << std::endl;
        return false;
    }

    try {
        nlohmann::json scene;
        file >> scene;

        clear();

        // ------------------------------------------------------------
        // Sky
        // ------------------------------------------------------------

        if (scene.contains("sky")) {
            const auto& sky = scene["sky"];

            const std::string type = sky.value("type", "");

            if (type == "HDR") {
                std::string skyPath = sky.value("path", "");

                skyPath = resolveAssetPath(skyPath);

                loadHDRMap(skyPath);
            }
        }

        // ------------------------------------------------------------
        // Meshes
        // ------------------------------------------------------------

        std::vector<std::shared_ptr<Mesh>> meshes;

        if (scene.contains("meshes")) {
            const auto& meshArray = scene["meshes"];

            if (!meshArray.is_array()) {
                std::cerr << "[Error] 'meshes' must be an array"
                          << std::endl;
                return false;
            }

            for (const auto& meshData : meshArray) {
                std::shared_ptr<Mesh> mesh;

                // External mesh
                if (meshData.is_string()) {
                    std::string meshPath = meshData.get<std::string>();
                    meshPath = resolveAssetPath(meshPath);

                    mesh = loadModelOBJ(meshPath);

                    if (!mesh) {
                        std::cerr << "[Error] Failed to load mesh: "
                                  << meshPath << std::endl;
                        return false;
                    }
                }

                // Embedded mesh
                else if (meshData.is_object()) {
                    const std::string type =
                        meshData.value("type", "embedded");

                    if (type != "embedded") {
                        std::cerr << "[Error] Unknown embedded mesh type: "
                                  << type << std::endl;
                        return false;
                    }

                    if (!meshData.contains("vertices") ||
                        !meshData.contains("indices")) {
                        std::cerr << "[Error] Embedded mesh is missing "
                                     "'vertices' or 'indices'"
                                  << std::endl;
                        return false;
                    }

                    mesh = std::make_shared<Mesh>();

                    for (const auto& vertexData :
                         meshData["vertices"]) {
                        Vertex vertex{};

                        const auto& position =
                            vertexData["position"];

                        vertex.Position = glm::vec3(
                            position[0].get<float>(),
                            position[1].get<float>(),
                            position[2].get<float>()
                        );

                        if (vertexData.contains("normal")) {
                            const auto& normal =
                                vertexData["normal"];

                            vertex.Normal = glm::vec3(
                                normal[0].get<float>(),
                                normal[1].get<float>(),
                                normal[2].get<float>()
                            );
                        } else {
                            vertex.Normal =
                                glm::vec3(0.0f, 1.0f, 0.0f);
                        }

                        if (vertexData.contains("texcoord")) {
                            const auto& uv =
                                vertexData["texcoord"];

                            vertex.TexCoords = glm::vec2(
                                uv[0].get<float>(),
                                uv[1].get<float>()
                            );
                        } else {
                            vertex.TexCoords =
                                glm::vec2(0.0f);
                        }

                        if (vertexData.contains("tangent")) {
                            const auto& tangent =
                                vertexData["tangent"];

                            vertex.Tangent = glm::vec3(
                                tangent[0].get<float>(),
                                tangent[1].get<float>(),
                                tangent[2].get<float>()
                            );
                        } else {
                            vertex.Tangent =
                                glm::vec3(0.0f);
                        }

                        mesh->vertices.push_back(vertex);
                    }

                    for (const auto& index :
                         meshData["indices"]) {
                        mesh->indices.push_back(
                            index.get<unsigned int>()
                        );
                    }

                    if (mesh->vertices.empty() ||
                        mesh->indices.empty()) {
                        std::cerr << "[Error] Embedded mesh is empty"
                                  << std::endl;
                        return false;
                    }

                    // Tangent가 없는 경우 계산
                    bool hasTangent = false;

                    for (const auto& vertex : mesh->vertices) {
                        if (glm::length(vertex.Tangent) > 0.0001f) {
                            hasTangent = true;
                            break;
                        }
                    }

                    if (!hasTangent) {
                        calculateMeshTangents(
                            mesh->vertices,
                            mesh->indices
                        );
                    }

                    mesh->indexCount =
                        static_cast<unsigned int>(
                            mesh->indices.size()
                        );

                    mesh->setup();
                }

                else {
                    std::cerr << "[Error] Invalid mesh entry"
                              << std::endl;
                    return false;
                }

                meshes.push_back(mesh);
            }
        }

        // ------------------------------------------------------------
        // Materials
        // ------------------------------------------------------------

        std::vector<std::shared_ptr<Material>> materials;

        if (scene.contains("materials")) {
            for (const auto& materialData :
                 scene["materials"]) {

                const std::string shaderName =
                    materialData.value("shader", "");

                    if (shaderName == "pbrShader") {
                        auto shader = resourceManager.getShader("pbrShader");

                        if (!shader) {
                            std::cerr << "[Error] Failed to find shader: "
                                    << shaderName << std::endl;
                            return false;
                        }

                        glm::vec3 albedo(1.0f);

                        if (materialData.contains("albedo")) {
                            const auto& c = materialData["albedo"];

                            if (!c.is_array() || c.size() != 3) {
                                std::cerr << "[Error] Invalid albedo value"
                                        << std::endl;
                                return false;
                            }

                            albedo = glm::vec3(
                                c[0].get<float>(),
                                c[1].get<float>(),
                                c[2].get<float>()
                            );
                        }

                        const float metallic =
                            materialData.value("metallic", 0.0f);

                        const float roughness =
                            materialData.value("roughness", 0.5f);

                        const float ao =
                            materialData.value("ao", 1.0f);

                        materials.push_back(
                            std::make_shared<PbrMaterial>(
                                shader,
                                albedo,
                                metallic,
                                roughness,
                                ao
                            )
                        );
                    }
                else {
                    std::cerr << "[Error] Unknown shader: "
                              << shaderName << std::endl;
                    return false;
                }
            }
        }

        // ------------------------------------------------------------
        // Models
        // ------------------------------------------------------------

        std::vector<std::shared_ptr<Model>> models;

        if (scene.contains("models")) {
            for (const auto& modelData : scene["models"]) {

                // --------------------------------------------------------
                // External OBJ (+ optional MTL)
                // --------------------------------------------------------

                if (modelData.contains("obj")) {
                    std::string objPath =
                        modelData.value("obj", "");

                    if (objPath.empty()) {
                        std::cerr << "[Error] Model OBJ path is empty"
                                << std::endl;
                        return false;
                    }

                    objPath = resolveAssetPath(objPath);

                    auto shader =
                        resourceManager.getShader("pbrShader");

                    if (!shader) {
                        std::cerr << "[Error] Failed to find shader: "
                                << "pbrShader"
                                << std::endl;
                        return false;
                    }

                    auto loadedModels =
                        loadModelOBJWithMTL(
                            objPath,
                            shader
                        );

                    if (loadedModels.empty()) {
                        std::cerr << "[Error] Failed to load OBJ: "
                                << objPath
                                << std::endl;
                        return false;
                    }

                    for (auto& model : loadedModels) {
                        models.push_back(std::move(model));
                    }

                    continue;
                }

                // --------------------------------------------------------
                // Mesh + Material
                // --------------------------------------------------------

                const int meshIndex =
                    modelData.value("mesh", -1);

                const int materialIndex =
                    modelData.value("material", -1);

                if (meshIndex < 0 ||
                    meshIndex >= static_cast<int>(meshes.size())) {
                    std::cerr << "[Error] Model mesh index "
                            << meshIndex
                            << " is out of range"
                            << std::endl;
                    return false;
                }

                if (materialIndex < 0 ||
                    materialIndex >=
                        static_cast<int>(materials.size())) {
                    std::cerr << "[Error] Model material index "
                            << materialIndex
                            << " is out of range"
                            << std::endl;
                    return false;
                }

                models.push_back(
                    std::make_shared<Model>(
                        meshes[meshIndex],
                        materials[materialIndex]
                    )
                );
            }
        }
        // ------------------------------------------------------------
        // Objects
        // ------------------------------------------------------------

        if (scene.contains("objects")) {
            for (const auto& objectData :
                 scene["objects"]) {

                const int modelIndex =
                    objectData.value("model", -1);

                if (modelIndex < 0 ||
                    modelIndex >=
                        static_cast<int>(models.size())) {
                    std::cerr << "[Error] Object model index "
                              << modelIndex
                              << " is out of range"
                              << std::endl;
                    return false;
                }

                auto object =
                    std::make_shared<Object>();

                object->model = models[modelIndex];

                if (objectData.contains("position")) {
                    const auto& p =
                        objectData["position"];

                    object->position = glm::vec3(
                        p[0].get<float>(),
                        p[1].get<float>(),
                        p[2].get<float>()
                    );
                }

                if (objectData.contains("scale")) {
                    const auto& s =
                        objectData["scale"];

                    object->scale = glm::vec3(
                        s[0].get<float>(),
                        s[1].get<float>(),
                        s[2].get<float>()
                    );
                }

                if (objectData.contains("rotation")) {
                    const auto& r =
                        objectData["rotation"];

                    object->rotation = glm::quat(
                        r[0].get<float>(),
                        r[1].get<float>(),
                        r[2].get<float>(),
                        r[3].get<float>()
                    );
                }

                objectManager.registerObject(object);
            }
        }

        // ------------------------------------------------------------
        // Lights
        // ------------------------------------------------------------

        if (scene.contains("lights")) {
            for (const auto& lightData :
                 scene["lights"]) {

                const std::string type =
                    lightData.value("type", "");

                if (type == "PbrPointLight") {
                    glm::vec3 position(0.0f);
                    glm::vec3 color(1.0f);
                    float intensity = 1.0f;

                    if (lightData.contains("position")) {
                        const auto& p =
                            lightData["position"];

                        position = glm::vec3(
                            p[0].get<float>(),
                            p[1].get<float>(),
                            p[2].get<float>()
                        );
                    }

                    if (lightData.contains("color")) {
                        const auto& c =
                            lightData["color"];

                        color = glm::vec3(
                            c[0].get<float>(),
                            c[1].get<float>(),
                            c[2].get<float>()
                        );
                    }

                    intensity =
                        lightData.value(
                            "intensity", 1.0f);

                    auto light =
                        std::make_shared<PbrPointLight>(
                            position,
                            color,
                            intensity
                        );

                    lightManager.registerLight(light);
                }

                else if (type == "DirLight") {
                    auto light =
                        std::make_shared<DirLight>();

                    if (lightData.contains("color")) {
                        const auto& c =
                            lightData["color"];

                        light->color = glm::vec3(
                            c[0].get<float>(),
                            c[1].get<float>(),
                            c[2].get<float>()
                        );
                    }

                    light->intensity =
                        lightData.value(
                            "intensity", 1.0f);

                    if (lightData.contains("ambient")) {
                        const auto& c =
                            lightData["ambient"];

                        light->ambient = glm::vec3(
                            c[0].get<float>(),
                            c[1].get<float>(),
                            c[2].get<float>()
                        );
                    }

                    if (lightData.contains("diffuse")) {
                        const auto& c =
                            lightData["diffuse"];

                        light->diffuse = glm::vec3(
                            c[0].get<float>(),
                            c[1].get<float>(),
                            c[2].get<float>()
                        );
                    }

                    if (lightData.contains("specular")) {
                        const auto& c =
                            lightData["specular"];

                        light->specular = glm::vec3(
                            c[0].get<float>(),
                            c[1].get<float>(),
                            c[2].get<float>()
                        );
                    }

                    if (lightData.contains("rotation")) {
                        const auto& r =
                            lightData["rotation"];

                        light->rotation = glm::quat(
                            r[0].get<float>(),
                            r[1].get<float>(),
                            r[2].get<float>(),
                            r[3].get<float>()
                        );
                    }

                    lightManager.registerLight(light);
                }
            }
        }

        // ------------------------------------------------------------
        // Camera
        // ------------------------------------------------------------

        if (scene.contains("camera")) {
            const auto& cameraData =
                scene["camera"];

            const std::string type =
                cameraData.value("type", "Camera");

            glm::vec3 position(0.0f, 0.0f, 5.0f);

            if (cameraData.contains("position")) {
                const auto& p =
                    cameraData["position"];

                position = glm::vec3(
                    p[0].get<float>(),
                    p[1].get<float>(),
                    p[2].get<float>()
                );
            }

            if (type == "MovingCamera") {
                auto cam =
                    std::make_shared<MovingCamera>(
                        position
                    );

                cam->yaw =
                    cameraData.value("yaw", -90.0f);

                cam->pitch =
                    cameraData.value("pitch", 0.0f);

                cam->fov =
                    cameraData.value("fov", 45.0f);

                cam->nearPlane =
                    cameraData.value("near", 0.1f);

                cam->farPlane =
                    cameraData.value("far", 100.0f);

                setCamera(cam);
            }
        }

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[Error] Failed to parse Seno scene: "
                  << e.what() << std::endl;
        return false;
    }
}
} // namespace knot