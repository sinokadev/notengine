// Rendering the object
#include <glad/gl.h>
#include <knot/resources.h>
#include <knot/renderer.h>

namespace knot {

    bool Renderer::renderObject(const Object &object) {
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
    bool Renderer::init(GLADloadfunc loadProc) {
        if (!gladLoadGL(loadProc)) {
            return false;
        }

        glEnable(GL_MULTISAMPLE);
        glEnable(GL_DEPTH_TEST);

        return true;
    }
}