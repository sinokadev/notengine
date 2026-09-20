#include "document.h"
#include <knot/renderer.h>
#include <knot/resources.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <cstdlib>

using seno::Json;
namespace fs=std::filesystem;
namespace {
// Keep navigation projection separate from the camera stored in the document.
class ViewportCamera : public knot::PerspectiveCamera {
public:
    bool orthographic=false;
    float span=10.f;
    glm::mat4 getProjectionMatrix(float aspect) const override {
        if(!orthographic)return PerspectiveCamera::getProjectionMatrix(aspect);
        return glm::ortho(-span*aspect*.5f,span*aspect*.5f,-span*.5f,span*.5f,nearPlane,farPlane);
    }
};
class Editor {
    GLFWwindow* window;
    seno::Document doc;
    Json draft=doc.value;
    std::string source=draft.dump(4), sourceBase=source;
    std::string collection="objects", filter, message="Ready", fileName;
    int selected=-1;
    bool quit=false, showBrowser=false, saveMode=false, overwrite=false;
    bool autoPreview=true, previewStale=true;
    enum class Action { None, New, Open, Quit } pending=Action::None;
    bool askUnsaved=false;
    fs::path directory=fs::current_path(), overwritePath;
    std::unique_ptr<knot::Scene> scene;
    std::shared_ptr<ViewportCamera> camera=std::make_shared<ViewportCamera>();
    bool navigating=false;
    GLuint texture=0;
    int texWidth=0, texHeight=0;
    float yaw=0.65f, pitch=0.35f, distance=12.f;
    glm::vec3 target{0,0,0};
    double lastChange=0;
    fs::path previewFile;
public:
    explicit Editor(GLFWwindow* w):window(w) {
        glGenTextures(1,&texture);
        previewFile=fs::temp_directory_path()/("seno-preview-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".seno");
    }
    ~Editor() {
        scene.reset(); glDeleteTextures(1,&texture);
        std::error_code ec; fs::remove(previewFile,ec);
    }
    bool done() const { return quit; }
    bool dirty() const { return doc.dirty() || draft!=doc.value || source!=sourceBase; }
    void sync() {
        draft=doc.value; source=draft.dump(4); sourceBase=source;
        previewStale=true; lastChange=glfwGetTime();
        if (!draft.contains(collection) || !draft[collection].is_array() || selected>=static_cast<int>(draft[collection].size())) selected=-1;
    }
    template<class F> bool safely(F f) {
        try { f(); return true; } catch(const std::exception& e) { message=e.what(); return false; }
    }
    bool open(const fs::path& path) {
        if(!safely([&]{doc.open(path);})) { std::cerr<<message<<'\n'; return false; }
        selected=-1; sync(); if(draft.contains(collection) && !draft[collection].empty())selected=0; message="Opened "+doc.path.string(); directory=doc.path.parent_path(); return true;
    }
    bool apply() {
        return safely([&] {
            if(source!=sourceBase) { doc.commit(Json::parse(source)); sync(); }
            else if(draft!=doc.value) { doc.commit(draft); sync(); }
        });
    }
    void perform(Action action) {
        pending=Action::None;
        if(action==Action::Quit) quit=true;
        if(action==Action::New) { doc=seno::Document{}; selected=-1; sync(); message="New scene"; }
        if(action==Action::Open) { showBrowser=true; saveMode=false; fileName.clear(); }
    }
    void request(Action action) {
        if(dirty()) {pending=action; askUnsaved=true;} else perform(action);
    }
    void saveAs() { showBrowser=true; saveMode=true; fileName=doc.path.empty()?"scene.seno":doc.path.filename().string(); }
    bool save(const fs::path& file, bool force=false) {
        if(!apply()) return false;
        if(!safely([&]{doc.save(file,force);})) return false;
        message="Saved "+doc.path.string(); showBrowser=false;
        if(pending!=Action::None) perform(pending);
        return true;
    }
    void saveCurrent() {
        if(doc.path.empty()) saveAs();
        else if(fs::exists(doc.path) && seno::read(doc.path)!=doc.disk) {
            overwritePath=doc.path; overwrite=true;
        } else save(doc.path);
    }
    void history(bool redo) {
        if(!apply()) return;
        if(redo) doc.redo(); else doc.undo();
        sync();
    }
    void add(const std::string& kind) {
        if(!apply()) return;
        Json next=doc.value;
        if(!next.contains(collection)) next[collection]=Json::array();
        Json item;
        if(collection=="objects") {
            if(next["models"].empty()) {message="Add a mesh, material and model first."; return;}
            item={{"position",{0,0,0}},{"pivot",{0,0,0}},{"scale",{1,1,1}},{"rotation",{1,0,0,0}},{"model",0},{"group","new"}};
        } else if(collection=="lights") {
            item={{"type",kind},{"color",{1,1,1}},{"intensity",1.0}};
            if(kind=="DirLight") item["rotation"]={1,0,0,0}; else item["position"]={0,3,0};
        } else if(collection=="materials") item={{"shader","pbrShader"},{"albedo",{0.7,0.7,0.7}},{"metallic",0.0},{"roughness",0.5},{"ao",1.0}};
        else if(collection=="meshes") item="{assetRoot}/assets/notbox.obj";
        else {
            if(next["meshes"].empty()||next["materials"].empty()) {message="Add a mesh and a material first."; return;}
            item={{"mesh",0},{"material",0}};
        }
        next[collection].push_back(item);
        if(safely([&]{doc.commit(next);})) {sync(); selected=static_cast<int>(draft[collection].size())-1;}
    }
    void duplicate() {
        if(selected<0 || !apply()) return;
        Json next=doc.value, item=next[collection][selected];
        if(collection=="objects") item.erase("id");
        next[collection].push_back(item);
        if(safely([&]{doc.commit(next);})) {sync();selected=static_cast<int>(draft[collection].size())-1;}
    }
    void remove() {
        if(selected<0 || !apply()) return;
        Json next=doc.value;
        // Shift references when removing resource entries. Reject referenced resources.
        const std::string key=collection=="models"?"model":collection=="materials"?"material":collection=="meshes"?"mesh":"";
        if(!key.empty()) {
            bool used=false;
            auto walk=[&](const auto& self,Json& v)->void {
                if(v.is_object()) for(auto it=v.begin();it!=v.end();++it) {
                    if(it.key()==key && it->is_number_integer()) {
                        int index=it->get<int>(); if(index==selected) used=true; else if(index>selected) *it=index-1;
                    } else self(self,it.value());
                } else if(v.is_array()) for(auto& x:v) self(self,x);
            };
            walk(walk,next);
            if(used) {message="Resource is still referenced. Reassign or remove its users first.";return;}
        }
        next[collection].erase(next[collection].begin()+selected);
        if(safely([&]{doc.commit(next);})) {selected=-1;sync();}
    }
    bool field(const std::string& label,Json& v,int depth=0) {
        ImGui::PushID(label.c_str()); bool changed=false;
        if(v.is_string()) {
            std::string text=v.get<std::string>();
            if(ImGui::InputText(label.c_str(),&text)) {v=text;changed=true;}
        } else if(v.is_boolean()) {
            bool b=v.get<bool>();if(ImGui::Checkbox(label.c_str(),&b)){v=b;changed=true;}
        } else if(v.is_number_integer()) {
            long long i=v.get<long long>();
            if(ImGui::InputScalar(label.c_str(),ImGuiDataType_S64,&i)){v=i;changed=true;}
        } else if(v.is_number()) {
            double x=v.get<double>();
            if(ImGui::InputDouble(label.c_str(),&x,0.01,0.1,"%.5f")){v=x;changed=true;}
        } else if(v.is_array() && label!="indices" && v.size()>=2 && v.size()<=4 && std::all_of(v.begin(),v.end(),[](const Json& x){return x.is_number();})) {
            float vec[4]={}; for(size_t i=0;i<v.size();++i)vec[i]=v[i].get<float>();
            const bool color=label=="albedo"||label=="color"||label=="ambient"||label=="diffuse"||label=="specular";
            if(color && v.size()==3) changed=ImGui::ColorEdit3(label.c_str(),vec,ImGuiColorEditFlags_Float|ImGuiColorEditFlags_HDR);
            else changed=ImGui::DragScalarN(label=="rotation"?"rotation (w,x,y,z)":label.c_str(),ImGuiDataType_Float,vec,static_cast<int>(v.size()),0.02f);
            if(changed)for(size_t i=0;i<v.size();++i)v[i]=vec[i];
        } else if(v.is_structured()) {
            if(depth<16 && ImGui::TreeNodeEx(label.c_str(),depth==0?ImGuiTreeNodeFlags_DefaultOpen:0,"%s (%zu)",label.c_str(),v.size())) {
                if(v.is_object()) for(auto it=v.begin();it!=v.end();++it) changed=field(it.key(),it.value(),depth+1)||changed;
                else {
                    // Large embedded meshes remain editable in the JSON tab.
                    size_t count=std::min<size_t>(v.size(),128);
                    for(size_t i=0;i<count;++i)changed=field(std::to_string(i),v[i],depth+1)||changed;
                    if(count<v.size())ImGui::TextDisabled("Remaining entries available in JSON source.");
                }
                ImGui::TreePop();
            }
        } else ImGui::TextDisabled("%s: null (edit in JSON)",label.c_str());
        ImGui::PopID(); return changed;
    }
    bool refreshPreview() {
        if(!apply()) return false;
        bool success=safely([&] {
            Json preview=doc.value;
            // Resolve relative assets against the source document, without rewriting saved paths.
            auto resolve=[&](Json& path) {
                if(!path.is_string()) return;
                std::string p=path.get<std::string>();
                if(p.find("{assetRoot}")==std::string::npos && fs::path(p).is_relative())
                    path=((doc.path.empty()?directory:doc.path.parent_path())/p).lexically_normal().string();
            };
            if(preview.contains("sky") && preview["sky"].contains("path"))resolve(preview["sky"]["path"]);
            if(preview.contains("meshes"))for(auto& m:preview["meshes"])resolve(m);
            if(preview.contains("models"))for(auto& m:preview["models"])if(m.contains("obj"))resolve(m["obj"]);
            std::ofstream out(previewFile); out<<preview.dump();out.close();
            seno::require(bool(out),"Cannot write preview snapshot");
            auto next=std::make_unique<knot::Scene>();
            seno::require(next->loadSeno(previewFile.string()),"Preview load failed. Check asset paths (details in terminal).");
            next->setCamera(camera); scene=std::move(next); message="Preview updated";
        });
        previewStale=false;
        if(!success)std::cerr<<message<<'\n';
        return success;
    }
    void viewport(ImVec2 size) {
        size.x=std::max(1.f,size.x); size.y=std::max(1.f,size.y);
        const auto& io=ImGui::GetIO();
        int fbw,fbh;glfwGetFramebufferSize(window,&fbw,&fbh);
        const int width=std::max(1,std::min(fbw,static_cast<int>(size.x*io.DisplayFramebufferScale.x)));
        const int height=std::max(1,std::min(fbh,static_cast<int>(size.y*io.DisplayFramebufferScale.y)));
        camera->position=target+distance*glm::vec3(std::cos(pitch)*std::sin(yaw),std::sin(pitch),std::cos(pitch)*std::cos(yaw));
        const glm::vec3 viewUp(-std::sin(pitch)*std::sin(yaw),std::cos(pitch),-std::sin(pitch)*std::cos(yaw));
        camera->rotation=glm::quatLookAt(glm::normalize(target-camera->position),viewUp);
        camera->farPlane=10000.f;
        camera->span=2.f*distance*std::tan(glm::radians(camera->fov)*.5f);
        glBindFramebuffer(GL_FRAMEBUFFER,0);glEnable(GL_DEPTH_TEST);glDepthMask(GL_TRUE);
        glClearColor(.055f,.065f,.085f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        auto& renderer=knot::Renderer::get();renderer.beginFrame(width,height);
        if(scene) renderer.renderScene(*scene,static_cast<float>(width)/height);
        glBindTexture(GL_TEXTURE_2D,texture);
        if(width!=texWidth || height!=texHeight) {
            texWidth=width;texHeight=height;
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        }
        glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,width,height);
        ImGui::Image(static_cast<ImTextureID>(texture),size,ImVec2(0,1),ImVec2(1,0));
        const bool hovered=ImGui::IsItemHovered();
        // A drag belongs to the viewport only if it started here; retain it at the edges.
        if(hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Middle))navigating=true;
        if(!ImGui::IsMouseDown(ImGuiMouseButton_Middle) || !glfwGetWindowAttrib(window,GLFW_FOCUSED))navigating=false;
        if(navigating) {
            if(io.KeyShift) {
                const glm::vec3 right(std::cos(yaw),0,-std::sin(yaw));
                const float unitsPerPixel=camera->span/size.y;
                target+=(-right*io.MouseDelta.x+viewUp*io.MouseDelta.y)*unitsPerPixel;
            } else if(io.KeyCtrl) {
                distance=std::clamp(distance*std::exp((io.MouseDelta.x+io.MouseDelta.y)*.01f),.1f,5000.f);
            } else {
                yaw-=io.MouseDelta.x*.008f;
                pitch=std::clamp(pitch+io.MouseDelta.y*.008f,-glm::half_pi<float>(),glm::half_pi<float>());
            }
        }
        if(hovered && !io.WantTextInput) {
            distance=std::clamp(distance*std::pow(.85f,io.MouseWheel),.1f,5000.f);
            if(ImGui::IsKeyPressed(ImGuiKey_KeypadAdd))distance=std::max(.1f,distance*.85f);
            if(ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract))distance=std::min(5000.f,distance/.85f);
            if(ImGui::IsKeyPressed(ImGuiKey_KeypadDecimal))safely([&]{focus();});
            if(ImGui::IsKeyPressed(ImGuiKey_Keypad5))camera->orthographic=!camera->orthographic;
            if(ImGui::IsKeyPressed(ImGuiKey_Keypad1)) {
                yaw=io.KeyCtrl?glm::pi<float>():0.f;pitch=0;camera->orthographic=true;
            }
            if(ImGui::IsKeyPressed(ImGuiKey_Keypad3)) {
                yaw=io.KeyCtrl?-glm::half_pi<float>():glm::half_pi<float>();pitch=0;camera->orthographic=true;
            }
            if(ImGui::IsKeyPressed(ImGuiKey_Keypad7)) {
                yaw=0;pitch=io.KeyCtrl?-glm::half_pi<float>():glm::half_pi<float>();camera->orthographic=true;
            }
            const float step=glm::radians(15.f);
            if(ImGui::IsKeyPressed(ImGuiKey_Keypad4))yaw-=step;
            if(ImGui::IsKeyPressed(ImGuiKey_Keypad6))yaw+=step;
            if(ImGui::IsKeyPressed(ImGuiKey_Keypad8))pitch=std::min(glm::half_pi<float>(),pitch+step);
            if(ImGui::IsKeyPressed(ImGuiKey_Keypad2))pitch=std::max(-glm::half_pi<float>(),pitch-step);
            if(ImGui::IsKeyPressed(ImGuiKey_Keypad9)) {yaw+=glm::pi<float>();pitch=-pitch;}
        }
    }
    void focus() {
        if(collection=="objects" && selected>=0) {
            const auto& obj=draft[collection][selected];
            if(obj.contains("position") && obj["position"].is_array() && obj["position"].size()==3)
                target={obj["position"][0].get<float>(),obj["position"][1].get<float>(),obj["position"][2].get<float>()};
        } else target={0,0,0};
        distance=8;
    }
    void browser() {
        if(showBrowser) {ImGui::OpenPopup("File browser");showBrowser=false;}
        ImGui::SetNextWindowSize(ImVec2(660,460),ImGuiCond_Appearing);
        if(ImGui::BeginPopupModal("File browser",nullptr,ImGuiWindowFlags_NoResize)) {
            ImGui::TextUnformatted(saveMode?"Save Seno scene":"Open Seno scene");
            std::string dir=directory.string();
            if(ImGui::InputText("Directory",&dir,ImGuiInputTextFlags_EnterReturnsTrue)) safely([&]{seno::require(fs::is_directory(dir),"Not a directory");directory=fs::absolute(dir);});
            if(ImGui::Button("Up"))directory=directory.parent_path();
            ImGui::BeginChild("files",ImVec2(0,260),ImGuiChildFlags_Borders);
            std::vector<fs::directory_entry> entries; std::error_code ec;
            fs::directory_iterator it(directory,fs::directory_options::skip_permission_denied,ec),end;
            while(!ec && it!=end) {entries.push_back(*it);it.increment(ec);}
            if(ec)ImGui::TextWrapped("%s",ec.message().c_str());
            std::sort(entries.begin(),entries.end(),[](const auto& a,const auto& b){return a.path().filename()<b.path().filename();});
            fs::path nextDir;
            for(const auto& entry:entries) {
                bool isDir=entry.is_directory(ec);if(ec)continue;
                if(!isDir && entry.path().extension()!=".seno" && entry.path().extension()!=".json")continue;
                std::string label=(isDir?"[dir] ":"")+entry.path().filename().string();
                if(ImGui::Selectable(label.c_str(),fileName==entry.path().filename().string(),ImGuiSelectableFlags_DontClosePopups)) {
                    if(isDir)nextDir=entry.path();else fileName=entry.path().filename().string();
                }
            }
            if(!nextDir.empty())directory=nextDir;
            ImGui::EndChild();ImGui::InputText("File name / path",&fileName);
            if(ImGui::Button(saveMode?"Save":"Open") && !fileName.empty()) {
                fs::path file=directory/fs::path(fileName);
                if(saveMode) {
                    if(file.extension().empty())file+=".seno";
                    if(fs::exists(file)) {overwritePath=file;overwrite=true;ImGui::CloseCurrentPopup();}
                    else if(save(file))ImGui::CloseCurrentPopup();
                } else if(safely([&]{doc.open(file);})) {sync();selected=-1;message="Opened "+file.string();ImGui::CloseCurrentPopup();}
            }
            ImGui::SameLine();if(ImGui::Button("Cancel")){pending=Action::None;ImGui::CloseCurrentPopup();}
            ImGui::TextWrapped("%s",message.c_str());ImGui::EndPopup();
        }
        if(overwrite){ImGui::OpenPopup("Overwrite file?");overwrite=false;}
        if(ImGui::BeginPopupModal("Overwrite file?",nullptr,ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted("This file exists or changed on disk. Replace its contents?");ImGui::TextUnformatted(overwritePath.string().c_str());
            if(ImGui::Button("Overwrite")){if(save(overwritePath,true))ImGui::CloseCurrentPopup();}
            ImGui::SameLine();if(ImGui::Button("Cancel")){pending=Action::None;ImGui::CloseCurrentPopup();}
            ImGui::TextWrapped("%s",message.c_str());ImGui::EndPopup();
        }
    }
    void frame() {
        if(glfwWindowShouldClose(window)){glfwSetWindowShouldClose(window,GLFW_FALSE);request(Action::Quit);}
        const auto& io=ImGui::GetIO();
        if(!ImGui::IsPopupOpen("",ImGuiPopupFlags_AnyPopupId)) {
            if(io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N))request(Action::New);
            if(io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O))request(Action::Open);
            if(io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)){if(io.KeyShift)saveAs();else safely([&]{saveCurrent();});}
            if(!io.WantTextInput && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z))history(io.KeyShift);
            if(!io.WantTextInput && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y))history(true);
            if(!io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F))safely([&]{focus();});
        }
        ImGui::SetNextWindowPos(ImVec2(0,0));ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Seno",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_MenuBar);
        if(ImGui::BeginMenuBar()) {
            if(ImGui::BeginMenu("File")) {
                if(ImGui::MenuItem("New","Ctrl+N"))request(Action::New);
                if(ImGui::MenuItem("Open...","Ctrl+O"))request(Action::Open);
                if(ImGui::MenuItem("Save","Ctrl+S"))safely([&]{saveCurrent();});
                if(ImGui::MenuItem("Save as...","Ctrl+Shift+S"))saveAs();
                ImGui::Separator();if(ImGui::MenuItem("Quit"))request(Action::Quit);ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("Edit")) {
                if(ImGui::MenuItem("Undo","Ctrl+Z",false,!doc.undoStack.empty()))history(false);
                if(ImGui::MenuItem("Redo","Ctrl+Y",false,!doc.redoStack.empty()))history(true);
                ImGui::EndMenu();
            }
            ImGui::TextDisabled("  SENO / SCENE EDITOR");ImGui::EndMenuBar();
        }
        ImGui::Text("%s%s",doc.path.empty()?"Untitled.seno":doc.path.filename().string().c_str(),dirty()?"  *":"");
        ImGui::SameLine();if(ImGui::Button("Refresh preview"))refreshPreview();
        ImGui::SameLine();ImGui::Checkbox("Auto",&autoPreview);
        ImGui::SameLine();if(ImGui::Button("Focus [F]"))safely([&]{focus();});
        ImGui::Separator();
        float contentHeight=std::max(100.f,ImGui::GetContentRegionAvail().y-45);
        if(ImGui::BeginTable("layout",3,ImGuiTableFlags_Resizable|ImGuiTableFlags_BordersInnerV)) {
            ImGui::TableSetupColumn("Hierarchy",ImGuiTableColumnFlags_WidthFixed,220);
            ImGui::TableSetupColumn("Viewport",ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Inspector",ImGuiTableColumnFlags_WidthFixed,350);
            ImGui::TableNextRow();ImGui::TableNextColumn();
            ImGui::BeginChild("hierarchy",ImVec2(0,contentHeight));
            ImGui::TextDisabled("SCENE CONTENTS");
            if(ImGui::BeginCombo("##collection",collection.c_str())) {
                for(auto key:{"objects","lights","materials","models","meshes"})if(ImGui::Selectable(key,collection==key)){collection=key;selected=-1;}
                ImGui::EndCombo();
            }
            ImGui::InputTextWithHint("##search","Filter...",&filter);
            if(ImGui::Button("Add"))add(collection=="lights"?"PbrPointLight":"");
            if(collection=="lights"){ImGui::SameLine();if(ImGui::Button("+ Sun"))add("DirLight");}
            ImGui::BeginDisabled(selected<0);
            if(ImGui::Button("Duplicate"))duplicate();ImGui::SameLine();if(ImGui::Button("Delete"))remove();
            ImGui::EndDisabled();ImGui::Separator();
            if(draft.contains(collection) && draft[collection].is_array()) {
                auto& entries=draft[collection];
                for(size_t i=0;i<entries.size();++i) {
                    auto& v=entries[i];std::string label=std::to_string(i)+"  ";
                    if(v.is_string())label+=fs::path(v.get<std::string>()).filename().string();
                    else if(v.is_object()) {
                        if(v.contains("group") && v["group"].is_string())label+=v["group"].get<std::string>();
                        else if(v.contains("type") && v["type"].is_string())label+=v["type"].get<std::string>();
                        else label+=collection;
                    }
                    if(!filter.empty() && label.find(filter)==std::string::npos)continue;
                    if(ImGui::Selectable(label.c_str(),selected==static_cast<int>(i)))selected=static_cast<int>(i);
                }
            }
            ImGui::EndChild();ImGui::TableNextColumn();
            ImGui::BeginChild("center",ImVec2(0,contentHeight),0,ImGuiWindowFlags_NoScrollWithMouse);
            if(ImGui::BeginTabBar("view")) {
                if(ImGui::BeginTabItem("3D Preview")) {
                        ImGui::TextDisabled("MMB orbit / Shift+MMB pan / Wheel zoom");
                    ImGui::TextDisabled("%s | Numpad 1/3/7 view, 5 projection, . focus",camera->orthographic?"Orthographic":"Perspective");
                    if(autoPreview && previewStale && !ImGui::IsAnyItemActive() && glfwGetTime()-lastChange>.4)refreshPreview();
                    viewport(ImGui::GetContentRegionAvail());ImGui::EndTabItem();
                }
                if(ImGui::BeginTabItem("JSON Source")) {
                    if(ImGui::Button("Apply JSON")) {if(apply())message="JSON validated and applied";}
                    ImGui::SameLine();if(ImGui::Button("Discard source edits")){source=doc.value.dump(4);sourceBase=source;}
                    ImGui::InputTextMultiline("##json",&source,ImGui::GetContentRegionAvail(),ImGuiInputTextFlags_AllowTabInput);
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::EndChild();ImGui::TableNextColumn();
            ImGui::BeginChild("inspector",ImVec2(0,contentHeight));
            ImGui::TextDisabled("INSPECTOR");
            ImGui::BeginDisabled(source!=sourceBase);
            if(selected>=0 && draft.contains(collection) && selected<static_cast<int>(draft[collection].size())) {
                ImGui::Text("%s [%d]",collection.c_str(),selected);ImGui::Separator();
                if(collection=="objects" && !draft[collection][selected].contains("pivot")) {
                    if(ImGui::Button("Add pivot (0, 0, 0)"))draft[collection][selected]["pivot"]={0,0,0};
                }
                ImGui::PushItemWidth(-120);field("Properties",draft[collection][selected]);ImGui::PopItemWidth();
            } else ImGui::TextWrapped("Select an entry to edit. Add meshes and materials before models, then add objects.");
            ImGui::Separator();
            if(ImGui::CollapsingHeader("Scene settings")) {
                for(auto key:{"sky","camera"}) {
                    if(draft.contains(key))field(key,draft[key]);
                    else {std::string label=std::string("Add ")+key;if(ImGui::Button(label.c_str())) {
                        if(std::string(key)=="sky")draft[key]={{"type","HDR"},{"path","{assetRoot}/assets/DaySkyHDRI015A_2K_HDR.hdr"}};
                        else draft[key]={{"type","PerspectiveCamera"},{"position",{0,2,5}},{"rotation",{1,0,0,0}},{"near",0.1},{"far",1000.0},{"fov",45.0}};
                    }}
                }
                ImGui::TextWrapped("The preview uses a separate orbit camera. Scene camera values are preserved in the file.");
            }
            ImGui::EndDisabled();
            if(draft!=doc.value) {
                ImGui::TextWrapped("Pending property edits");
                if(ImGui::Button("Apply properties"))apply();ImGui::SameLine();
                if(ImGui::Button("Revert properties"))draft=doc.value;
                if(!ImGui::IsAnyItemActive())apply();
            }
            ImGui::EndChild();ImGui::EndTable();
        }
        ImGui::Separator();ImGui::TextWrapped("%s",message.c_str());
        if(askUnsaved){ImGui::OpenPopup("Unsaved changes");askUnsaved=false;}
        if(ImGui::BeginPopupModal("Unsaved changes",nullptr,ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted("Save your changes before continuing?");
            if(ImGui::Button("Save")){ImGui::CloseCurrentPopup();safely([&]{saveCurrent();});}
            ImGui::SameLine();if(ImGui::Button("Discard")){ImGui::CloseCurrentPopup();perform(pending);}
            ImGui::SameLine();if(ImGui::Button("Cancel")){pending=Action::None;ImGui::CloseCurrentPopup();}
            ImGui::EndPopup();
        }
        browser();ImGui::End();
        std::string title=std::string(dirty()?"* ":"")+(doc.path.empty()?"Untitled.seno":doc.path.filename().string())+" - Seno Editor";
        glfwSetWindowTitle(window,title.c_str());
    }
};
}
int main(int argc,char** argv) {
    glfwSetErrorCallback([](int,const char* text){std::cerr<<text<<'\n';});
    if(!glfwInit())return 1;
    // Knot's shaders and point-light SSBOs require OpenGL 4.3.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    bool smoke=argc>1 && std::string(argv[1])=="--smoke-test";
    if(smoke)glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
    auto* window=glfwCreateWindow(1440,900,"Seno Editor",nullptr,nullptr);
    if(!window){glfwTerminate();return 1;}
    glfwSetWindowSizeLimits(window,960,600,GLFW_DONT_CARE,GLFW_DONT_CARE);
    glfwMakeContextCurrent(window);glfwSwapInterval(1);
    knot::setAssetRoot(fs::absolute(SENO_ASSET_ROOT).lexically_normal().string());
    auto& renderer=knot::Renderer::get();
    if(!renderer.init(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress))) {glfwDestroyWindow(window);glfwTerminate();return 1;}
    IMGUI_CHECKVERSION();ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::GetIO().IniFilename=nullptr;
    // Keep Korean scene names and paths readable when a system font is available.
    const char* customFont=std::getenv("SENO_FONT");
    for(const char* font : {customFont, "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc", "C:/Windows/Fonts/malgun.ttf"}) {
        if(font && fs::exists(font)) {
            ImGui::GetIO().Fonts->AddFontFromFileTTF(font,16.f,nullptr,ImGui::GetIO().Fonts->GetGlyphRangesKorean());
            break;
        }
    }
    ImGui::StyleColorsDark();auto& style=ImGui::GetStyle();style.WindowRounding=0;style.FrameRounding=4;style.FramePadding=ImVec2(7,5);
    style.Colors[ImGuiCol_CheckMark]=ImVec4(.35f,.8f,.7f,1);style.Colors[ImGuiCol_Header]=ImVec4(.16f,.35f,.34f,1);
    ImGui_ImplGlfw_InitForOpenGL(window,true);ImGui_ImplOpenGL3_Init("#version 430");
    int result=0;
    try {
        Editor editor(window);
        if(argc>1 && !smoke)editor.open(argv[1]);
        if(smoke && argc>2 && !editor.open(argv[2])) throw std::runtime_error("Smoke test: failed to open scene");
        if(smoke && !editor.refreshPreview()) throw std::runtime_error("Smoke test: preview failed");
        int frames=0;
        while(!editor.done()) {
            glfwPollEvents();int width,height;glfwGetFramebufferSize(window,&width,&height);
            if(width==0||height==0){glfwWaitEventsTimeout(.1);continue;}
            ImGui_ImplOpenGL3_NewFrame();ImGui_ImplGlfw_NewFrame();ImGui::NewFrame();
            editor.frame();ImGui::Render();
            glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,width,height);
            glClearColor(.055f,.065f,.085f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            if(smoke && frames==4 && argc>3) {
                std::vector<unsigned char> pixels(width*height*3);
                glPixelStorei(GL_PACK_ALIGNMENT,1);glReadBuffer(GL_BACK);
                glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
                std::ofstream shot(argv[3],std::ios::binary);shot<<"P6\n"<<width<<" "<<height<<"\n255\n";
                for(int row=height-1;row>=0;--row)shot.write(reinterpret_cast<char*>(pixels.data()+row*width*3),width*3);
            }
            glfwSwapBuffers(window);
            if(smoke && ++frames>=5)break;
        }
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';result=1;}
    ImGui_ImplOpenGL3_Shutdown();ImGui_ImplGlfw_Shutdown();ImGui::DestroyContext();
    renderer.shutdown();glfwDestroyWindow(window);glfwTerminate();return result;
}
