#include <vector>
#include <knot/resources.h>
#include <knot/manager.h>

namespace knot {


    Object& ObjectManager::createObject(Mesh* mesh) {
        unsigned int newId = nextId++;
        
        objects.emplace_back(mesh, newId);

        idToIndex[newId] = objects.size() - 1;

        return objects.back(); 
    }

    Object* ObjectManager::getObjectById(unsigned int id) {
        auto it = idToIndex.find(id);
        if (it != idToIndex.end()) {

            return &objects[it->second];
        }
        return nullptr;
    }

}