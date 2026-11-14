#pragma once

#include <filesystem>
#include <string>

namespace knot {

inline std::string resolveSceneAssetPath(std::string path, const std::filesystem::path& sceneDirectory, const std::string& sourceRoot) {
    for (const std::string& token : {std::string("{sourceRoot}"), std::string("{assetRoot}")}) {
        const auto pos = path.find(token);
        if (pos != std::string::npos) {
            path.replace(pos, token.length(), sourceRoot);
            return path;
        }
    }
    if (path.empty() || std::filesystem::path(path).is_absolute()) {
        return path;
    }
    return (sceneDirectory / path).lexically_normal().string();
}

} // namespace knot
