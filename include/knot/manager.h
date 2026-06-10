#pragma once

#include <list>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <knot/resources.h>

namespace knot {
    class ObjectManager {
    public:

        Object& createObject(std::shared_ptr<Mesh> mesh);

        bool removeObject(unsigned int id);

        Object* getObject(unsigned int id);

        std::list<Object>& getObjectList();

    private:
        std::list<Object> objects;
        std::unordered_map<unsigned int, std::list<Object>::iterator> idToIterator;

        unsigned int nextId = 1;
    };

    class ResourceManager {
    public:
        ResourceManager();

        Shader& createShader(std::shared_ptr<ShaderSource> ss);

        bool removeShader(unsigned int id);

        Shader* getShader(unsigned int id);

        std::list<Shader>& getShaderList();

    private:
        std::list<Shader> shaders;
        std::unordered_set<unsigned int> defaultShaderIds;
        std::unordered_map<unsigned int, std::list<Shader>::iterator> idToIterator;

        AlphaShader alphashader;

        unsigned int nextId = 1;
    };
}