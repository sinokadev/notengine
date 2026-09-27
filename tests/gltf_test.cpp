#include <knot/scene.h>
#include <cgltf/cgltf.h>
#include <nlohmann/json.hpp>
#include <stb/stb_image.h>

#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using nlohmann::json;
namespace fs = std::filesystem;
namespace {
void check(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
bool near(float a, float b) {
    return std::abs(a - b) < 0.0001f;
}
struct Fixtures {
    fs::path directory = fs::temp_directory_path() / ("knot-gltf-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::vector<unsigned char> buffer;
    json document;
    Fixtures() {
        fs::create_directories(directory);
        // Interleaved POSITION, NORMAL and UV; an indexed triangle.
        float vertices[] = {0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1};
        auto* bytes = reinterpret_cast<unsigned char*>(vertices);
        buffer.assign(bytes, bytes + sizeof(vertices));
        buffer.insert(buffer.end(), {0, 0, 1, 0, 2, 0});
        document = {
            {"asset", {{"version", "2.0"}}},
            {"buffers", {{{"uri", "mesh%20data.bin"}, {"byteLength", buffer.size()}}}},
            {"bufferViews", {{{"buffer", 0}, {"byteLength", 96}, {"byteStride", 32}}, {{"buffer", 0}, {"byteOffset", 96}, {"byteLength", 6}}}},
            {"accessors",
             {{{"bufferView", 0}, {"componentType", 5126}, {"count", 3}, {"type", "VEC3"}, {"min", {0, 0, 0}}, {"max", {1, 1, 0}}},
              {{"bufferView", 0}, {"byteOffset", 12}, {"componentType", 5126}, {"count", 3}, {"type", "VEC3"}},
              {{"bufferView", 0}, {"byteOffset", 24}, {"componentType", 5126}, {"count", 3}, {"type", "VEC2"}},
              {{"bufferView", 1}, {"componentType", 5123}, {"count", 3}, {"type", "SCALAR"}}}},
            {"meshes", {{{"primitives", {{{"attributes", {{"POSITION", 0}, {"NORMAL", 1}, {"TEXCOORD_0", 2}}}, {"indices", 3}, {"material", 0}}}}}}},
            {"materials", {{{"pbrMetallicRoughness", {{"baseColorFactor", {0.5, 1, 1, 1}}, {"metallicFactor", 0.5}, {"roughnessFactor", 0.25}}}}}},
            {"nodes",
             {{{"translation", {10, 0, 0}}, {"children", {1, 2, 4, 5}}},
              {{"name", "mirrored"}, {"mesh", 0}, {"translation", {0, 2, 0}}, {"scale", {-2, 3, 1}}},
              {{"name", "instance"}, {"mesh", 0}, {"translation", {0, 0, 5}}},
              {{"name", "other-scene"}, {"mesh", 0}, {"translation", {99, 0, 0}}},
              {{"camera", 0}, {"translation", {0, 0, 4}}},
              {{"extensions", {{"KHR_lights_punctual", {{"light", 0}}}}}, {"translation", {0, 3, 0}}}}},
            {"cameras", {{{"type", "perspective"}, {"perspective", {{"yfov", 1.0}, {"znear", 0.1}, {"aspectRatio", 2.0}}}}}},
            {"extensionsUsed", {"KHR_lights_punctual"}},
            {"extensions", {{"KHR_lights_punctual", {{"lights", {{{"type", "directional"}, {"color", {0.5, 1, 0.25}}, {"intensity", 2.0}}}}}}}},
            {"scenes", {{{"nodes", {3}}}, {{"nodes", {0}}}}},
            {"scene", 1}};
        binary("mesh data.bin", buffer);
    }
    ~Fixtures() {
        std::error_code error;
        fs::remove_all(directory, error);
    }
    void binary(const std::string& name, const std::vector<unsigned char>& bytes) {
        std::ofstream out(directory / name, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    }
    std::string write(json doc, const std::string& name = "test.gltf") {
        std::ofstream(directory / name) << doc.dump();
        return (directory / name).string();
    }
    std::string glb(json doc) {
        doc["buffers"][0].erase("uri");
        std::string text = doc.dump();
        while (text.size() % 4)
            text += ' ';
        auto bin = buffer;
        while (bin.size() % 4)
            bin.push_back(0);
        std::vector<unsigned char> bytes;
        auto word = [&](uint32_t v) {
            for (int i = 0; i < 4; ++i)
                bytes.push_back(static_cast<unsigned char>(v >> (8 * i)));
        };
        word(0x46546c67);
        word(2);
        word(static_cast<uint32_t>(28 + text.size() + bin.size()));
        word(static_cast<uint32_t>(text.size()));
        word(0x4e4f534a);
        bytes.insert(bytes.end(), text.begin(), text.end());
        word(static_cast<uint32_t>(bin.size()));
        word(0x004e4942);
        bytes.insert(bytes.end(), bin.begin(), bin.end());
        binary("test.glb", bytes);
        return (directory / "test.glb").string();
    }
};
std::array<unsigned char, 8> texture(GLuint id) {
    std::array<unsigned char, 8> pixels{};
    glBindTexture(GL_TEXTURE_2D, id);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    return pixels;
}
void run() {
    Fixtures fixture;
    knot::Scene scene;
    auto shader = scene.getResourceManager().getShader("pbrShader");
    check(shader && shader->isValid(), "PBR shader failed to compile");
    auto path = fixture.write(fixture.document);
    auto model = knot::loadModelGLTF(path, shader);
    check(model && model->subMeshes.size() == 1, "model should load mesh resources once");
    check(near(model->getMesh()->vertices[1].Position.x, 1), "model must ignore scene transforms");
    check(model->getMesh()->isReady(), "mesh upload failed");
    check(near(model->boundsCenter.x, .5f), "model bounds are wrong");
    check(scene.loadGLTF(path), "scene load failed");
    check(scene.getObjectManager().getObjects().size() == 2, "default scene or mesh instances lost");
    auto* mirrored = scene.getObjectManager().getGroup("mirrored").at(0);
    check(near(mirrored->position.x, 10) && near(mirrored->position.y, 2), "hierarchy translation lost");
    auto mesh = mirrored->getMesh();
    check(near(mesh->vertices[1].Position.x, -2) && near(mesh->vertices[2].Position.y, 3), "scale lost");
    check(mesh->indices[1] == 2 && mesh->indices[2] == 1, "negative scale winding lost");
    check(mesh->vertices[0].TangentSign == -1, "tangent handedness lost");
    check(near(scene.getCamera().position.x, 10) && near(scene.getCamera().position.z, 4), "camera transform lost");
    auto projection = scene.getCamera().getProjectionMatrix(1);
    check(near(projection[1][1] / projection[0][0], 2) && near(projection[2][2], -1), "camera projection lost");
    auto* light = scene.getLightManager().getDirLights().at(0);
    check(near(light->diffuse.x, 1) && near(light->diffuse.y, 2), "light color/intensity lost");
    auto preserved = scene.getObjectManager().getObjects().front();
    check(!scene.loadGLTF(path, 5) && scene.getObjectManager().getObjects().front() == preserved, "failed import changed scene");
    check(!scene.loadGLTF("missing.gltf") && scene.getObjectManager().getObjects().front() == preserved, "missing file changed scene");
    check(scene.loadGLTF(path, 0) && scene.getObjectManager().getObjects().size() == 1, "explicit scene failed");
    check(scene.loadGLTF(fixture.glb(fixture.document)), "GLB scene failed");
    check(knot::loadModelGLTF(fixture.glb(fixture.document), shader) != nullptr, "GLB model failed");

    auto doc = fixture.document;
    doc.erase("scene");
    check(scene.loadGLTF(fixture.write(doc)) && scene.getObjectManager().getGroup("other-scene").size() == 1, "first-scene fallback failed");
    doc.erase("scenes");
    check(scene.loadGLTF(fixture.write(doc)) && scene.getObjectManager().getObjects().size() == 3, "root-node fallback failed");
    doc["scenes"] = {{{"nodes", json::array()}}};
    check(scene.loadGLTF(fixture.write(doc)) && scene.getObjectManager().getObjects().empty(), "empty scene failed");

    doc = fixture.document;
    // Matrix with shear must survive flattening; it cannot be represented by TRS alone.
    doc["nodes"][1].erase("scale");
    doc["nodes"][1].erase("translation");
    doc["nodes"][1]["matrix"] = {1, 0, 0, 0, 0.5, 1, 0, 0, 0, 0, 1, 0, 0, 2, 0, 1};
    check(scene.loadGLTF(fixture.write(doc)), "matrix scene failed");
    check(near(scene.getObjectManager().getGroup("mirrored")[0]->getMesh()->vertices[2].Position.x, .5f), "shear lost");
    doc = fixture.document;
    auto& primitive = doc["meshes"][0]["primitives"][0];
    primitive.erase("indices");
    primitive["attributes"].erase("NORMAL");
    primitive["attributes"].erase("TEXCOORD_0");
    for (int mode : {4, 5, 6}) {
        primitive["mode"] = mode;
        auto simple = knot::loadModelGLTF(fixture.write(doc), shader);
        check(simple && simple->getMesh()->indices.size() == 3, "unindexed triangle mode failed");
        for (const auto& v : simple->getMesh()->vertices)
            check(near(v.Normal.z, 1) && std::isfinite(v.Tangent.x), "missing normal/UV fallback is invalid");
    }
    primitive["mode"] = 1;
    check(!knot::loadModelGLTF(fixture.write(doc), shader), "lines should be rejected");
    doc = fixture.document;
    doc["extensionsRequired"] = {"KHR_draco_mesh_compression"};
    check(!knot::loadModelGLTF(fixture.write(doc), shader), "required extension should be rejected");
    doc = fixture.document;
    doc["meshes"][0]["primitives"][0]["targets"] = {{{"POSITION", 0}}};
    check(!knot::loadModelGLTF(fixture.write(doc), shader), "morph targets should be rejected");
    auto bad = fixture.buffer;
    bad[100] = 99;
    fixture.binary("mesh data.bin", bad);
    check(!knot::loadModelGLTF(fixture.write(fixture.document), shader), "out-of-range indices should fail");
    fixture.binary("mesh data.bin", fixture.buffer);

    // Sparse POSITION with no base buffer; all three vertices are supplied as overrides.
    doc = fixture.document;
    std::vector<unsigned char> sparse = {0, 1, 2, 0};
    float positions[] = {0, 0, 0, 2, 0, 0, 0, 2, 0};
    const auto* bytes = reinterpret_cast<unsigned char*>(positions);
    sparse.insert(sparse.end(), bytes, bytes + sizeof(positions));
    fixture.binary("sparse.bin", sparse);
    doc["buffers"].push_back({{"uri", "sparse.bin"}, {"byteLength", sparse.size()}});
    doc["bufferViews"].push_back({{"buffer", 1}, {"byteLength", 3}});
    doc["bufferViews"].push_back({{"buffer", 1}, {"byteOffset", 4}, {"byteLength", 36}});
    doc["accessors"][0].erase("bufferView");
    doc["accessors"][0]["sparse"] = {{"count", 3}, {"indices", {{"bufferView", 2}, {"componentType", 5121}}}, {"values", {{"bufferView", 3}}}};
    auto sparseModel = knot::loadModelGLTF(fixture.write(doc), shader);
    check(sparseModel && near(sparseModel->getMesh()->vertices[1].Position.x, 2), "sparse accessor failed");

    const std::string png = "iVBORw0KGgoAAAANSUhEUgAAAAEAAAACCAYAAACZgbYnAAAAEklEQVR4nGNocDjwn+E/A8N/ABa/BH7HG3LRAAAAAElFTkSuQmCC";
    doc = fixture.document;
    doc["images"] = {{{"uri", "data:image/png;base64," + png}}};
    doc["samplers"] = {{{"wrapS", 33071}, {"wrapT", 10497}, {"magFilter", 9728}}};
    doc["textures"] = {{{"source", 0}, {"sampler", 0}}};
    auto& pbr = doc["materials"][0]["pbrMetallicRoughness"];
    pbr["baseColorTexture"] = {{"index", 0}};
    pbr["metallicRoughnessTexture"] = {{"index", 0}};
    auto textured = knot::loadModelGLTF(fixture.write(doc), shader);
    check(textured != nullptr, "data URI image failed");
    auto mat = std::dynamic_pointer_cast<knot::PbrMaterial>(textured->getMaterial());
    auto albedo = texture(mat->albedoMap), metal = texture(mat->metallicMap), rough = texture(mat->roughnessMap);
    check(albedo[0] == 28 && albedo[4] == 128, "albedo factor, sRGB or image orientation is wrong");
    check(metal[0] == 96 && rough[0] == 16, "packed material channels/factors are wrong");
    GLint wrap = 0;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &wrap);
    check(wrap == GL_CLAMP_TO_EDGE, "sampler lost");
    GLuint ownedTexture = mat->albedoMap;
    textured.reset();
    mat.reset();
    check(!glIsTexture(ownedTexture), "imported texture leaked");
    // External and buffer-view images use the same orientation and material path.
    void* decoded = nullptr;
    cgltf_options options{};
    auto pngSize = png.size() / 4 * 3;
    check(cgltf_load_buffer_base64(&options, pngSize, png.c_str(), &decoded) == cgltf_result_success, "fixture decode failed");
    std::vector<unsigned char> pngBytes(static_cast<unsigned char*>(decoded), static_cast<unsigned char*>(decoded) + pngSize);
    std::free(decoded);
    fixture.binary("color image.png", pngBytes);
    auto legacyTexture = knot::loadTextureFromFile((fixture.directory / "color image.png").string());
    auto legacyPixels = texture(legacyTexture);
    check(legacyPixels[0] == 255 && legacyPixels[4] == 128, "glTF changed the legacy texture loader's orientation");
    glDeleteTextures(1, &legacyTexture);
    doc["images"][0]["uri"] = "color%20image.png";
    textured = knot::loadModelGLTF(fixture.write(doc), shader);
    check(textured != nullptr, "relative image URI failed");
    mat = std::dynamic_pointer_cast<knot::PbrMaterial>(textured->getMaterial());
    check(texture(mat->albedoMap)[0] == 28, "legacy texture loader changed glTF orientation");
    doc["buffers"].push_back({{"uri", "color%20image.png"}, {"byteLength", pngBytes.size()}});
    doc["bufferViews"].push_back({{"buffer", 1}, {"byteLength", pngBytes.size()}});
    doc["images"][0] = {{"bufferView", 2}, {"mimeType", "image/png"}};
    check(knot::loadModelGLTF(fixture.write(doc), shader) != nullptr, "buffer-view image failed");
    auto embedded = doc;
    auto savedBuffer = fixture.buffer;
    while (fixture.buffer.size() % 4)
        fixture.buffer.push_back(0);
    auto imageOffset = fixture.buffer.size();
    fixture.buffer.insert(fixture.buffer.end(), pngBytes.begin(), pngBytes.end());
    embedded["buffers"].erase(1);
    embedded["buffers"][0]["byteLength"] = fixture.buffer.size();
    embedded["bufferViews"][2] = {{"buffer", 0}, {"byteOffset", imageOffset}, {"byteLength", pngBytes.size()}};
    check(knot::loadModelGLTF(fixture.glb(embedded), shader) != nullptr, "GLB embedded image failed");
    fixture.buffer = std::move(savedBuffer);
    doc["images"][0] = {{"uri", "missing.png"}};
    preserved = scene.getObjectManager().getObjects().front();
    check(!scene.loadGLTF(fixture.write(doc)) && scene.getObjectManager().getObjects().front() == preserved, "missing image changed scene");
    // Seno model references use paths relative to the Seno file.
    fixture.write(fixture.document, "asset.gltf");
    json seno = {{"version", 8}, {"models", {{{"gltf", "asset.gltf"}}}}, {"objects", {{{"model", 0}}}}};
    check(scene.loadSeno(fixture.write(seno, "test.seno")) && scene.getObjectManager().getObjects().size() == 1, "Seno glTF reference failed");
    check(glGetError() == GL_NO_ERROR, "OpenGL error during import");
}
} // namespace
int main() {
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed (a display is required)\n";
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    auto* window = glfwCreateWindow(64, 64, "glTF import tests", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    int result = 0;
    try {
        check(gladLoadGL(glfwGetProcAddress), "GLAD initialization failed");
        run();
        std::cout << "glTF model/scene import tests passed\n";
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        result = 1;
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}
