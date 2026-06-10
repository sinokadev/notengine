#pragma once

#include <list>
#include <unordered_map>
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

        Shader& createShader(std::shared_ptr<Mesh> mesh);

        bool removeShader(unsigned int id);

        Shader* getShader(unsigned int id);

        std::list<Shader>& getShaderList();

    private:
        std::list<Shader> objects;
        std::unordered_map<unsigned int, std::list<Shader>::iterator> idToIterator;

        unsigned int nextId = 1;
    };
}