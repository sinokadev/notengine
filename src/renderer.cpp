// Rendering the object
#include <glad/gl.h>
#include <knot/resources.h>
#include <knot/renderer.h>
#include <knot/camera.hpp>

namespace knot {

    bool Renderer::renderObject(const Object &object, const Camera& camera, float aspectRatio) {
        if (object.material == nullptr) {
            std::cerr << "Error: Object ID: " << object.id << " - Material is missing." << std::endl;
            return false;
        }

        object.material->bind();

        // 2. 셰이더 객체를 가져와서 유니폼 세팅
        auto shader = object.material->getShader(); 
        
        shader->set("view", camera.get_view_matrix());
        
        // 2. 프로젝션 행렬 (카메라의 FOV 및 화면 비율)
        glm::mat4 projection = glm::perspective(glm::radians(camera.fov), aspectRatio, 0.1f, 100.0f);
        shader->set("projection", projection);
        
        // 3. 모델 행렬
        shader->set("model", object.getWorldMatrix());

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