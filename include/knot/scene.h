#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <knot/camera.h>
#include <knot/manager.h>
#include <knot/particle.h>

namespace knot {

/** @brief Local reflection probe configuration and its baked cubemaps. */
struct ReflectionProbe {
    glm::vec3 position{0.0f};
    glm::vec3 boundsMin{-1.0f};
    glm::vec3 boundsMax{1.0f};
    int resolution = 128;
    unsigned int cubeMap = 0;
    unsigned int irradianceMap = 0;
    unsigned int prefilterMap = 0;
};

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
    ShaderManager& getShaderManager();

    /** @brief Returns the active camera. @pre A camera has been assigned. */
    Camera& getCamera();
    /** @brief Returns the active camera. @pre A camera has been assigned. */
    const Camera& getCamera() const;
    /** @brief Replaces the active camera. */
    void setCamera(std::shared_ptr<Camera> cam);

    /** @brief Sets the per-frame scene update callback. */
    void setUpdateCallback(UpdateCallback callback);

    /**
     * @brief Registers a particle system that the scene advances and draws.
     *
     * The scene keeps a shared reference to the system, so the caller may drop
     * its own. Registered systems are updated by update() before the update
     * callback runs and are drawn by Renderer::renderScene() after the opaque
     * and translucent object passes.
     *
     * @param system Particle system to take ownership of; null is ignored.
     */
    void addParticleSystem(std::shared_ptr<ParticleSystem> system);

    /**
     * @brief Returns the registered particle systems.
     *
     * The pointers stay valid until the next addParticleSystem(), clear() or
     * destruction of the scene.
     */
    const std::vector<const ParticleSystem*>& getParticleSystems() const {
        return particleSystemView;
    }

    /** @brief Removes every registered particle system. */
    void clearParticleSystems();

    /** @brief Invokes the update callback when one is registered.
     *  Registered particle systems are advanced first.
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

    /** @brief Configures the scene's local reflection probe.
     *  A successful change invalidates any previously captured maps.
     *  The probe position must lie inside its bounds; resolution must be a
     *  power of two from 16 through 2048.
     *  @return false if the position, bounds, or resolution are invalid. */
    bool setReflectionProbe(const glm::vec3& position, const glm::vec3& boundsMin, const glm::vec3& boundsMax, int resolution = 128);

    /** @brief Returns the configured reflection probe, or nullptr if none is configured. */
    const ReflectionProbe* getReflectionProbe() const {
        return reflectionProbe ? &*reflectionProbe : nullptr;
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
     *  assigned as the object's first group.  PBR materials and textures are
     *  loaded from the file.
     *  glTF animations are not imported or played back.
     *  @param path Absolute or relative path to the .gltf or .glb file.
     *  @return true when the file is parsed and at least one object is created. */
    bool loadGLTF(const std::string& path);

private:
    friend class Renderer;

    void releaseReflectionProbeMaps();

    ObjectManager objectManager;
    LightManager lightManager;
    ShaderManager resourceManager;

    UpdateCallback updateCallback;

    /** @brief Particle systems owned by the scene. */
    std::vector<std::shared_ptr<ParticleSystem>> particleSystems;

    /** @brief Non-owning view of particleSystems handed to the renderer. */
    std::vector<const ParticleSystem*> particleSystemView;

    /** @brief Rebuilds particleSystemView after the owning list changes. */
    void rebuildParticleSystemView();

    std::shared_ptr<Camera> camera;

    unsigned int cubeMap = 0;
    unsigned int irradianceMap = 0;
    unsigned int prefilterMap = 0;
    std::optional<ReflectionProbe> reflectionProbe;
};

} // namespace knot
