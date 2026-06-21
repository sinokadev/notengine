#include <knot/manager.h>

#include <iostream>

namespace knot {

unsigned int ObjectManager::registerObject(std::shared_ptr<Object> newObject) {
    if (!newObject) return 0;

    if (newObject->id == 0) {
        newObject->id = nextId++; 
    }

    objects.push_back(newObject);
    auto it = --objects.end();
    idToIterator[newObject->id] = it;

    return newObject->id;
}
bool ObjectManager::removeObject(unsigned int id) {
    auto it = idToIterator.find(id);
    if (it == idToIterator.end()) {
        return false;
    }

    objects.erase(it->second);
    idToIterator.erase(it);
    return true;
}

Object* ObjectManager::getObject(unsigned int id) {
    auto it = idToIterator.find(id);
    if (it != idToIterator.end()) {
        return it->second->get();
    }

    return nullptr;
}

bool ResourceManager::init() {
    auto alphaSource = std::make_shared<ShaderSource>(AlphaShader::GetSource());
    if (!alphaSource->isValid()) {
        std::cerr << "[Error] Failed to load default alpha shader sources" << std::endl;
        return false;
    }

    auto alphaShader = createShader(alphaSource, "alphaShader");
    if (!alphaShader) {
        std::cerr << "[Error] Failed to create default alpha shader" << std::endl;
        return false;
    }

    defaultShaderIds.insert(alphaShader->getId());

    auto pongSource = std::make_shared<ShaderSource>(PongShader::GetSource());
    if (!pongSource->isValid()) {
        std::cerr << "[Error] Failed to load default pong shader sources" << std::endl;
        return false;
    }

    auto pongShader = createShader(pongSource, "pongShader");
    if (!pongShader) {
        std::cerr << "[Error] Failed to create default pong shader" << std::endl;
        return false;
    }

    defaultShaderIds.insert(pongShader->getId());
    return true;
}

std::shared_ptr<Shader> ResourceManager::createShader(std::shared_ptr<ShaderSource> ss, const std::string& name) {
    if (!ss || !ss->isValid()) {
        std::cerr << "[Error] Invalid shader source for '" << name << "'" << std::endl;
        return nullptr;
    }

    const unsigned int newId = nextId++;
    auto shader = std::make_shared<Shader>(ss, newId);
    if (!shader->isValid()) {
        std::cerr << "[Error] Shader creation failed for '" << name << "'" << std::endl;
        return nullptr;
    }

    shaders.push_back(shader);

    auto it = --shaders.end();
    idToIterator[newId] = it;
    nameToId[name] = newId;

    return shader;
}

bool ResourceManager::removeShader(unsigned int id) {
    if (defaultShaderIds.find(id) != defaultShaderIds.end()) {
        return false;
    }

    auto it = idToIterator.find(id);
    if (it == idToIterator.end()) {
        return false;
    }

    shaders.erase(it->second);
    idToIterator.erase(it);

    for (auto nameIt = nameToId.begin(); nameIt != nameToId.end();) {
        if (nameIt->second == id) {
            nameIt = nameToId.erase(nameIt);
        } else {
            ++nameIt;
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

} // namespace knot
