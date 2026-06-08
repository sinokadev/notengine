#pragma once

#include <list>
#include <unordered_map>
#include <string>
#include <knot/resources.h>

namespace knot {
        class ObjectManager {
    public:

        Object& createObject(std::shared_ptr<Mesh> mesh);

        Object* getObject(unsigned int id);

        std::list<Object>& getObjectList();

    private:
        std::list<Object> objects;
        std::unordered_map<unsigned int, std::list<Object>::iterator> idToIterator;

        unsigned int nextId = 1;
    };
}