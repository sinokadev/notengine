#pragma once

#include <list>
#include <unordered_map>
#include <unordered_set>
#include <string>

#include <knot/resources.h>

namespace knot {
    class ObjectManager {
    public:
        Object& createObject(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);

        bool removeObject(unsigned int id);

        Object* getObject(unsigned int id);

        const std::list<Object>& getObjects() const { return objects; }

    private:
        std::list<Object> objects;
        std::unordered_map<unsigned int, std::list<Object>::iterator> idToIterator;

        unsigned int nextId = 1;
    };

    class ResourceManager {
    public:
        ResourceManager() = default;

        bool init();

        std::shared_ptr<Shader> createShader(std::shared_ptr<ShaderSource> ss, const std::string& name);

        bool removeShader(unsigned int id);

        std::shared_ptr<Shader> getShader(unsigned int id);
        std::shared_ptr<Shader> getShader(const std::string& name);

        const std::list<std::shared_ptr<Shader>>& getShaders() const { return shaders; }

    private:
        std::list<std::shared_ptr<Shader>> shaders;
        std::unordered_set<unsigned int> defaultShaderIds;
        std::unordered_map<unsigned int, std::list<std::shared_ptr<Shader>>::iterator> idToIterator;
        std::unordered_map<std::string, unsigned int> nameToId;

        unsigned int nextId = 1;
    };
}
