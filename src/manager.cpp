#include <list>
#include <unordered_map>
#include <string>
#include <knot/resources.h>
#include <knot/manager.h>

namespace knot {


    Object& ObjectManager::createObject(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material) {
        unsigned int newId = nextId++;
        
        objects.emplace_back(mesh, material, newId);
        
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


    ResourceManager::ResourceManager() {}

    void ResourceManager::init() {
        auto alphaSource = std::make_shared<ShaderSource>(AlphaShader::GetSource());
        auto defaultShader = createShader(alphaSource, "my_alpha_shader");
        defaultShaderIds.insert(defaultShader->get_id());
    }

    std::shared_ptr<Shader> ResourceManager::createShader(std::shared_ptr<ShaderSource> ss, const std::string& name) {
        unsigned int newId = nextId++;
        
        shaders.emplace_back(std::make_shared<Shader>(ss, newId));
        
        auto it = --shaders.end(); 

        idToIterator[newId] = it;
        nameToId[name] = newId;
        
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

        for (auto it = nameToId.begin(); it != nameToId.end(); ++it) {
            if (it->second == id) {
                nameToId.erase(it);
                break;
            }
        }

        return true;
    }

    std::shared_ptr<Shader> ResourceManager::getShader(unsigned int id) {
        auto it = idToIterator.find(id);
        
        if (it != idToIterator.end()) {
            return *it->second;
        }
        
        return nullptr;
    }

    std::shared_ptr<Shader> ResourceManager::getShader(const std::string& name) {
        auto it = nameToId.find(name);
        if (it != nameToId.end()) {
            return getShader(it->second);
        }
        return nullptr;
    }

    std::list<std::shared_ptr<Shader>>& ResourceManager::getShaderList() {
        return shaders;
    }

}