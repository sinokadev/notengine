#include "../document.h"
#include <knot/scene.h>
#include <glm/gtc/constants.hpp>
#include <iostream>
using namespace seno;
void check(bool ok,const char* message) { if(!ok)throw std::runtime_error(message); }
int main() {
    const auto file=fs::temp_directory_path()/("seno-loader-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".seno");
    try {
        // Empty submeshes let us exercise the real loader without an OpenGL context.
        Json input={{"version",8},{"models",Json::array({{{"submeshes",Json::array()}}})},
            {"objects",Json::array({{{"id",12},{"model",0},{"position",{4,5,6}},
                {"pivot",{1,2,3}},{"scale",{2,3,4}},{"rotation",{0.70710678,0,0.70710678,0}}}})}};
        auto load=[&](knot::Scene& scene,const Json& j) { {std::ofstream out(file);out<<j.dump();} return scene.loadSeno(file.string()); };
        knot::Scene scene;
        check(load(scene,input),"v8 load failed");
        auto object=scene.getObjectManager().getObject(12);
        check(object && object->pivot==glm::vec3(1,2,3),"Pivot not loaded");
        auto matrix=object->getWorldMatrix();
        auto pivotWorld=glm::vec3(matrix*glm::vec4(object->pivot,1));
        check(glm::length(pivotWorld-(object->position+object->pivot))<1e-4f,"Pivot moved under rotation and scale");
        check(glm::length(glm::vec3(matrix*glm::vec4(2,2,3,1))-glm::vec3(5,7,7))<1e-4f,"Pivot transform incorrect");
        for(Json version:{Json(7),Json(9),Json("8"),Json(8.0),Json(nullptr)}) {
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
        fs::remove(file);std::cout<<"Seno v8 loader tests passed\n";return 0;
    } catch(const std::exception& e) {fs::remove(file);std::cerr<<e.what()<<'\n';return 1;}
}
