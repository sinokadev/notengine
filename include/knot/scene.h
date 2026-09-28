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

    /** @brief Loads a scene from a Seno v8 JSON file.
     *  Missing or non-integer/non-8 versions are rejected before clearing the scene.
     *  Object pivot is an optional local-space [x,y,z] vector, defaulting to zero.
     *  models[].obj accepts OBJ/MTL and glTF/GLB with file-provided materials.
     *  glTF is not accepted in meshes or with mesh/material/submeshes overrides.
     *  glTF animations are not imported or played back.
     *  @return true when the file is parsed and all referenced resources load. */
    // Relative external assets are resolved against the .seno file's directory.
    bool loadSeno(const std::string& path);

    /** @brief Loads a glTF (.gltf / .glb) file as a scene.
     *
     *  All mesh nodes in the default glTF scene are converted into knot::Objects
     *  and registered with the ObjectManager.  Each node's name (if present) is
     *  assigned as the object's primary group.  PBR materials and textures are
     *  loaded from the file.
     *  glTF animations are not imported or played back.
     *  @param path Absolute or relative path to the .gltf or .glb file.
     *  @return true when the file is parsed and at least one object is created. */
    bool loadGLTF(const std::string& path);

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
