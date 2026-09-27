// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#include <iostream>
#include <cassert>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <knot/resources.h>
#include <knot/manager.h>
#include <knot/scene.h>
#include <knot/utility.h>

int main() {
    std::cout << "[INFO] Starting Object Group tests..." << std::endl;

    // 1. Basic Object Group API tests
    {
        auto obj = std::make_shared<knot::Object>();
        assert(obj->group.empty());
        assert(obj->getGroup().empty());
        assert(!obj->isInGroup("enemy"));

        obj->setGroup("enemy");
        assert(obj->group == "enemy");
        assert(obj->getGroup() == "enemy");
        assert(obj->isInGroup("enemy"));
        assert(!obj->isInGroup("ally"));

        // Direct member assignment
        obj->group = "boss";
        assert(obj->getGroup() == "boss");
        assert(obj->isInGroup("boss"));

        // Multi-group support
        obj->addGroup("flying");
        assert(obj->isInGroup("boss"));
        assert(obj->isInGroup("flying"));
        assert(!obj->isInGroup("ground"));

        obj->removeGroup("boss");
        assert(!obj->isInGroup("boss"));
        assert(obj->isInGroup("flying"));

        std::cout << "[PASS] Object group API tests passed." << std::endl;
    }

    // 2. ObjectManager Group API tests
    {
        knot::ObjectManager objManager;

        auto obj1 = std::make_shared<knot::Object>();
        obj1->setGroup("heroes");
        auto id1 = objManager.registerObject(obj1);

        auto obj2 = std::make_shared<knot::Object>();
        auto id2 = objManager.registerObject(obj2, "heroes");

        auto obj3 = std::make_shared<knot::Object>();
        auto id3 = objManager.registerObject(obj3, "monsters");

        auto obj4 = std::make_shared<knot::Object>();
        auto id4 = objManager.registerObject(obj4); // unassigned initially

        assert(obj1->id == id1);
        assert(obj2->id == id2);
        assert(obj3->id == id3);
        assert(obj4->id == id4);

        // Test getObjectsByGroup
        auto heroes = objManager.getObjectsByGroup("heroes");
        assert(heroes.size() == 2);
        assert(heroes[0] == obj1.get() || heroes[0] == obj2.get());
        assert(heroes[1] == obj1.get() || heroes[1] == obj2.get());

        // Test getSharedObjectsByGroup
        auto sharedHeroes = objManager.getSharedObjectsByGroup("heroes");
        assert(sharedHeroes.size() == 2);
        assert(sharedHeroes[0] == obj1 || sharedHeroes[0] == obj2);
        assert(sharedHeroes[1] == obj1 || sharedHeroes[1] == obj2);

        // Test aliases: getGroup, getObjects, getSharedGroup
        auto heroesAlias = objManager.getGroup("heroes");
        assert(heroesAlias.size() == 2);
        auto heroesOverload = objManager.getObjects("heroes");
        assert(heroesOverload.size() == 2);
        auto sharedHeroesAlias = objManager.getSharedGroup("heroes");
        assert(sharedHeroesAlias.size() == 2);

        // Test monsters group
        auto monsters = objManager.getObjectsByGroup("monsters");
        assert(monsters.size() == 1);
        assert(monsters[0] == obj3.get());

        // Test non-existent group
        auto emptyGroup = objManager.getObjectsByGroup("nonexistent");
        assert(emptyGroup.empty());

        // Test setObjectGroup on obj4
        bool setOk = objManager.setObjectGroup(id4, "monsters");
        assert(setOk);
        monsters = objManager.getObjectsByGroup("monsters");
        assert(monsters.size() == 2);

        // Test addToGroup on obj1
        bool addOk = objManager.addToGroup("flying", id1);
        assert(addOk);
        auto flying = objManager.getObjectsByGroup("flying");
        assert(flying.size() == 1);
        assert(flying[0] == obj1.get());

        // Test getGroups
        auto groups = objManager.getGroups();
        assert(groups.size() >= 3); // heroes, monsters, flying

        // Test mutation through returned pointers
        for (auto* hero : objManager.getObjectsByGroup("heroes")) {
            hero->position.y += 10.0f;
        }
        assert(obj1->position.y == 10.0f);
        assert(obj2->position.y == 10.0f);

        // Test removeObject updates group queries
        bool remOk = objManager.removeObject(id2);
        assert(remOk);
        heroes = objManager.getObjectsByGroup("heroes");
        assert(heroes.size() == 1);
        assert(heroes[0] == obj1.get());

        std::cout << "[PASS] ObjectManager group API tests passed." << std::endl;
    }

    // 3. Scene loading with group specification
    {
        if (!glfwInit()) {
            std::cerr << "Failed to init GLFW\n";
            return 1;
        }

        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow* window = glfwCreateWindow(640, 480, "Group Test", nullptr, nullptr);
        if (!window) {
            std::cerr << "Failed to create window\n";
            glfwTerminate();
            return 1;
        }

        glfwMakeContextCurrent(window);

        if (!gladLoadGL(glfwGetProcAddress)) {
            std::cerr << "Failed to init GLAD\n";
            glfwDestroyWindow(window);
            glfwTerminate();
            return 1;
        }

        knot::Scene scene;
        const std::string scenePath = knot::getAssetRoot() + "assets/scene.seno";
        bool loaded = scene.loadSeno(scenePath);
        assert(loaded);

        auto& om = scene.getObjectManager();

        // In assets/scene.seno, we set:
        // Object 100: "boxes"
        // Object 2: "boxes"
        // Object 3: "ground"
        auto boxes = om.getObjectsByGroup("boxes");
        assert(boxes.size() == 2);

        auto ground = om.getObjectsByGroup("ground");
        assert(ground.size() == 1);

        // Check ID 100 is in boxes
        auto* obj100 = om.getObject(100);
        assert(obj100 != nullptr);
        assert(obj100->group == "boxes");
        assert(obj100->isInGroup("boxes"));

        // Check ground object properties
        assert(ground[0]->group == "ground");
        assert(ground[0]->scale.x == 200.0f);

        glfwDestroyWindow(window);
        glfwTerminate();

        std::cout << "[PASS] Scene loadSeno with group tests passed." << std::endl;
    }

    std::cout << "[SUCCESS] All Object Group tests passed successfully!" << std::endl;
    return 0;
}
