#include <knot/renderer.h>
#include <GLFW/glfw3.h>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <GL/gl.h>
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
    fs::path directory =
        fs::temp_directory_path() / ("knot-seno-gltf-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Fixtures() {
        fs::create_directories(directory);
    }
    ~Fixtures() {
        std::error_code error;
        fs::remove_all(directory, error);
    }
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
    while (json.size() % 4)
        json.push_back(' ');
    while (binary.size() % 4)
        binary.push_back('\0');
    {
        std::ofstream glb(fixtures.directory / "cube.glb", std::ios::binary);
        writeU32(glb, 0x46546c67);
        writeU32(glb, 2);
        writeU32(glb, static_cast<uint32_t>(28 + json.size() + binary.size()));
        writeU32(glb, static_cast<uint32_t>(json.size()));
        writeU32(glb, 0x4e4f534a);
        glb.write(json.data(), json.size());
        writeU32(glb, static_cast<uint32_t>(binary.size()));
        writeU32(glb, 0x004e4942);
        glb.write(binary.data(), binary.size());
    }

    knot::Scene scene;
    auto document = [](const std::string& path) {
        return Json{{"version", 8},
                    {"models", Json::array({{{"obj", path}}})},
                    {"objects", Json::array({{{"model", 0}}, {{"model", 0}, {"position", {3, 0, 0}}}})}};
    };
    for (const auto& path : {std::string("cube.GLTF"), std::string("cube.glb"), std::string("{assetRoot}/assets/Cube/glTF/Cube.gltf")}) {
        require(scene.loadSeno(fixtures.write("valid.seno", document(path)).string()), "glTF/GLB Seno load failed");
        const auto& objects = scene.getObjectManager().getObjects();
        require(objects.size() == 2, "object instances missing");
        require(objects.front()->model == objects.back()->model, "model sharing lost");
        require(objects.back()->position.x == 3, "Seno transform lost");
        const auto& model = objects.front()->model;
        require(!model->subMeshes.empty() && model->boundsRadius > 0, "geometry missing");
        auto material = std::dynamic_pointer_cast<knot::PbrMaterial>(model->subMeshes.front().material);
        require(material && sg_query_view_state({material->albedoMap}) == SG_RESOURCESTATE_VALID &&
                    sg_query_view_state({material->metallicMap}) == SG_RESOURCESTATE_VALID &&
                    sg_query_view_state({material->roughnessMap}) == SG_RESOURCESTATE_VALID,
                "glTF materials missing");
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
}
// Readback checks exercise actual GPU bindings, pass ordering and blending.
void runRenderTests() {
    auto& renderer = knot::Renderer::get();
    knot::Scene scene;
    auto camera = std::make_shared<knot::PerspectiveCamera>(glm::vec3(0, 0, 5));
    scene.setCamera(camera);
    auto alpha = scene.getShaderManager().getShader("alphaShader");
    auto red = std::make_shared<knot::AlphaMaterial>(alpha, glm::vec3(1, 0, 0));
    auto model = std::make_shared<knot::Model>(knot::createCube(), red);
    for (int i = 0; i < 5; ++i) {
        auto object = std::make_shared<knot::Object>(model);
        object->position.x = (i - 2) * 0.5f;
        scene.getObjectManager().registerObject(object);
    }
    auto light = std::make_shared<knot::PbrPointLight>(glm::vec3(2, 3, 4), glm::vec3(1), 4);
    scene.getLightManager().registerLight(light);
    auto render = [&] {
        renderer.beginFrame(320, 240, {0, 0, 0, 1});
        require(renderer.renderScene(scene, 4.0f / 3), "Scene rendering failed");
        renderer.endFrame();
    };
    auto pixel = [] {
        std::array<unsigned char, 4> result{};
        glReadPixels(160, 120, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, result.data());
        require(glGetError() == GL_NO_ERROR, "GPU readback failed");
        sg_reset_state_cache();
        return result;
    };
    render();
    auto color = pixel();
    require(color[0] > 240 && color[1] < 10 && color[2] < 10, "Instanced alpha geometry did not render red");

    const unsigned char bluePixel[] = {0, 0, 255, 128};
    const auto blueTexture = knot::createTexture(bluePixel, 1, 1);
    auto blue = std::make_shared<knot::TextureMaterial>(alpha, blueTexture, true);
    auto glass = std::make_shared<knot::Object>(knot::createCube(), blue);
    glass->position.z = 2;
    scene.getObjectManager().registerObject(glass);
    render();
    color = pixel();
    require(color[0] > 100 && color[0] < 155 && color[2] > 100 && color[2] < 155, "Translucent pass did not blend over opaque depth");

    auto pbr = scene.getShaderManager().getShader("pbrShader");
    model->setMaterial(std::make_shared<knot::PbrMaterial>(pbr));
    render(); // PBR without an HDR map must still have complete texture bindings.
    scene.loadHDRMap(knot::getAssetRoot() + "assets/DaySkyHDRI015A_2K_HDR.hdr");
    require(scene.getCubeMap() && scene.getIrradianceMap() && scene.getPrefilterMap(), "HDR/IBL baking failed");
    require(sg_query_image_num_mipmaps(sg_query_view_image({scene.getPrefilterMap()})) == 5, "Prefilter mip chain missing");
    for (int count : {0, 2, 1}) {
        scene.getLightManager().clear();
        for (int i = 0; i < count; ++i)
            scene.getLightManager().registerLight(std::make_shared<knot::PbrPointLight>(glm::vec3(i + 1, 3, 4)));
        render(); // Reallocating point-shadow layers must refresh all views.
    }
    renderer.beginFrame(0, 0);
    require(!renderer.renderScene(scene, -1), "Minimized frame should skip rendering");
    renderer.endFrame();
    render();
    const auto texture = knot::createSolidColorTexture({1, 1, 1});
    const auto image = sg_query_view_image({texture});
    knot::destroyTexture(texture);
    require(sg_query_image_state(image) == SG_RESOURCESTATE_INVALID, "Texture image leaked after destruction");
    require(sg_query_view_state({texture}) == SG_RESOURCESTATE_INVALID, "Texture view leaked after destruction");
    std::cout << "[PASS] Instancing, transparency, PBR, shadows, HDR/IBL, resize and texture lifetime\n";
}

} // namespace

int main() {
    if (!glfwInit())
        return 1;
    int result = 0;
    for (int samples : {1, 4}) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, samples == 1 ? 0 : samples);
        glfwWindowHint(GLFW_STENCIL_BITS, 8);
        auto* window = glfwCreateWindow(320, 240, "Sokol renderer test", nullptr, nullptr);
        if (!window) {
            result = 1;
            break;
        }
        glfwMakeContextCurrent(window);
        try {
            require(knot::Renderer::get().init(), "sokol_gfx initialization failed");
            runTests();
            runRenderTests();
            std::cout << "[PASS] Seno glTF/GLB and renderer (samples=" << samples << ")\n";
        } catch (const std::exception& error) {
            std::cerr << "[FAIL] " << error.what() << '\n';
            result = 1;
        }
        knot::Renderer::get().shutdown();
        glfwDestroyWindow(window);
        if (result)
            break;
    }
    glfwTerminate();
    return result;
}
