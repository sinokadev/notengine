#pragma once

#include <filesystem>
#include <string>

namespace knot {

inline std::string resolveSceneAssetPath(std::string path,
                                        const std::filesystem::path& sceneDirectory,
                                        const std::string& assetRoot) {
    const std::string token = "{assetRoot}";
    const auto pos = path.find(token);
    if (pos != std::string::npos) {
        // Explicit asset-root references retain their existing semantics.
        path.replace(pos, token.length(), assetRoot);
        return path;
    }
    if (path.empty() || std::filesystem::path(path).is_absolute()) {
        return path;
    }
    return (sceneDirectory / path).lexically_normal().string();
}

} // namespace knot
