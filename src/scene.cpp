#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include <knot/scene.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

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
    shutdown();
}

void Scene::shutdown() {
    const bool hasContext = (glfwGetCurrentContext() != nullptr);
    if (cubeMap != 0) {
        if (hasContext) {
            glDeleteTextures(1, &cubeMap);
        }
        cubeMap = 0;
    }
    if (irradianceMap != 0) {
        if (hasContext) {
            glDeleteTextures(1, &irradianceMap);
        }
        irradianceMap = 0;
    }

    objectManager.shutdown();
    resourceManager.shutdown();
    updateCallback = nullptr;
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

    nlohmann::json root;

    try {
        file >> root;
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "[Error] Failed to parse Seno scene: "
                  << e.what() << std::endl;
        return false;
    }

    try {
        const int version = root.at("version").get<int>();

        if (version != 1) {
            std::cerr << "[Error] Failed to load Seno scene: "
                      << "unsupported version " << version << std::endl;
            return false;
        }

        auto readVec3 = [](const nlohmann::json& value) {
            return glm::vec3(
                value.at(0).get<float>(),
                value.at(1).get<float>(),
                value.at(2).get<float>()
            );
        };

        auto readQuat = [](const nlohmann::json& value) {
            // Seno rotation format: [w, x, y, z]
            return glm::quat(
                value.at(0).get<float>(),
                value.at(1).get<float>(),
                value.at(2).get<float>(),
                value.at(3).get<float>()
            );
        };

        /*
         * Meshes
         */

        std::vector<std::shared_ptr<Mesh>> meshes;

        if (root.contains("meshes")) {
            for (const auto& meshValue : root["meshes"]) {
                const std::string meshPath =
                    resolveAssetPath(meshValue.get<std::string>());

                auto mesh = loadModelOBJ(meshPath);

                if (!mesh || !mesh->isReady()) {
                    std::cerr << "[Error] Failed to load mesh: "
                              << meshPath << std::endl;
                    return false;
                }

                meshes.push_back(mesh);
            }
        }

        /*
         * Materials
         */

        std::vector<std::shared_ptr<Material>> materials;

        if (root.contains("materials")) {
            for (const auto& materialData : root["materials"]) {
                const std::string shaderName =
                    materialData.at("shader").get<std::string>();

                auto shader =
                    resourceManager.getShader(shaderName);

                if (!shader || !shader->isValid()) {
                    std::cerr << "[Error] Failed to find shader: "
                              << shaderName << std::endl;
                    return false;
                }

                if (shaderName == "pbrShader") {
                    const glm::vec3 albedo =
                        materialData.contains("albedo")
                            ? readVec3(materialData["albedo"])
                            : glm::vec3(1.0f);

                    const float metallic =
                        materialData.value("metallic", 0.0f);

                    const float roughness =
                        materialData.value("roughness", 0.5f);

                    const float ao =
                        materialData.value("ao", 1.0f);

                    auto material =
                        std::make_shared<PbrMaterial>(
                            shader,
                            albedo,
                            metallic,
                            roughness,
                            ao
                        );

                    materials.push_back(material);
                } else {
                    std::cerr << "[Error] Failed to create material: "
                              << "unsupported shader '"
                              << shaderName << "'"
                              << std::endl;
                    return false;
                }
            }
        }

        /*
         * Models
         */

        std::vector<std::shared_ptr<Model>> models;

        if (root.contains("models")) {
            for (const auto& modelData : root["models"]) {
                const size_t meshIndex =
                    modelData.at("mesh").get<size_t>();

                const size_t materialIndex =
                    modelData.at("material").get<size_t>();

                if (meshIndex >= meshes.size()) {
                    std::cerr << "[Error] Failed to create model: "
                              << "mesh index " << meshIndex
                              << " is out of range"
                              << std::endl;
                    return false;
                }

                if (materialIndex >= materials.size()) {
                    std::cerr << "[Error] Failed to create model: "
                              << "material index " << materialIndex
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

        /*
         * Objects
         */

        if (root.contains("objects")) {
            for (const auto& objectData : root["objects"]) {
                const size_t modelIndex =
                    objectData.at("model").get<size_t>();

                if (modelIndex >= models.size()) {
                    std::cerr << "[Error] Failed to create object: "
                              << "model index " << modelIndex
                              << " is out of range"
                              << std::endl;
                    return false;
                }

                auto object =
                    std::make_shared<Object>(models[modelIndex]);

                if (objectData.contains("position")) {
                    object->position =
                        readVec3(objectData["position"]);
                }

                if (objectData.contains("rotation")) {
                    object->rotation =
                        readQuat(objectData["rotation"]);
                }

                if (objectData.contains("scale")) {
                    object->scale =
                        readVec3(objectData["scale"]);
                }

                objectManager.registerObject(object);
            }
        }

        /*
         * Lights
         */

        if (root.contains("lights")) {
            for (const auto& lightData : root["lights"]) {
                const std::string type =
                    lightData.at("type").get<std::string>();

                std::shared_ptr<Light> light;

                if (type == "DirLight") {
                    auto dirLight =
                        std::make_shared<DirLight>();

                    if (lightData.contains("rotation")) {
                        dirLight->rotation =
                            readQuat(lightData["rotation"]);
                    }

                    if (lightData.contains("color")) {
                        dirLight->color =
                            readVec3(lightData["color"]);
                    }

                    if (lightData.contains("intensity")) {
                        dirLight->intensity =
                            lightData["intensity"].get<float>();
                    }

                    if (lightData.contains("ambient")) {
                        dirLight->ambient =
                            readVec3(lightData["ambient"]);
                    }

                    if (lightData.contains("diffuse")) {
                        dirLight->diffuse =
                            readVec3(lightData["diffuse"]);
                    }

                    if (lightData.contains("specular")) {
                        dirLight->specular =
                            readVec3(lightData["specular"]);
                    }

                    light = dirLight;
                } else if (type == "PbrPointLight") {
                    auto pointLight =
                        std::make_shared<PbrPointLight>();

                    if (lightData.contains("position")) {
                        pointLight->position =
                            readVec3(lightData["position"]);
                    }

                    if (lightData.contains("color")) {
                        pointLight->color =
                            readVec3(lightData["color"]);
                    }

                    if (lightData.contains("intensity")) {
                        pointLight->intensity =
                            lightData["intensity"].get<float>();
                    }

                    light = pointLight;
                } else {
                    std::cerr << "[Error] Failed to create light: "
                              << "unknown type '" << type << "'"
                              << std::endl;
                    return false;
                }

                lightManager.registerLight(light);
            }
        }

        /*
         * Camera
         */

        if (root.contains("camera")) {
            const auto& cameraData = root.at("camera");

            const std::string type =
                cameraData.at("type").get<std::string>();

            if (type == "MovingCamera") {
                const glm::vec3 position =
                    readVec3(cameraData.value(
                        "position",
                        nlohmann::json::array({0.0f, 0.0f, 0.0f})
                    ));

                const float yaw =
                    cameraData.value("yaw", -90.0f);

                const float pitch =
                    cameraData.value("pitch", 0.0f);

                const float fov =
                    cameraData.value("fov", 45.0f);

                const float nearPlane =
                    cameraData.value("near", 0.1f);

                const float farPlane =
                    cameraData.value("far", 100.0f);

                auto camera = std::make_shared<MovingCamera>(
                    position,
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    yaw,
                    pitch,
                    fov,
                    nearPlane,
                    farPlane
                );

                setCamera(camera);
            } else {
                std::cerr << "[Error] Failed to create camera: "
                        << "unknown type '" << type << "'"
                        << std::endl;
                return false;
            }
        }

        if (root.contains("sky")) {
            const auto& sky = root["sky"];

            if (sky.value("type", "") == "HDR") {
                std::string path = sky.value("path", "");

                path = resolveAssetPath(path);

                if (!path.empty()) {
                    loadHDRMap(path);
                }
            }
        }

    } catch (const nlohmann::json::exception& e) {
        std::cerr << "[Error] Failed to load Seno scene: "
                  << e.what() << std::endl;
        return false;
    }

    return true;
}
} // namespace knot