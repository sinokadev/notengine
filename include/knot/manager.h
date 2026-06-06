#pragma once

#include <vector>
#include <knot/resources.h>

namespace knot {
        class ObjectManager {
    public:

        Object& createObject(Mesh* mesh);

        Object* getObjectById(unsigned int id);

        std::vector<Object>& getObjectList();

    private:
        std::vector<Object> objects;
        std::unordered_map<unsigned int, size_t> idToIndex;

        unsigned int nextId = 1;
    };
}