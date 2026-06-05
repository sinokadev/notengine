#include <vector>
#include <knot/resources.h>

namespace knot {
    class ObjectManager {
    public:

        Object& createObject(Mesh* mesh) {
            unsigned int newId = nextId++;
            
            objects.emplace_back(mesh, newId);

            idToIndex[newId] = objects.size() - 1;

            return objects.back(); 
        }

        Object* getObjectById(unsigned int id) {
            auto it = idToIndex.find(id);
            if (it != idToIndex.end()) {

                return &objects[it->second];
            }
            return nullptr;
        }

    private:
        std::vector<Object> objects;
        std::unordered_map<unsigned int, size_t> idToIndex;

        unsigned int nextId = 1;
    };
}