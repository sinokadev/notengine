#include <knot/manager.h>

#include <iostream>

namespace knot {

unsigned int ObjectManager::allocateId() {
    while (nextId == 0 || idToIterator.find(nextId) != idToIterator.end()) {
        nextId++;
    }
    return nextId++;
}

unsigned int ObjectManager::registerObject(std::shared_ptr<Object> newObject, int id) {
    if (!newObject)
        return 0;

    unsigned int finalId = 0;
    if (id == -1) {
        finalId = allocateId();
    } else {
        const unsigned int requestedId = static_cast<unsigned int>(id);
        if (idToIterator.find(requestedId) != idToIterator.end()) {
            finalId = allocateId();
            std::cerr << "[Warning] Object ID " << requestedId << " already exists. Auto-assigning ID: " << finalId << std::endl;
        } else {
            finalId = requestedId;
        }
    }

    newObject->id = finalId;
    objects.push_back(newObject);
    auto it = --objects.end();
    idToIterator[finalId] = it;

    return finalId;
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

ObjectManager::~ObjectManager() {
    shutdown();
}

void ObjectManager::clear() {
    objects.clear();
    idToIterator.clear();
    nextId = 1;
}

void ObjectManager::shutdown() {
    clear();
}

Object* ObjectManager::getObject(unsigned int id) {
    auto it = idToIterator.find(id);
    if (it != idToIterator.end()) {
        return it->second->get();
    }

    return nullptr;
}

const Object* ObjectManager::getObject(unsigned int id) const {
    auto it = idToIterator.find(id);
    if (it != idToIterator.end()) {
        return it->second->get();
    }

    return nullptr;
}

unsigned int ObjectManager::registerObject(std::shared_ptr<Object> newObject, const std::string& group, int id) {
    if (!newObject)
        return 0;

    if (!group.empty()) {
        newObject->setGroup(group);
    }

    return registerObject(newObject, id);
}

bool ObjectManager::setObjectGroup(unsigned int id, const std::string& group) {
    Object* obj = getObject(id);
    if (!obj) {
        return false;
    }
    obj->setGroup(group);
    return true;
}

bool ObjectManager::addToGroup(const std::string& group, unsigned int id) {
    Object* obj = getObject(id);
    if (!obj) {
        return false;
    }
    obj->addGroup(group);
    return true;
}

std::vector<Object*> ObjectManager::getObjectsByGroup(const std::string& group) const {
    std::vector<Object*> result;
    if (group.empty()) {
        return result;
    }

    for (const auto& obj : objects) {
        if (obj && obj->isInGroup(group)) {
            result.push_back(obj.get());
        }
    }

    return result;
}

std::vector<std::shared_ptr<Object>> ObjectManager::getSharedObjectsByGroup(const std::string& group) const {
    std::vector<std::shared_ptr<Object>> result;
    if (group.empty()) {
        return result;
    }

    for (const auto& obj : objects) {
        if (obj && obj->isInGroup(group)) {
            result.push_back(obj);
        }
    }

    return result;
}

std::vector<std::string> ObjectManager::getGroups() const {
    std::vector<std::string> result;
    std::unordered_set<std::string> seen;

    for (const auto& obj : objects) {
        if (!obj) {
            continue;
        }
        if (!obj->group.empty() && seen.insert(obj->group).second) {
            result.push_back(obj->group);
        }
        for (const auto& g : obj->groups) {
            if (!g.empty() && seen.insert(g).second) {
                result.push_back(g);
            }
        }
    }

    return result;
}

LightManager::~LightManager() {
    shutdown();
}

unsigned int LightManager::registerLight(std::shared_ptr<Light> newLight) {
    if (!newLight)
        return 0;

    if (newLight->id == 0) {
        newLight->id = nextId++;
    }

    lights.push_back(newLight);
    auto it = --lights.end();
    idToIterator[newLight->id] = it;

    return newLight->id;
}

bool LightManager::removeLight(unsigned int id) {
    auto it = idToIterator.find(id);

    if (it == idToIterator.end()) {
        return false;
    }

    lights.erase(it->second);
    idToIterator.erase(it);

    return true;
}

void LightManager::clear() {
    lights.clear();
    idToIterator.clear();
    nextId = 1;
}

void LightManager::shutdown() {
    clear();
}

Light* LightManager::getLight(unsigned int id) {
    auto it = idToIterator.find(id);

    if (it != idToIterator.end()) {
        return it->second->get();
    }

    return nullptr;
}

std::vector<const DirLight*> LightManager::getDirLights() const {
    std::vector<const DirLight*> result;

    for (const auto& light : lights) {
        if (const auto* dirLight = dynamic_cast<const DirLight*>(light.get())) {
            result.push_back(dirLight);
        }
    }

    return result;
}

std::vector<const PbrPointLight*> LightManager::getPointLights() const {
    std::vector<const PbrPointLight*> result;

    for (const auto& light : lights) {
        if (const auto* pointLight = dynamic_cast<const PbrPointLight*>(light.get())) {
            result.push_back(pointLight);
        }
    }

    return result;
}

ResourceManager::~ResourceManager() {
    shutdown();
}

void ResourceManager::clear() {
    shaders.clear();
    defaultShaderIds.clear();
    idToIterator.clear();
    nameToId.clear();
    nextId = 1;
}

void ResourceManager::shutdown() {
    clear();
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

    auto pbrSource = std::make_shared<ShaderSource>(PbrShader::GetSource());
    if (!pbrSource->isValid()) {
        std::cerr << "[Error] Failed to load default pbr shader sources" << std::endl;
        return false;
    }

    auto pbrShader = createShader(pbrSource, "pbrShader");
    if (!pbrShader) {
        std::cerr << "[Error] Failed to create default pbr shader" << std::endl;
        return false;
    }

    defaultShaderIds.insert(pbrShader->getId());
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
