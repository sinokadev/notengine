#include "test_support.h"
#include <knot/manager.h>

namespace {
void objectIdsAndLookup() {
    knot::ObjectManager manager;
    CHECK(manager.registerObject(nullptr) == 0);
    CHECK(manager.registerObject(nullptr, "group") == 0);
    CHECK(manager.getObjects().empty());
    auto first = std::make_shared<knot::Object>();
    auto second = std::make_shared<knot::Object>();
    auto third = std::make_shared<knot::Object>();
    CHECK(manager.registerObject(first, 1) == 1);
    CHECK(manager.registerObject(second) == 2);
    const auto reassigned = manager.registerObject(third, 1);
    CHECK(reassigned != 1 && reassigned != 2 && reassigned != 0);
    CHECK(third->id == reassigned);
    CHECK(manager.getObject(1) == first.get());
    const auto& constManager = manager;
    CHECK(constManager.getObject(2) == second.get());
    CHECK(constManager.getObject(999) == nullptr);
    CHECK(manager.getObject(999) == nullptr);
    auto iterator = manager.getObjects().begin();
    CHECK(*iterator++ == first);
    CHECK(*iterator++ == second);
    CHECK(*iterator == third);
}

void objectGroups() {
    knot::Object object;
    object.addGroup("");
    CHECK(object.getGroups().empty());
    object.addGroup("enemy");
    object.addGroup("visible");
    object.addGroup("enemy");
    CHECK(object.getGroup() == "enemy");
    CHECK(object.getGroups().size() == 2);
    object.removeGroup("missing");
    CHECK(object.getGroups().size() == 2);
    object.removeGroup("enemy");
    CHECK(object.getGroup() == "visible");
    CHECK(!object.isInGroup("enemy"));
    object.setGroup("player");
    CHECK(object.getGroups() == std::vector<std::string>{"player"});
    CHECK(!object.isInGroup("visible"));
    object.setGroup("");
    CHECK(object.getGroups().empty());
    CHECK(object.getGroup().empty());
    CHECK(!object.isInGroup(""));
}

void groupQueriesAndUpdates() {
    knot::ObjectManager manager;
    auto first = std::make_shared<knot::Object>();
    auto second = std::make_shared<knot::Object>();
    auto firstId = manager.registerObject(first, "enemy");
    auto secondId = manager.registerObject(second, "enemy");
    CHECK(manager.addToGroup("visible", firstId));
    CHECK(manager.addToGroup("visible", firstId));
    CHECK(manager.getGroup("enemy") == std::vector<knot::Object*>({first.get(), second.get()}));
    CHECK(manager.getObjects("visible") == std::vector<knot::Object*>{first.get()});
    CHECK(manager.getSharedGroup("visible") == std::vector<std::shared_ptr<knot::Object>>{first});
    CHECK(manager.getGroups() == std::vector<std::string>({"enemy", "visible"}));
    CHECK(manager.getGroup("").empty());
    CHECK(manager.getSharedGroup("").empty());
    CHECK(manager.getGroup("missing").empty());
    CHECK(!manager.addToGroup("enemy", 999));
    CHECK(!manager.setObjectGroup(999, "enemy"));
    CHECK(manager.setObjectGroup(firstId, "player"));
    CHECK(manager.getGroup("visible").empty());
    CHECK(manager.getGroup("enemy") == std::vector<knot::Object*>{second.get()});
    CHECK(manager.removeObject(secondId));
    CHECK(manager.getGroup("enemy").empty());
    CHECK(manager.getGroups() == std::vector<std::string>{"player"});
}

void objectRemovalAndLifetime() {
    knot::ObjectManager manager;
    auto object = std::make_shared<knot::Object>();
    std::weak_ptr<knot::Object> lifetime = object;
    auto id = manager.registerObject(object);
    object.reset();
    CHECK(!lifetime.expired());
    CHECK(manager.removeObject(id));
    CHECK(lifetime.expired());
    CHECK(!manager.removeObject(id));
    CHECK(manager.getObject(id) == nullptr);
    CHECK(manager.getObjects().empty());
    manager.registerObject(std::make_shared<knot::Object>(), "group");
    manager.clear();
    CHECK(manager.getGroups().empty());
    CHECK(manager.getObjects().empty());
    CHECK(manager.registerObject(std::make_shared<knot::Object>()) == 1);
    manager.shutdown();
    CHECK(manager.getObject(1) == nullptr);
    CHECK(manager.registerObject(std::make_shared<knot::Object>()) == 1);
}

void lightTypesAndIds() {
    knot::LightManager manager;
    CHECK(manager.registerLight(nullptr) == 0);
    auto directional = std::make_shared<knot::DirLight>(glm::vec3(0, 0, -1));
    auto point = std::make_shared<knot::PbrPointLight>();
    auto generic = std::make_shared<knot::Light>();
    generic->id = 42;
    CHECK(manager.registerLight(directional) == 1);
    CHECK(manager.registerLight(point) == 2);
    CHECK(manager.registerLight(generic) == 42);
    CHECK(manager.getLight(1) == directional.get());
    CHECK(manager.getLight(42) == generic.get());
    CHECK(manager.getLight(999) == nullptr);
    CHECK(manager.getDirLights() == std::vector<const knot::DirLight*>{directional.get()});
    CHECK(manager.getPointLights() == std::vector<const knot::PbrPointLight*>{point.get()});
    CHECK(manager.getLights().size() == 3);
    CHECK(manager.removeLight(1));
    CHECK(manager.getDirLights().empty());
    CHECK(manager.getPointLights().size() == 1);
    CHECK(!manager.removeLight(1));
}

void lightRemovalAndReset() {
    knot::LightManager manager;
    auto light = std::make_shared<knot::PbrPointLight>();
    std::weak_ptr<knot::Light> lifetime = light;
    manager.registerLight(light);
    light.reset();
    CHECK(!lifetime.expired());
    manager.clear();
    CHECK(lifetime.expired());
    CHECK(manager.getLights().empty());
    CHECK(manager.getLight(1) == nullptr);
    CHECK(manager.getPointLights().empty());
    CHECK(manager.registerLight(std::make_shared<knot::Light>()) == 1);
    manager.shutdown();
    CHECK(manager.getLights().empty());
    CHECK(manager.registerLight(std::make_shared<knot::Light>()) == 1);
}
} // namespace

int main() {
    return runTests({{"object IDs and lookup", objectIdsAndLookup}, {"object groups", objectGroups},
                     {"group queries and updates", groupQueriesAndUpdates}, {"object removal and lifetime", objectRemovalAndLifetime},
                     {"light types and IDs", lightTypesAndIds}, {"light removal and reset", lightRemovalAndReset}});
}
