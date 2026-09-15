#include <iostream>
#include <cassert>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <knot/resources.h>
#include <knot/utility/mesh_helper.h>

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return 1;
    }

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(640, 480, "SubMesh Test", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Failed to init GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    auto shaderSource = std::make_shared<knot::ShaderSource>(knot::AlphaShader::GetSource());
    auto shader = std::make_shared<knot::Shader>(shaderSource, 1);

    // 1. multi-submesh test
    std::string objPath = knot::getAssetRoot() + "assets/test_submesh.obj";
    auto model = knot::loadModelOBJWithMTL(objPath, shader);

    if (!model) {
        std::cerr << "[FAIL] model is nullptr!\n";
        return 1;
    }

    std::cout << "[INFO] Loaded model successfully!\n";
    std::cout << "[INFO] subMeshes count: " << model->subMeshes.size() << "\n";
    std::cout << "[INFO] boundsCenter: (" << model->boundsCenter.x << ", " << model->boundsCenter.y << ", " << model->boundsCenter.z << ")\n";
    std::cout << "[INFO] boundsRadius: " << model->boundsRadius << "\n";

    if (model->subMeshes.size() != 2) {
        std::cerr << "[FAIL] Expected 2 submeshes, got " << model->subMeshes.size() << "\n";
        return 1;
    }

    for (size_t i = 0; i < model->subMeshes.size(); ++i) {
        const auto& sm = model->subMeshes[i];
        if (!sm.mesh) {
            std::cerr << "[FAIL] subMesh[" << i << "].mesh is nullptr!\n";
            return 1;
        }
        if (!sm.material) {
            std::cerr << "[FAIL] subMesh[" << i << "].material is nullptr!\n";
            return 1;
        }
        std::cout << "[INFO] subMesh[" << i << "] vertexCount=" << sm.mesh->vertices.size() << ", indexCount=" << sm.mesh->indexCount << "\n";

        auto pbrMat = std::dynamic_pointer_cast<knot::PbrMaterial>(sm.material);
        if (pbrMat) {
            std::cout << "[INFO] subMesh[" << i << "] baseAlbedo=(" << pbrMat->baseAlbedo.r << ", " << pbrMat->baseAlbedo.g << ", "
                      << pbrMat->baseAlbedo.b << ")\n";
        }
    }

    // 2. single-submesh test with Untitled.obj
    std::string untitledPath = knot::getAssetRoot() + "assets/Untitled.obj";
    auto untitledModel = knot::loadModelOBJWithMTL(untitledPath, shader);
    if (!untitledModel) {
        std::cerr << "[FAIL] Untitled.obj model is nullptr!\n";
        return 1;
    }
    std::cout << "[INFO] Untitled.obj subMeshes count: " << untitledModel->subMeshes.size() << "\n";

    // 3. runtime material modification test
    auto testObj = std::make_shared<knot::Object>(model);
    assert(testObj->getMaterial(0) != nullptr);
    assert(testObj->getMaterial(1) != nullptr);
    assert(testObj->getMaterial(999) == nullptr);

    auto pbr0 = std::dynamic_pointer_cast<knot::PbrMaterial>(testObj->getMaterial(0));
    assert(pbr0 != nullptr);
    pbr0->setAlbedoColor(glm::vec3(0.5f, 0.5f, 0.5f));
    assert(pbr0->baseAlbedo == glm::vec3(0.5f, 0.5f, 0.5f));

    auto newMat = std::make_shared<knot::AlphaMaterial>(shader, glm::vec3(0.0f, 1.0f, 0.0f));
    bool replaced = testObj->setMaterial(newMat, 1);
    assert(replaced);
    assert(testObj->getMaterial(1) == newMat);

    std::cout << "[INFO] Runtime material replacement and property modification API passed!\n";

    glfwDestroyWindow(window);
    glfwTerminate();
    std::cout << "[SUCCESS] All submesh checks passed!\n";
    return 0;
}
