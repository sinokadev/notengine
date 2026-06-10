#include <list>
#include <unordered_map>
#include <string>
#include <knot/resources.h>
#include <knot/manager.h>

namespace knot {


    Object& ObjectManager::createObject(std::shared_ptr<Mesh> mesh) {
        unsigned int newId = nextId++;
        
        objects.emplace_back(mesh, newId);
        
        auto it = --objects.end(); 

        idToIterator[newId] = it;
        
        return *it;
    }

    bool ObjectManager::removeObject(unsigned int id) {
        auto it = idToIterator.find(id);
        if (it == idToIterator.end()) {
            return false;
        }

        auto listIterator = it->second;

        objects.erase(listIterator);

        idToIterator.erase(it);

        return true;
    }

    Object* ObjectManager::getObject(unsigned int id) {
        auto it = idToIterator.find(id);
        
        if (it != idToIterator.end()) {
            return &(*it->second);
        }
        
        return nullptr;
    }

    std::list<Object>& ObjectManager::getObjectList() {
        return objects;
    }



}