// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <knot/engine.h>
#include <knot/scene.h>
#include <knot/resources.h>
#include <knot/utility.h>

int main() {
    knot::Engine engine;

    if (!engine.init(1280, 720, "Knot Orthographic Camera Demo")) {
        return 1;
    }

    engine.setClearColor(0.12f, 0.14f, 0.18f, 1.0f);

    knot::Scene scene;

    scene.loadHDRMap(
        knot::getAssetRoot() + "assets/DaySkyHDRI015A_2K_HDR.hdr"
    );

    auto shader = scene.getResourceManager().getShader("pbrShader");

    auto model = knot::loadModelOBJWithMTL(
        knot::getAssetRoot() + "assets/Untitled.obj",
        shader
    );

    auto cubeObject = std::make_shared<knot::Object>(model.front());

    cubeObject->position = glm::vec3(0.0f);
    cubeObject->scale = glm::vec3(0.5f);

    scene.getObjectManager().registerObject(cubeObject);

    auto dirLight = std::make_shared<knot::DirLight>();

    dirLight->rotation = glm::quat(glm::vec3(
        glm::radians(45.0f),
        glm::radians(-20.0f),
        0.0f
    ));

    dirLight->ambient = glm::vec3(0.1f);
    dirLight->diffuse = glm::vec3(1.0f);
    dirLight->specular = glm::vec3(1.0f);

    scene.getLightManager().registerLight(dirLight);

    auto pointLight = std::make_shared<knot::PbrPointLight>(
        glm::vec3(1.5f, 1.5f, 2.0f),
        glm::vec3(1.0f),
        2.0f
    );

    scene.getLightManager().registerLight(pointLight);

    auto camera = std::make_shared<knot::OrthographicCamera>(
        glm::vec3(0.0f, 0.0f, 5.0f)
    );

    camera->rotation = glm::quat(glm::vec3(
        0.0f,
        0.0f,
        0.0f
    ));

    camera->size = 5.0f;

    scene.setCamera(camera);

    float totalTime = 0.0f;

    scene.setUpdateCallback([&](knot::Scene& currentScene, float deltaTime) {
        totalTime += deltaTime;

        cubeObject->rotation = glm::quat(glm::vec3(
            sin(totalTime * 0.5f) * 0.2f,
            totalTime * 0.5f,
            0.0f
        ));
    });

    engine.setScene(scene);

    return engine.run();
}