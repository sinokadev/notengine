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


    ResourceManager::ResourceManager() {
        auto alphaSource = std::make_shared<ShaderSource>(AlphaShader::GetSource());
        
        Shader& defaultShader = createShader(alphaSource);
        
        defaultShaderIds.insert(defaultShader.get_id());
    }

    Shader& ResourceManager::createShader(std::shared_ptr<ShaderSource> ss) {
        unsigned int newId = nextId++;
        
        shaders.emplace_back(ss, newId);
        
        auto it = --shaders.end(); 

        idToIterator[newId] = it;
        
        return *it;
    }

    bool ResourceManager::removeShader(unsigned int id) {
        if (defaultShaderIds.find(id) != defaultShaderIds.end()) {
            return false; 
        }

        auto it = idToIterator.find(id);
        if (it == idToIterator.end()) {
            return false;
        }

        auto listIterator = it->second;

        shaders.erase(listIterator);

        idToIterator.erase(it);

        return true;
    }

    Shader* ResourceManager::getShader(unsigned int id) {
        auto it = idToIterator.find(id);
        
        if (it != idToIterator.end()) {
            return &(*it->second);
        }
        
        return nullptr;
    }

    std::list<Shader>& ResourceManager::getShaderList() {
        return shaders;
    }

}