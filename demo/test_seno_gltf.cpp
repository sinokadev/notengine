#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <knot/scene.h>
#include <knot/utility.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using Json = nlohmann::json;
namespace fs = std::filesystem;

void require(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}

struct Fixtures {
    fs::path directory = fs::temp_directory_path() /
        ("knot-seno-gltf-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Fixtures() { fs::create_directories(directory); }
    ~Fixtures() { std::error_code error; fs::remove_all(directory, error); }
    fs::path write(const char* name, const Json& value) {
        auto path = directory / name;
        std::ofstream(path) << value.dump();
        return path;
    }
};

void writeU32(std::ostream& stream, uint32_t value) {
    for (int i = 0; i < 4; ++i)
        stream.put(static_cast<char>((value >> (i * 8)) & 255));
}

void runTests() {
    Fixtures fixtures;
    const fs::path assets = fs::path(knot::getAssetRoot()) / "assets";
    const fs::path cubeDir = assets / "Cube/glTF";
    Json cube;
    std::ifstream(cubeDir / "Cube.gltf") >> cube;
    for (const auto& image : cube["images"])
        fs::copy_file(cubeDir / image["uri"].get<std::string>(), fixtures.directory / image["uri"].get<std::string>());
    const auto bufferName = cube["buffers"][0]["uri"].get<std::string>();
    fs::copy_file(cubeDir / bufferName, fixtures.directory / bufferName);
    fixtures.write("cube.GLTF", cube);

    // Build a binary glTF fixture with the same geometry and materials.
    std::ifstream input(cubeDir / bufferName, std::ios::binary);
    std::string binary((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    cube["buffers"][0].erase("uri");
    std::string json = cube.dump();
    while (json.size() % 4) json.push_back(' ');
    while (binary.size() % 4) binary.push_back('\0');
    {
        std::ofstream glb(fixtures.directory / "cube.glb", std::ios::binary);
        writeU32(glb, 0x46546c67); writeU32(glb, 2);
        writeU32(glb, static_cast<uint32_t>(28 + json.size() + binary.size()));
        writeU32(glb, static_cast<uint32_t>(json.size())); writeU32(glb, 0x4e4f534a);
        glb.write(json.data(), json.size());
        writeU32(glb, static_cast<uint32_t>(binary.size())); writeU32(glb, 0x004e4942);
        glb.write(binary.data(), binary.size());
    }

    knot::Scene scene;
    auto document = [](const std::string& path) {
        return Json{{"version", 8}, {"models", Json::array({{{"obj", path}}})},
                    {"objects", Json::array({{{"model", 0}}, {{"model", 0}, {"position", {3, 0, 0}}}})}};
    };
    for (const auto& path : {std::string("cube.GLTF"), std::string("cube.glb"),
                            std::string("{assetRoot}/assets/Cube/glTF/Cube.gltf")}) {
        require(scene.loadSeno(fixtures.write("valid.seno", document(path)).string()), "glTF/GLB Seno load failed");
        const auto& objects = scene.getObjectManager().getObjects();
        require(objects.size() == 2, "object instances missing");
        require(objects.front()->model == objects.back()->model, "model sharing lost");
        require(objects.back()->position.x == 3, "Seno transform lost");
        const auto& model = objects.front()->model;
        require(!model->subMeshes.empty() && model->boundsRadius > 0, "geometry missing");
        auto material = std::dynamic_pointer_cast<knot::PbrMaterial>(model->subMeshes.front().material);
        require(material && glIsTexture(material->albedoMap) && glIsTexture(material->metallicMap) &&
                glIsTexture(material->roughnessMap), "glTF materials missing");
    }

    for (const auto* field : {"mesh", "material", "submeshes"}) {
        auto invalid = document("cube.GLTF");
        invalid["models"][0][field] = (std::string(field) == "submeshes") ? Json::array() : Json(0);
        require(!scene.loadSeno(fixtures.write("invalid.seno", invalid).string()), "glTF override accepted");
    }
    for (const auto* path : {"cube.GLTF", "cube.glb"}) {
        Json invalid{{"version", 8}, {"meshes", Json::array({path})}};
        require(!scene.loadSeno(fixtures.write("invalid.seno", invalid).string()), "glTF accepted as a bare mesh");
    }
    require(!scene.loadSeno(fixtures.write("missing.seno", document("missing.gltf")).string()), "missing glTF accepted");

    auto legacy = document("{assetRoot}/assets/test_submesh.obj");
    legacy["meshes"] = Json::array({"{assetRoot}/assets/notbox.obj"});
    legacy["materials"] = Json::array({{{"shader", "pbrShader"}}});
    legacy["models"].push_back({{"mesh", 0}, {"material", 0}});
    legacy["models"].push_back({{"submeshes", Json::array({{{"mesh", 0}, {"material", 0}}})}});
    legacy["objects"].push_back({{"model", 1}});
    legacy["objects"].push_back({{"model", 2}});
    require(scene.loadSeno(fixtures.write("legacy.seno", legacy).string()), "existing Seno model formats regressed");
    require(scene.getObjectManager().getObjects().size() == 4, "legacy objects missing");
    require(glGetError() == GL_NO_ERROR, "OpenGL error during model loading");
}
} // namespace

int main() {
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    auto* window = glfwCreateWindow(320, 240, "Seno glTF test", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    int result = 0;
    try {
        require(gladLoadGL(glfwGetProcAddress), "GLAD initialization failed");
        runTests();
        std::cout << "[PASS] Seno glTF/GLB paths, materials, validation and legacy formats\n";
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        result = 1;
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}
