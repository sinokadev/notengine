#include "../document.h"
#include <iostream>
using namespace seno;
void check(bool v,const char* message){if(!v)throw std::runtime_error(message);}
template<class F> void fails(F f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}check(threw,"Expected rejection");}
int main() {
    auto dir=fs::temp_directory_path()/("seno-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directory(dir);
    try {
        Document d;
        auto j=emptyScene(); j["extension"]={{"custom",42}};
        j["meshes"].push_back("mesh.obj");j["materials"].push_back({{"shader","pbrShader"}});
        j["models"].push_back({{"mesh",0},{"material",0}});
        j["objects"].push_back({{"id",100},{"model",0},{"position",{0,1,2}}});
        d.commit(j);check(d.dirty(),"Commit dirty"); d.undo();check(d.value==emptyScene(),"Undo");d.redo();check(d.value==j,"Redo");
        auto file=dir/"scene.seno";d.save(file);check(!d.dirty(),"Save clears dirty");
        Document loaded;loaded.open(file);check(loaded.value==j,"Lossless round trip");
        auto bad=j;bad["objects"][0]["model"]=2;fails([&]{d.commit(bad);});check(d.value==j,"Failed commit is transactional");
        bad=j;bad["objects"].push_back(bad["objects"][0]);fails([&]{validate(bad);});
        bad=j;bad["objects"][0]["rotation"]={0,0,0,0};fails([&]{validate(bad);});
        bad=j;bad["objects"][0]["position"]={1,"x",0};fails([&]{validate(bad);});
        bad=j;bad["camera"]={{"near",2},{"far",1}};fails([&]{validate(bad);});
        {std::ofstream out(file);out<<"invalid";}
        fails([&]{d.save(file);});check(read(file)=="invalid","External change preserved");
        fails([&]{loaded.open(file);});check(loaded.value==j,"Failed open preserved document");
        d.save(file,true);check(Json::parse(read(file))==j,"Confirmed overwrite");
        fails([&]{Document{}.save(file);});
        fails([&]{d.save(dir/"missing"/"scene.seno");});check(Json::parse(read(file))==j,"Failed save preserved file");
        d.commit(emptyScene());d.undo();check(!d.dirty(),"Undo to saved clears dirty");
        d.commit(emptyScene());check(d.redoStack.empty(),"New edit drops redo");
        fs::remove_all(dir);std::cout<<"All document tests passed\n";return 0;
    }catch(const std::exception& e){fs::remove_all(dir);std::cerr<<e.what()<<'\n';return 1;}
}
