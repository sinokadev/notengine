#pragma once
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <set>
#include <vector>
#include <chrono>
#include <cmath>

namespace seno {
using Json = nlohmann::json;
namespace fs = std::filesystem;
inline std::string read(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open: " + path.string());
    std::ostringstream out; out << file.rdbuf();
    if (file.bad()) throw std::runtime_error("Cannot read: " + path.string());
    return out.str();
}
inline Json emptyScene() {
    return Json{{"version",8},{"objects",Json::array()},{"lights",Json::array()},
        {"meshes",Json::array()},{"materials",Json::array()},{"models",Json::array()}};
}
inline void require(bool ok, const std::string& message) {
    if (!ok) throw std::runtime_error(message);
}
inline void validate(const Json& j) {
    require(j.is_object(), "Scene root must be an object");
    require(j.contains("version") && j["version"].is_number_integer() && j["version"] == 8, "Seno requires integer version 8");
    for (auto key : {"objects","lights","meshes","materials","models"})
        if (j.contains(key)) require(j[key].is_array(), std::string(key)+" must be an array");
    auto count = [&](const char* key) { return j.contains(key) ? j[key].size() : 0; };
    auto ref = [&](const Json& v, const char* key, const char* collection) {
        require(v.contains(key) && v[key].is_number_integer(), std::string(key)+" must be an integer index");
        const auto i=v[key].get<long long>();
        require(i>=0 && static_cast<size_t>(i)<count(collection), std::string(key)+" index outside "+collection);
    };
    // Validate known vector/scalar shapes recursively while preserving extension fields.
    auto fields = [&](const auto& self, const Json& v) -> void {
        if (v.is_number_float()) require(std::isfinite(v.get<double>()), "Numbers must be finite");
        if (v.is_array()) { for (const auto& e:v) self(self,e); return; }
        if (!v.is_object()) return;
        for (auto it=v.begin();it!=v.end();++it) {
            const auto& k=it.key(); const auto& x=it.value();
            int n=0;
            if (k=="position"||k=="pivot"||k=="scale"||k=="normal"||k=="tangent"||k=="color"||k=="albedo"||k=="ambient"||k=="diffuse"||k=="specular") n=3;
            if (k=="rotation") n=4;
            if (k=="texcoord") n=2;
            if (n) {
                require(x.is_array() && x.size()==static_cast<size_t>(n), k+" must have "+std::to_string(n)+" components");
                double norm=0;
                for (const auto& e:x) { require(e.is_number(),k+" components must be numeric"); if (k=="pivot") require(std::isfinite(e.get<float>()),"pivot components must be finite floats"); norm+=e.get<double>()*e.get<double>(); }
                if (k=="rotation") require(norm>1e-12,"Rotation quaternion cannot be zero (order: w,x,y,z)");
            }
            if (k=="intensity"||k=="metallic"||k=="roughness"||k=="ao"||k=="fov"||k=="near"||k=="far"||k=="size")
                require(x.is_number(),k+" must be numeric");
            if (k=="type"||k=="shader"||k=="path"||k=="obj"||k=="group") require(x.is_string(),k+" must be a string");
            self(self,x);
        }
    };
    fields(fields,j);
    std::set<long long> ids;
    for (auto collection:{"objects","lights","materials","models"}) {
        if (!j.contains(collection)) continue;
        for (const auto& v:j[collection]) {
            require(v.is_object(),std::string(collection)+" entries must be objects");
            if (std::string(collection)=="objects") {
                ref(v,"model","models");
                if (v.contains("id")) {
                    require(v["id"].is_number_integer(),"Object ID must be an integer");
                    auto id=v["id"].get<long long>();
                    require(id>=0 && id<=4294967295LL && ids.insert(id).second,"Object IDs must be unique unsigned 32-bit integers");
                }
                if (v.contains("groups")) {
                    require(v["groups"].is_array(),"groups must be an array");
                    for (const auto& g:v["groups"]) require(g.is_string(),"groups entries must be strings");
                }
            }
            if (std::string(collection)=="models" && !v.contains("obj")) {
                if (v.contains("submeshes")) {
                    require(v["submeshes"].is_array(),"submeshes must be an array");
                    for (const auto& s:v["submeshes"]) { ref(s,"mesh","meshes"); ref(s,"material","materials"); }
                } else { ref(v,"mesh","meshes"); ref(v,"material","materials"); }
            }
            if (std::string(collection)=="materials") require(v.value("shader","")=="pbrShader","Material shader must be pbrShader");
            if (std::string(collection)=="lights") require(v.value("type","")=="DirLight"||v.value("type","")=="PbrPointLight","Unsupported light type");
        }
    }
    if (j.contains("meshes")) for (const auto& mesh:j["meshes"]) {
        if (mesh.is_string()) continue;
        require(mesh.is_object() && mesh.contains("vertices") && mesh["vertices"].is_array() && mesh.contains("indices") && mesh["indices"].is_array(),"Embedded mesh needs vertices and indices arrays");
        for (const auto& v:mesh["vertices"]) require(v.is_object() && v.contains("position"),"Vertex needs position");
        for (const auto& i:mesh["indices"]) require(i.is_number_integer() && i.get<long long>()>=0 && i.get<unsigned long long>()<mesh["vertices"].size(),"Mesh index outside vertices");
    }
    if (j.contains("sky")) require(j["sky"].is_object(),"sky must be an object");
    if (j.contains("camera")) {
        const auto& c=j["camera"]; require(c.is_object(),"camera must be an object");
        require(c.value("near",0.1)>0 && c.value("far",100.0)>c.value("near",0.1),"Camera requires 0 < near < far");
        require(c.value("fov",45.0)>0 && c.value("fov",45.0)<180 && c.value("size",10.0)>0,"Invalid camera FOV or size");
    }
}
struct Document {
    Json value=emptyScene(), saved=value;
    fs::path path;
    std::string disk;
    std::vector<Json> undoStack, redoStack;
    bool dirty() const { return value!=saved; }
    void commit(Json next) {
        validate(next);
        if (next==value) return;
        undoStack.push_back(value);
        if (undoStack.size()>100) undoStack.erase(undoStack.begin());
        redoStack.clear(); value=std::move(next);
    }
    void undo() { if(undoStack.empty())return; redoStack.push_back(value); value=undoStack.back(); undoStack.pop_back(); }
    void redo() { if(redoStack.empty())return; undoStack.push_back(value); value=redoStack.back(); redoStack.pop_back(); }
    void open(const fs::path& file) {
        auto text=read(file); auto next=Json::parse(text); validate(next);
        value=next; saved=next; path=fs::absolute(file); disk=text; undoStack.clear(); redoStack.clear();
    }
    void save(fs::path file, bool overwrite=false) {
        validate(value); file=fs::absolute(file);
        if (fs::exists(file) && !overwrite) {
            require(file==path,"File already exists; confirm overwrite");
            require(read(file)==disk,"File changed on disk; use Save As or confirm overwrite");
        }
        auto text=value.dump(4)+"\n";
        // Stage beside the target so rename stays on the same filesystem.
        auto temp=file; temp += ".tmp-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        try {
            std::ofstream out(temp,std::ios::binary|std::ios::trunc);
            require(bool(out),"Cannot create temporary save file"); out << text; out.close();
            require(bool(out),"Write failed; original file was preserved");
            fs::rename(temp,file);
        } catch (...) { std::error_code ec; fs::remove(temp,ec); throw; }
        path=file; saved=value; disk=text;
    }
};
}
