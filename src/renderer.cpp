// Rendering the object
#include <knot/resources.h>

namespace knot {

    class Renderer {
        bool renderObject(const Object &object) {
            if (object.mesh == nullptr) {
                std::cerr << "Error: Object ID: " << object.id << " - Mesh has not been assigned." << std::endl;
                return false;
            }
            
            if (object.mesh->vao == 0) {
                std::cerr << "Error: Object ID: " << object.id << " - This is an invalid VAO." << std::endl;
                return false;
            }
                        
            glBindVertexArray(object.mesh->vao);
            glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(object.mesh->indices.size()), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            
            return true;
        }
    };

}