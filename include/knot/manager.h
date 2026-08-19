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
class ObjectManager {
public:
    ObjectManager() = default;
    ~ObjectManager();

    unsigned int registerObject(std::shared_ptr<Object> newObject);

    bool removeObject(unsigned int id);

    Object* getObject(unsigned int id);

    void clear();
    void shutdown();

    const std::list<std::shared_ptr<Object>>& getObjects() const {
        return objects;
    }

private:
    std::list<std::shared_ptr<Object>> objects;
    std::unordered_map<unsigned int, std::list<std::shared_ptr<Object>>::iterator> idToIterator;

    unsigned int nextId = 1;
};

class LightManager {
public:
    LightManager() = default;
    ~LightManager();

    unsigned int registerLight(std::shared_ptr<Light> newLight);

    bool removeLight(unsigned int id);

    Light* getLight(unsigned int id);
    std::vector<const DirLight*> getDirLights() const;
    std::vector<const PbrPointLight*> getPointLights() const;

    void clear();
    void shutdown();

    const std::list<std::shared_ptr<Light>>& getLights() const {
        return lights;
    }

private:
    std::list<std::shared_ptr<Light>> lights;
    std::unordered_map<unsigned int, std::list<std::shared_ptr<Light>>::iterator> idToIterator;

    unsigned int nextId = 1;
};

class ResourceManager {
public:
    ResourceManager() = default;
    ~ResourceManager();

    bool init();
    void clear();
    void shutdown();

    std::shared_ptr<Shader> createShader(std::shared_ptr<ShaderSource> ss, const std::string& name);

    bool removeShader(unsigned int id);

    std::shared_ptr<Shader> getShader(unsigned int id);
    std::shared_ptr<Shader> getShader(const std::string& name);

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
