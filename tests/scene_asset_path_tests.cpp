#include "test_support.h"
#include <knot/utility/scene_asset_path.h>
#include <filesystem>

namespace {
const auto scene = std::filesystem::temp_directory_path() / "knot-scene";
const auto root = std::filesystem::temp_directory_path() / "knot-assets";

void relativePaths() {
    CHECK(knot::resolveSceneAssetPath("models/cube.gltf", scene, root.string()) == (scene / "models/cube.gltf").string());
    CHECK(knot::resolveSceneAssetPath("./models/../cube.glb", scene, root.string()) == (scene / "cube.glb").string());
    CHECK(knot::resolveSceneAssetPath("../shared/a.obj", scene, root.string()) == (scene.parent_path() / "shared/a.obj").string());
}

void absoluteAndEmptyPaths() {
    CHECK(knot::resolveSceneAssetPath("", scene, root.string()).empty());
    const auto absolute = (root / "mesh.obj").string();
    CHECK(knot::resolveSceneAssetPath(absolute, scene, "unused") == absolute);
    CHECK(knot::resolveSceneAssetPath("mesh.obj", {}, "unused") == "mesh.obj");
}

void explicitAssetRoot() {
    CHECK(knot::resolveSceneAssetPath("{assetRoot}/mesh.obj", scene, root.string()) == root.string() + "/mesh.obj");
    CHECK(knot::resolveSceneAssetPath("{assetRoot}", scene, root.string()) == root.string());
    // Explicit root substitution intentionally bypasses relative-path normalization.
    CHECK(knot::resolveSceneAssetPath("{assetRoot}/a/../b.obj", scene, "assets") == "assets/a/../b.obj");
}

void spacesAndUnknownTokens() {
    CHECK(knot::resolveSceneAssetPath("models/my mesh.obj", scene, root.string()) == (scene / "models/my mesh.obj").string());
    CHECK(knot::resolveSceneAssetPath("{unknown}/mesh.obj", scene, root.string()) == (scene / "{unknown}/mesh.obj").string());
}
} // namespace

int main() {
    return runTests({{"relative paths", relativePaths}, {"absolute and empty paths", absoluteAndEmptyPaths},
                     {"explicit asset root", explicitAssetRoot}, {"spaces and unknown tokens", spacesAndUnknownTokens}});
}
