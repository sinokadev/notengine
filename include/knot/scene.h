// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#pragma once

#include <functional>
#include <memory>
#include <string>

#include <knot/camera.h>
#include <knot/manager.h>

namespace knot {

/** @brief Collection of objects, lights, resources, camera, and environment maps. */
class Scene {
public:
    /** @brief Callback invoked by update() once per engine frame. */
    using UpdateCallback = std::function<void(Scene&, float)>;

    /** @brief Creates a scene and initializes default shaders when OpenGL is ready. */
    Scene();
    /** @brief Releases scene-owned objects, shaders, and environment textures. */
    ~Scene();

    /** @brief Removes objects, lights, environment maps, and the update callback.
     *  The resource manager is retained. */
    void clear();
    /** @brief Clears the scene and releases all managed shaders. */
    void shutdown();

    /** @brief Returns the scene object manager. */
    ObjectManager& getObjectManager();
    /** @brief Returns the scene light manager. */
    LightManager& getLightManager();
    /** @brief Returns the scene shader resource manager. */
    ResourceManager& getResourceManager();

    /** @brief Returns the active camera. @pre A camera has been assigned. */
    Camera& getCamera();
    /** @brief Returns the active camera. @pre A camera has been assigned. */
    const Camera& getCamera() const;
    /** @brief Replaces the active camera. */
    void setCamera(std::shared_ptr<Camera> cam);

    /** @brief Sets the per-frame scene update callback. */
    void setUpdateCallback(UpdateCallback callback);

    /** @brief Invokes the update callback when one is registered.
     *  @param dt Elapsed frame time in seconds. */
    void update(float dt);

    /** @brief Loads an equirectangular HDR environment and derives IBL maps. */
    void loadHDRMap(const std::string& path);

    /** @brief Returns the environment cubemap texture ID. */
    unsigned int getCubeMap() const {
        return cubeMap;
    }

    /** @brief Returns the diffuse irradiance cubemap texture ID. */
    unsigned int getIrradianceMap() const {
        return irradianceMap;
    }

    /** @brief Returns the prefilter map texture ID. */
    unsigned int getPrefilterMap() const {
        return prefilterMap;
    }

    /** @brief Loads a scene from a Seno JSON file.
     *  @return true when the file is parsed and all referenced resources load. */
    bool loadSeno(const std::string& path);

private:
    ObjectManager objectManager;
    LightManager lightManager;
    ResourceManager resourceManager;

    UpdateCallback updateCallback;

    std::shared_ptr<Camera> camera;

    unsigned int cubeMap = 0;
    unsigned int irradianceMap = 0;
    unsigned int prefilterMap = 0;
};

} // namespace knot
