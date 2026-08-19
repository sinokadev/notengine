// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <limits>
#include <cmath>

#include <knot/engine.h>
#include <knot/scene.h>
#include <knot/resources.h>
#include <knot/utility.h>

#include <GLFW/glfw3.h>

enum UserEventType : uint32_t { BENCHMARK_NEXT };

struct BenchmarkResult {
    int objectCount;
    double averageFPS;
    double minFPS;
};

int main() {
    knot::Engine engine;

    if (!engine.init(1280, 720, "Knot Engine Benchmark")) {
        return 1;
    }

    engine.getWindow().disableVsync();

    engine.setClearColor(0.12f, 0.14f, 0.18f, 1.0f);

    knot::Scene scene;

    scene.loadHDRMap(knot::getAssetRoot() + "assets/DaySkyHDRI015A_2K_HDR.hdr");

    auto mesh = knot::loadModelOBJ(knot::getAssetRoot() + "assets/notbox.obj");

    auto shader = scene.getResourceManager().getShader("pbrShader");

    auto material = std::make_shared<knot::PbrMaterial>(shader, glm::vec3(1.0f), 0.2f, 0.0f, 1.0f);

    // 모든 Object가 하나의 Model을 공유한다.
    auto model = std::make_shared<knot::Model>(mesh, material);

    auto dirLightObj = std::make_shared<knot::DirLight>(glm::vec3(-0.2f, -1.0f, -0.3f), glm::vec3(0.1f), glm::vec3(1.0f), glm::vec3(1.0f));

    scene.getLightManager().registerLight(dirLightObj);

    auto pointLightObj = std::make_shared<knot::PbrPointLight>(glm::vec3(1.0f), glm::vec3(1.0f), 2.0f);

    scene.getLightManager().registerLight(pointLightObj);

    knot::Camera camera(glm::vec3(0.0f, 20.0f, 0.0f));
    camera.lookAtTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    scene.setCamera(camera);

    // 테스트할 Object 개수
    const std::vector<int> objectCounts = {1, 500, 1'000, 5'000, 10'000, 25'000, 50'000, 100'000, 250'000, 500'000, 1'000'000};
    std::vector<BenchmarkResult> results;

    // 현재 테스트에 생성된 Object들의 ID
    std::vector<unsigned int> objectIds;

    constexpr double WARMUP_TIME = 2.0;
    constexpr double TEST_TIME = 5.0;

    int benchmarkIndex = 0;

    double elapsedTime = 0.0;
    double measuredTime = 0.0;

    uint64_t frameCount = 0;

    bool measuring = false;
    bool finished = false;

    auto removeObjects = [&]() {
        for (unsigned int id : objectIds) {
            scene.getObjectManager().removeObject(id);
        }

        objectIds.clear();
    };

    auto createObjects = [&](int count) {
        objectIds.clear();
        objectIds.reserve(count);

        constexpr float SPACING = 1.0f;

        // 한 변에 배치할 Object 개수
        const int gridSize = static_cast<int>(std::ceil(std::sqrt(count)));

        // Grid의 중앙을 (0, 0, 0)에 맞춘다.
        const float halfGrid = (gridSize - 1) * SPACING * 0.5f;

        for (int i = 0; i < count; ++i) {
            const int x = i % gridSize;
            const int z = i / gridSize;

            auto object = std::make_shared<knot::Object>(model);

            object->position = glm::vec3(x * SPACING - halfGrid, 0.0f, z * SPACING - halfGrid);

            object->scale = glm::vec3(0.5f);

            unsigned int id = scene.getObjectManager().registerObject(object);

            objectIds.push_back(id);
        }
    };

    auto printResults = [&]() {
        std::cout << "\n";
        std::cout << "========================================\n";
        std::cout << "        KNOT ENGINE BENCHMARK\n";
        std::cout << "========================================\n";
        std::cout << "VSync: ON\n";
        std::cout << "\n";

        std::cout << std::left << std::setw(15) << "Objects" << std::setw(18) << "Average FPS" << std::setw(18) << "Minimum FPS"
                  << "\n";

        std::cout << "-------------------------------------------------\n";

        for (const auto& result : results) {
            std::cout << std::left << std::setw(15) << result.objectCount

                      << std::setw(18) << std::fixed << std::setprecision(2) << result.averageFPS

                      << std::setw(18) << result.minFPS

                      << "\n";
        }

        std::cout << "-------------------------------------------------\n";
        std::cout << "Benchmark complete.\n";
    };

    auto startTest = [&]() {
        if (benchmarkIndex >= objectCounts.size()) {
            finished = true;
            printResults();
            engine.quit();
            return;
        }

        removeObjects();

        const int objectCount = objectCounts[benchmarkIndex];

        std::cout << "\nTesting " << objectCount << " objects...\n";

        createObjects(objectCount);

        elapsedTime = 0.0;
        measuredTime = 0.0;
        frameCount = 0;

        measuring = false;
    };

    engine.setEventCallback([&](knot::Event& event) {
        if (event.type != knot::EventType::User)
            return;

        if (event.userCode == BENCHMARK_NEXT) {
            benchmarkIndex++;

            if (benchmarkIndex >= objectCounts.size()) {
                removeObjects();

                finished = true;

                printResults();
                engine.quit();
            } else {
                startTest();
            }

            event.handled = true;
        }
    });

    scene.setUpdateCallback([&](knot::Scene& currentScene, float deltaTime) {
        if (finished)
            return;

        elapsedTime += deltaTime;

        /*
         * Warmup
         *
         * 처음 몇 초 동안은 로딩이나 캐시 등의 영향을
         * 측정하지 않는다.
         */
        if (!measuring) {
            if (elapsedTime >= WARMUP_TIME) {
                measuring = true;

                elapsedTime = 0.0;
                measuredTime = 0.0;
                frameCount = 0;

                std::cout << "Measuring...\n";
            }

            return;
        }

        /*
         * Benchmark
         */
        frameCount++;
        measuredTime += deltaTime;

        if (measuredTime >= TEST_TIME) {
            const double averageFPS = static_cast<double>(frameCount) / measuredTime;

            // 일단 평균 FPS를 최소 FPS로도 기록한다.
            // 이후 frame time 기반 측정으로 개선 가능.
            const double minFPS = averageFPS;

            results.push_back({objectCounts[benchmarkIndex], averageFPS, minFPS});

            std::cout << "Result: " << objectCounts[benchmarkIndex] << " objects | Average FPS: " << std::fixed << std::setprecision(2) << averageFPS
                      << "\n";

            /*
             * 다음 테스트는 이벤트를 통해
             * Engine의 이벤트 시스템으로 전환한다.
             */
            engine.pushUserEvent(BENCHMARK_NEXT);
        }

        /*
         * 카메라를 천천히 움직여서
         * 매 프레임 동일한 장면만 렌더링하지 않도록 한다.
         */
        auto& camera = currentScene.getCamera();

        const float t = static_cast<float>(measuredTime);

        // TEST_TIME(5.0초) 동안 2 * M_PI 라디안을 도는 각속도 계산
        constexpr float TWO_PI = 6.28318530717958647692f;
        const float angularVelocity = TWO_PI / static_cast<float>(TEST_TIME);
        const float angle = t * angularVelocity;

        camera.position.x = std::sin(angle) * 20.0f;
        camera.position.y = 10.0f;
        camera.position.z = std::cos(angle) * 20.0f;

        camera.lookAtTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    });

    engine.setScene(scene);

    startTest();

    return engine.run();
}