#include "../document.h"
#include <knot/utility/scene_asset_path.h>
#include <knot/scene.h>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <sstream>
using namespace seno;
void check(bool ok,const char* message) { if(!ok)throw std::runtime_error(message); }
int main() {
    const auto file=fs::temp_directory_path()/("seno-loader-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".seno");
    try {
        const auto sceneDir = file.parent_path() / "scene folder";
        auto resolve = [&](const std::string& path) {
            return knot::resolveSceneAssetPath(path, sceneDir, "assets/");
        };
        check(resolve("model.obj") == (sceneDir / "model.obj").string(), "Sibling OBJ path not resolved");
        check(resolve("models/../textures/sky.hdr") == (sceneDir / "textures/sky.hdr").string(), "Nested HDR path not normalized");
        check(resolve("../shared/model.obj") == (sceneDir.parent_path() / "shared/model.obj").string(), "Parent-relative path not resolved");
        check(resolve(file.string()) == file.string(), "Absolute path changed");
        check(resolve("{assetRoot}/model.obj") == "assets//model.obj", "Asset-root reference rebased onto scene directory");
        check(resolve("").empty(), "Empty path became a directory");
        // Empty submeshes let us exercise the real loader without an OpenGL context.
        Json input={{"version",10},{"models",Json::array({{{"submeshes",Json::array()}}})},
            {"objects",Json::array({{{"id",12},{"name","player_main"},{"model",0},{"position",{4,5,6}},
                {"pivot",{1,2,3}},{"scale",{2,3,4}},{"rotation",{0.70710678,0,0.70710678,0}}}})}};
        auto load=[&](knot::Scene& scene,const Json& j) { {std::ofstream out(file);out<<j.dump();} return scene.loadSeno(file.string()); };
        knot::Scene scene;
        check(load(scene,input),"v10 load failed");
        auto object=scene.getObjectManager().getObject(12);
        check(object && object->getName()=="player_main" && object->hasNamePrefix("player"),"Object name not loaded");
        check(object && object->pivot==glm::vec3(1,2,3),"Pivot not loaded");
        auto matrix=object->getWorldMatrix();
        auto pivotWorld=glm::vec3(matrix*glm::vec4(object->pivot,1));
        check(glm::length(pivotWorld-(object->position+object->pivot))<1e-4f,"Pivot moved under rotation and scale");
        check(glm::length(glm::vec3(matrix*glm::vec4(2,2,3,1))-glm::vec3(5,7,7))<1e-4f,"Pivot transform incorrect");
        for(Json version:{Json(7),Json(9),Json("10"),Json(10.0),Json(nullptr)}) {
            auto bad=input;bad["version"]=version;
            check(!load(scene,bad),"Invalid version accepted");
            check(scene.getObjectManager().getObject(12)==object,"Invalid version cleared scene");
        }
        auto bad=input;bad.erase("version");check(!load(scene,bad),"Missing version accepted");
        for(Json pivot:{Json{1,2},Json{1,2,3,4},Json{1,"x",3},Json{1e100,0,0},Json(nullptr)}) {
            bad=input;bad["objects"][0]["pivot"]=pivot;check(!load(scene,bad),"Invalid pivot accepted");
            check(scene.getObjectManager().getObject(12)==object,"Invalid pivot cleared scene");
        }
        input["objects"][0].erase("pivot");check(load(scene,input),"Optional pivot rejected");
        check(scene.getObjectManager().getObject(12)->pivot==glm::vec3(0),"Default pivot not zero");
        auto lightInput = input;
        lightInput["lights"] = Json::array({{{"type","PbrPointLight"}},
            {{"type","PbrPointLight"},{"castsShadow",false},{"intensity",3.0}}});
        check(load(scene,lightInput),"Point shadow option load failed");
        const auto lights = scene.getLightManager().getPointLights();
        check(lights.size()==2 && lights[0]->castsShadow && !lights[1]->castsShadow,
              "Point shadow default or explicit false not loaded");
        check(lights[1]->intensity==3.0f,"Unshadowed light lost intensity");
        // Exercise the actual loader without requiring OpenGL: a missing OBJ
        // reports the resolved path before GPU allocation can take place.
        Json missing = {{"version",10},{"meshes",Json::array({"missing-model.obj"})}};
        { std::ofstream out(file); out << missing.dump(); }
        for (const auto& scenePath : {file, fs::relative(file, fs::current_path())}) {
            std::ostringstream errors;
            auto* previous = std::cerr.rdbuf(errors.rdbuf());
            const bool loaded = scene.loadSeno(scenePath.string());
            std::cerr.rdbuf(previous);
            check(!loaded, "Missing OBJ accepted");
            const auto expected = (file.parent_path() / "missing-model.obj").string();
            check(errors.str().find("Failed to load mesh: " + expected) != std::string::npos,
                  "Scene loader did not resolve mesh relative to the scene file");
        }
        // A file that fails halfway must leave the previously loaded scene alone.
        auto keptInput=input;keptInput["lights"]=Json::array({{{"type","PbrPointLight"}}});
        keptInput["camera"]=Json{{"type","PerspectiveCamera"}};
        check(load(scene,keptInput),"Reload failed");
        auto* kept=scene.getObjectManager().getObject(12);
        auto* keptCamera=&scene.getCamera();
        auto rejects=[&](const Json& j,const char* message) {
            check(!load(scene,j),message);
            check(scene.getObjectManager().getObject(12)==kept && scene.getLightManager().getPointLights().size()==1 &&
                  &scene.getCamera()==keptCamera,"Rejected file replaced the loaded scene");
        };
        bad=input;bad["objects"][0]["position"]={1};rejects(bad,"Short position array accepted");
        bad=input;bad["objects"][0]["rotation"]={1,0,0};rejects(bad,"Three-component rotation accepted");
        bad=input;bad["objects"].push_back({{"model",5}});rejects(bad,"Out-of-range model index accepted");
        bad=input;bad["camera"]=Json{{"type","Perspective"}};rejects(bad,"Unknown camera type accepted");
        auto triangle=Json::parse(R"({"vertices":[{"position":[0,0,0]},{"position":[1,0,0]},{"position":[0,1,0]}]})");
        for(Json indices:{Json{0,1,3},Json{0,1},Json{0,1,-1},Json{0,1,4294967296LL},Json{0,1,1.5}}) {
            bad=input;auto mesh=triangle;mesh["indices"]=indices;bad["meshes"]=Json::array({mesh});
            rejects(bad,"Invalid embedded mesh indices accepted");
        }
        bad=input;auto missingPosition=triangle;missingPosition["vertices"][1].erase("position");
        missingPosition["indices"]={0,1,2};bad["meshes"]=Json::array({missingPosition});
        rejects(bad,"Vertex without position accepted");
        // Straight up/down used to hit the quatLookAt singularity and come out tilted 45 degrees.
        auto sunInput=input;sunInput["lights"]=Json::array({{{"type","DirLight"}}});
        check(load(scene,sunInput),"DirLight load failed");
        check(glm::length(scene.getLightManager().getDirLights().at(0)->getDirection()-glm::vec3(0,-1,0))<1e-4f,"Default DirLight is not straight down");
        for(float y:{-1.0f,1.0f}) {
            knot::PerspectiveCamera camera(glm::vec3(0,20*-y,0));camera.lookAtTarget(glm::vec3(0));
            check(glm::length(camera.getFront()-glm::vec3(0,y,0))<1e-4f,"Vertical lookAtTarget missed the target");
            check(glm::length(camera.getRight()-glm::vec3(1,0,0))<1e-4f,"Vertical lookAtTarget flipped the right vector");
        }
        // Almost vertical but still fine for quatLookAt: keep the old +Y up result (right = -X here).
        knot::PerspectiveCamera nearVertical(glm::vec3(0,20,-0.5f));nearVertical.lookAtTarget(glm::vec3(0));
        check(glm::length(nearVertical.getRight()-glm::vec3(-1,0,0))<1e-3f,"Near-vertical lookAtTarget changed roll");
        fs::remove(file);std::cout<<"Seno v10 loader tests passed\n";return 0;
    } catch(const std::exception& e) {fs::remove(file);std::cerr<<e.what()<<'\n';return 1;}
}
