#include <knot/resources.h>
#include <knot/utility/texture.h>

#include <fstream>
#include <iostream>
#include <sstream>

namespace knot {

std::vector<std::shared_ptr<Material>> loadMaterialsFromMtl(const std::string& path, ResourceManager& resourceManager) {
    std::vector<std::shared_ptr<Material>> materials;

    std::ifstream mtlFile(path);
    if (!mtlFile.is_open()) {
        std::cerr << "[Error] Failed to open MTL file: " << path << std::endl;
        return materials;
    }

    struct MtlEntry {
        std::string name;
        glm::vec3 kd = glm::vec3(1.0f);
        float ns = 0.0f;
        std::string map_kd;
        float d = 1.0f;
    };

    std::vector<MtlEntry> entries;
    MtlEntry current;

    std::string line;
    while (std::getline(mtlFile, line)) {
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        std::string trimmed = line.substr(start);

        if (trimmed.empty() || trimmed[0] == '#')
            continue;

        std::istringstream iss(trimmed);
        std::string token;
        iss >> token;

        if (token == "newmtl") {
            if (!current.name.empty())
                entries.push_back(current);
            current = MtlEntry();
            iss >> current.name;
        } else if (token == "Kd") {
            float r, g, b;
            if (iss >> r >> g >> b) {
                current.kd = glm::vec3(r, g, b);
            }
        } else if (token == "Ns") {
            iss >> current.ns;
        } else if (token == "d") {
            iss >> current.d;
        } else if (token == "Tr") {
            float tr = 0.0f;
            if (iss >> tr) current.d = 1.0f - tr;
        } else if (token == "map_Kd") {
            std::string tex;
            // Read the rest as filename (options ignored)
            if (iss >> tex) {
                current.map_kd = tex;
            }
        }
    }

    if (!current.name.empty())
        entries.push_back(current);

    mtlFile.close();

    auto shader = resourceManager.getShader("pbrShader");
    if (!shader) {
        std::cerr << "[Error] Failed to find shader: pbrShader" << std::endl;
        return materials;
    }

    // Resolve base directory for relative texture paths
    std::string baseDir;
    auto lastSlash = path.find_last_of("/\\");
    if (lastSlash != std::string::npos)
        baseDir = path.substr(0, lastSlash);

    for (const auto& e : entries) {
        unsigned int albedoTex = 0;

        if (!e.map_kd.empty()) {
            std::string texPath = e.map_kd;

            // If not absolute, resolve relative to .mtl file
            if (!(texPath.size() > 0 && (texPath[0] == '/' || (texPath.size() >= 2 && texPath[1] == ':')))) {
                if (!baseDir.empty())
                    texPath = baseDir + "/" + texPath;
            }

            texPath = resolveAssetPath(texPath);
            albedoTex = loadTextureFromFile(texPath);
        }

        float roughness = glm::clamp(1.0f - (e.ns / 1000.0f), 0.0f, 1.0f);

        materials.push_back(std::make_shared<PbrMaterial>(
            shader,
            e.kd,
            0.0f,
            roughness,
            1.0f,
            albedoTex
        ));
    }

    return materials;
}

} // namespace knot
