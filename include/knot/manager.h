// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#pragma once

#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <knot/resources.h>
#include <knot/camera.h>

namespace knot {
/** @brief Owns and indexes scene objects by their numeric IDs. */
class ObjectManager {
public:
    ObjectManager() = default;
    ~ObjectManager();

    /** @brief Adds an object and assigns an ID when it has none.
     *  @param newObject Object to manage; may be null.
     *  @return The object's ID, or 0 when @p newObject is null. */
    unsigned int registerObject(std::shared_ptr<Object> newObject);

    /** @brief Removes the object with @p id.
     *  @return true when an object was removed. */
    bool removeObject(unsigned int id);

    /** @brief Finds a managed object.
     *  @return The object, or nullptr if no object has @p id. */
    Object* getObject(unsigned int id);

    /** @brief Removes every managed object and resets ID allocation. */
    void clear();
    /** @brief Releases all managed objects. Equivalent to clear(). */
    void shutdown();

    /** @brief Returns the managed objects in registration order. */
    const std::list<std::shared_ptr<Object>>& getObjects() const {
        return objects;
    }

private:
    std::list<std::shared_ptr<Object>> objects;
    std::unordered_map<unsigned int, std::list<std::shared_ptr<Object>>::iterator> idToIterator;

    unsigned int nextId = 1;
};

/** @brief Owns and indexes scene lights by their numeric IDs. */
class LightManager {
public:
    LightManager() = default;
    ~LightManager();

    /** @brief Adds a light and assigns an ID when it has none.
     *  @return The light's ID, or 0 when @p newLight is null. */
    unsigned int registerLight(std::shared_ptr<Light> newLight);

    /** @brief Removes the light with @p id.
     *  @return true when a light was removed. */
    bool removeLight(unsigned int id);

    /** @brief Finds a managed light, or nullptr when absent. */
    Light* getLight(unsigned int id);
    /** @brief Returns all managed directional lights. */
    std::vector<const DirLight*> getDirLights() const;
    /** @brief Returns all managed PBR point lights. */
    std::vector<const PbrPointLight*> getPointLights() const;

    /** @brief Removes every managed light and resets ID allocation. */
    void clear();
    /** @brief Releases all managed lights. Equivalent to clear(). */
    void shutdown();

    /** @brief Returns the managed lights in registration order. */
    const std::list<std::shared_ptr<Light>>& getLights() const {
        return lights;
    }

private:
    std::list<std::shared_ptr<Light>> lights;
    std::unordered_map<unsigned int, std::list<std::shared_ptr<Light>>::iterator> idToIterator;

    unsigned int nextId = 1;
};

/** @brief Owns shader programs created for a scene. */
class ResourceManager {
public:
    ResourceManager() = default;
    ~ResourceManager();

    /** @brief Creates the built-in alpha and PBR shaders.
     *  @return true if both default shaders were created. */
    bool init();
    /** @brief Releases every managed shader and resets ID allocation. */
    void clear();
    /** @brief Releases all shader resources. Equivalent to clear(). */
    void shutdown();

    /** @brief Compiles and registers a named shader program.
     *  @param ss Source files for the vertex and fragment stages.
     *  @param name Lookup name for the new shader.
     *  @return The compiled shader, or nullptr when the source or compilation is invalid. */
    std::shared_ptr<Shader> createShader(std::shared_ptr<ShaderSource> ss, const std::string& name);

    /** @brief Removes a non-default shader.
     *  @return true when the shader existed and was removable. */
    bool removeShader(unsigned int id);

    /** @brief Finds a shader by its numeric ID. */
    std::shared_ptr<Shader> getShader(unsigned int id);
    /** @brief Finds a shader by its registered name. */
    std::shared_ptr<Shader> getShader(const std::string& name);

    /** @brief Returns the managed shaders in creation order. */
    const std::list<std::shared_ptr<Shader>>& getShaders() const {
        return shaders;
    }

private:
    std::list<std::shared_ptr<Shader>> shaders;
    std::unordered_set<unsigned int> defaultShaderIds;
    std::unordered_map<unsigned int, std::list<std::shared_ptr<Shader>>::iterator> idToIterator;
    std::unordered_map<std::string, unsigned int> nameToId;

    unsigned int nextId = 1;
};
} // namespace knot
