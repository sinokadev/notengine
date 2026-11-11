// SPDX-License-Identifier: CC0 or Unlicense
// Copyright 2026 SinokaDev

// Rain demo: the first real use of knot::ParticleSystem.
//
// A skybox, a floor slab and a directional light carry the scene; the rain
// itself is one ParticleSystem registered with the scene, emitting downwards
// from a box that follows the camera. Run without arguments to fly around the
// rain with WASD; run with --frames N --screenshot out.png to capture a
// deterministic frame on a headless machine.

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <knot/engine.h>
#include <knot/particle.h>
#include <knot/renderer.h>
#include <knot/resources.h>
#include <knot/scene.h>
#include <knot/utility.h>
#include <GLFW/glfw3.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

namespace {

enum UserEventType : uint32_t { PRINT_FPS };

/** @brief Half extents of the rain spawn box around its origin, in world units. */
constexpr float kSpawnRadius = 14.0f;

/** @brief Half height of the rain spawn box, in world units. */
constexpr float kSpawnHeight = 6.0f;

/** @brief Height of the spawn box center above the camera, in world units. */
constexpr float kSpawnLift = 5.0f;

/** @brief Frame delta used when --frames is given, so captures are reproducible. */
constexpr float kFixedDelta = 1.0f / 60.0f;

/** @brief Emission rate of the rain preset at density 1.0, in particles per second. */
constexpr float kBaseEmissionRate = 9000.0f;

/** @brief Pool size of the rain preset at density 1.0. */
constexpr float kBaseMaxParticles = 30000.0f;

struct Options {
    int frames = 0;
    int width = 1280;
    int height = 720;
    std::string screenshot;
    uint32_t seed = 20261111u;
    float density = 1.0f;
    float speed = 1.0f;
    float stretch = 2.0f;
    bool orbit = false;
};

void printUsage(const char* exe) {
    std::cout << "Usage: " << exe << " [options]\n"
              << "\n"
              << "Renders the notengine rain demo. With no options it opens a window and\n"
              << "runs until the window is closed.\n"
              << "\n"
              << "Options:\n"
              << "  --frames N        quit after N rendered frames (0, the default, runs forever)\n"
              << "  --screenshot PATH save that final frame as a PNG (implies --frames 180)\n"
              << "  --seed N          particle random seed (default 20261111)\n"
              << "  --density F       emission rate and pool size multiplier (default 1.0)\n"
              << "  --speed F         fall speed multiplier (default 1.0)\n"
              << "  --stretch F       velocity-to-streak-length factor (default 2.0)\n"
              << "  --orbit           move the camera in a slow circle to show the rain follows it\n"
              << "  --width N         window width (default 1280)\n"
              << "  --height N        window height (default 720)\n"
              << "  --help            print this help and exit\n"
              << "\n"
              << "Keys:\n"
              << "  1                 toggle rain on/off\n"
              << "  2 / 3             halve / double the rain density\n"
              << "  ESC               release/capture the cursor\n"
              << "  W A S D           move the camera\n"
              << "\n"
              << "Headless capture:\n"
              << "  xvfb-run -a -s \"-screen 0 1280x720x24\" env LIBGL_ALWAYS_SOFTWARE=1 \\\n"
              << "      ./build/ninja-release/particle_rain --frames 180 --screenshot rain.png\n";
}

bool parseFloat(const char* text, float& out) {
    char* end = nullptr;
    const float value = std::strtof(text, &end);

    if (end == text || *end != '\0' || !std::isfinite(value))
        return false;

    out = value;
    return true;
}

bool parseInt(const char* text, int& out) {
    char* end = nullptr;
    const long value = std::strtol(text, &end, 10);

    if (end == text || *end != '\0')
        return false;

    out = static_cast<int>(value);
    return true;
}

/**
 * @brief Parses the command line.
 *
 * @return 0 to continue, 1 on a usage error, 2 when help was printed.
 */
int parseArgs(int argc, char** argv, Options& opt) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        const bool hasValue = i + 1 < argc;

        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 2;
        } else if (arg == "--frames" && hasValue) {
            if (!parseInt(argv[++i], opt.frames) || opt.frames < 0) {
                std::cerr << "[Error] --frames needs a non-negative integer\n";
                return 1;
            }
        } else if (arg == "--screenshot" && hasValue) {
            opt.screenshot = argv[++i];
        } else if (arg == "--seed" && hasValue) {
            int seed = 0;

            if (!parseInt(argv[++i], seed) || seed < 0) {
                std::cerr << "[Error] --seed needs a non-negative integer\n";
                return 1;
            }

            opt.seed = static_cast<uint32_t>(seed);
        } else if (arg == "--density" && hasValue) {
            if (!parseFloat(argv[++i], opt.density) || opt.density <= 0.0f) {
                std::cerr << "[Error] --density needs a positive number\n";
                return 1;
            }

            opt.density = glm::clamp(opt.density, 0.05f, 8.0f);
        } else if (arg == "--speed" && hasValue) {
            if (!parseFloat(argv[++i], opt.speed) || opt.speed <= 0.0f) {
                std::cerr << "[Error] --speed needs a positive number\n";
                return 1;
            }

            opt.speed = glm::clamp(opt.speed, 0.05f, 8.0f);
        } else if (arg == "--stretch" && hasValue) {
            if (!parseFloat(argv[++i], opt.stretch) || opt.stretch < 0.0f) {
                std::cerr << "[Error] --stretch needs a non-negative number\n";
                return 1;
            }
        } else if (arg == "--orbit") {
            opt.orbit = true;
        } else if (arg == "--width" && hasValue) {
            if (!parseInt(argv[++i], opt.width) || opt.width <= 0) {
                std::cerr << "[Error] --width needs a positive integer\n";
                return 1;
            }
        } else if (arg == "--height" && hasValue) {
            if (!parseInt(argv[++i], opt.height) || opt.height <= 0) {
                std::cerr << "[Error] --height needs a positive integer\n";
                return 1;
            }
        } else {
            std::cerr << "[Error] Unknown or incomplete argument: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    if (!opt.screenshot.empty() && opt.frames <= 0) {
        opt.frames = 180;
        std::cout << "[Info] --screenshot without --frames: capturing frame 180\n";
    }

    return 0;
}

/**
 * @brief Builds the rain preset.
 *
 * The spawn box is a tall square volume around the system origin (which the
 * demo keeps on the camera), the particles fall almost straight down with a
 * light prevailing wind, and stretch turns their velocity into a thin streak.
 *
 * @param density Multiplies emission rate and pool size.
 * @param speed Multiplies the initial fall speed.
 * @return Settings ready for a ParticleSystem.
 */
knot::ParticleSystemSettings makeRainSettings(float density, float speed) {
    knot::ParticleSystemSettings settings;

    settings.maxParticles = static_cast<unsigned int>(std::lround(kBaseMaxParticles * density));
    settings.maxParticles = std::max(512u, std::min(settings.maxParticles, 200000u));

    settings.emissionRate = kBaseEmissionRate * density;

    settings.spawnHalfExtents = glm::vec3(kSpawnRadius, kSpawnHeight, kSpawnRadius);
    settings.lifetimeRange = glm::vec2(0.8f, 1.4f);
    settings.speedRange = glm::vec2(7.0f * speed, 10.0f * speed);
    settings.direction = glm::vec3(0.0f, -1.0f, 0.0f);
    settings.directionJitter = 0.03f;

    // Rain reaches the ground at nearly terminal velocity, so the extra
    // acceleration is small and mostly there to bend older streaks.
    settings.gravity = glm::vec3(0.0f, -3.0f, 0.0f);
    settings.wind = glm::vec3(0.9f, 0.0f, 0.35f);
    settings.drag = 0.0f;

    settings.sizeRange = glm::vec2(0.006f, 0.010f);
    settings.color = glm::vec4(0.62f, 0.72f, 0.88f, 0.45f);

    return settings;
}

} // namespace

int main(int argc, char** argv) {
    Options opt;

    const int parseResult = parseArgs(argc, argv, opt);

    if (parseResult == 2)
        return 0;

    if (parseResult != 0)
        return parseResult;

    knot::Engine engine;

    if (!engine.init(opt.width, opt.height, "Knot Rain Demo")) {
        return 1;
    }

    if (opt.frames > 0) {
        // A fixed timestep makes the particle simulation and the camera path a
        // pure function of the frame count, so two captures of the same seed
        // are identical.
        engine.setFixedDeltaTime(kFixedDelta);
    }

    glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    engine.setClearColor(0.09f, 0.11f, 0.14f, 1.0f);

    knot::Scene scene;

    scene.loadHDRMap(knot::getAssetRoot() + "assets/DaySkyHDRI015A_2K_HDR.hdr");

    auto shader = scene.getShaderManager().getShader("pbrShader");

    // The floor: a unit cube flattened into a slab the rain falls onto. The demo
    // cube model (assets/Untitled.obj) carries the engine logo texture, which
    // stretched over a ground plane reads as huge dark polygons, so the floor is
    // built from an untextured cube instead.
    auto floorMesh = knot::createCube();

    floorMesh->setup();

    auto floorMaterial = std::make_shared<knot::PbrMaterial>(shader, glm::vec3(0.30f, 0.32f, 0.36f), 0.0f, 0.9f);

    auto floorModel = std::make_shared<knot::Model>(floorMesh, floorMaterial);

    auto floor = std::make_shared<knot::Object>(floorModel);

    floor->position = glm::vec3(0.0f, -2.0f, 0.0f);
    floor->scale = glm::vec3(60.0f, 0.1f, 60.0f);

    scene.getObjectManager().registerObject(floor);

    auto dirLight = std::make_shared<knot::DirLight>();

    dirLight->rotation = glm::quat(glm::vec3(glm::radians(45.0f), glm::radians(-20.0f), 0.0f));
    dirLight->ambient = glm::vec3(0.15f);
    dirLight->diffuse = glm::vec3(1.0f);
    dirLight->specular = glm::vec3(1.0f);

    scene.getLightManager().registerLight(dirLight);

    auto camera = std::make_shared<knot::MovingCamera>(glm::vec3(0.0f, 2.0f, 10.0f));

    // Look slightly down so the floor is in frame and the streaks read against it.
    // MovingCamera exposes pitch only through rotate(), which scales by sensitivity.
    constexpr float kPitchDownDegrees = 6.0f;

    camera->rotate(0.0f, -kPitchDownDegrees / camera->sensitivity);

    scene.setCamera(camera);

    knot::Renderer::get().setParticleStretch(opt.stretch);

    float density = opt.density;

    auto rain = std::make_shared<knot::ParticleSystem>(makeRainSettings(density, opt.speed));

    rain->seed(opt.seed);
    rain->setOrigin(camera->position + glm::vec3(0.0f, kSpawnLift, 0.0f));

    scene.addParticleSystem(rain);

    std::cout << "[Info] Rain: seed=" << opt.seed << " density=" << density << " speed=" << opt.speed << " stretch=" << opt.stretch
              << " pool=" << rain->capacity() << "\n";
    std::cout << "[Info] Controls: 1=rain on/off, 2/3=density -/+, ESC=cursor, WASD=move\n";

    std::unordered_map<knot::ScanCode, bool> keyStates;

    float lastX = static_cast<float>(opt.width) * 0.5f;
    float lastY = static_cast<float>(opt.height) * 0.5f;

    bool cursorReleased = false;
    bool rainEnabled = true;

    int frameCount = 0;
    int renderedFrames = 0;
    float elapsed = 0.0f;

    engine.repeat(1000, PRINT_FPS);

    engine.setEventCallback([&](knot::Event& event) {
        if (event.type == knot::KeyInput) {
            if (event.action == knot::KeyState::PRESS) {
                if (event.key == knot::ScanCode::ESCAPE) {
                    cursorReleased = !cursorReleased;

                    glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, cursorReleased ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

                    event.handled = true;
                }

                if (event.key == knot::ScanCode::NUM_1) {
                    rainEnabled = !rainEnabled;

                    auto settings = rain->settings();
                    settings.emissionRate = rainEnabled ? makeRainSettings(density, opt.speed).emissionRate : 0.0f;
                    rain->setSettings(settings);

                    std::cout << "[Info] Rain " << (rainEnabled ? "on" : "off") << "\n";
                    event.handled = true;
                }

                if (event.key == knot::ScanCode::NUM_2 || event.key == knot::ScanCode::NUM_3) {
                    const float factor = event.key == knot::ScanCode::NUM_2 ? 0.5f : 2.0f;

                    density = glm::clamp(density * factor, 0.05f, 8.0f);

                    auto settings = rain->settings();
                    settings.emissionRate = rainEnabled ? makeRainSettings(density, opt.speed).emissionRate : 0.0f;
                    rain->setSettings(settings);

                    std::cout << "[Info] Density " << density << "\n";
                    event.handled = true;
                }

                keyStates[event.key] = true;
            } else if (event.action == knot::KeyState::RELEASE) {
                keyStates[event.key] = false;
            }
        }

        if (event.type == knot::MouseMoved) {
            if (cursorReleased)
                return;

            const float xOffset = static_cast<float>(event.x) - lastX;
            const float yOffset = lastY - static_cast<float>(event.y);

            lastX = static_cast<float>(event.x);
            lastY = static_cast<float>(event.y);

            camera->rotate(xOffset, yOffset);

            event.handled = true;
        }

        if (event.type == knot::EventType::User) {
            if (event.userCode == PRINT_FPS) {
                std::cout << "FPS: " << frameCount << " alive: " << rain->aliveCount() << "\n";

                frameCount = 0;
                event.handled = true;
            }
        }
    });

    scene.setUpdateCallback([&](knot::Scene& currentScene, float deltaTime) {
        (void)currentScene;

        frameCount++;
        elapsed += deltaTime;

        if (opt.orbit) {
            const float angle = elapsed * 0.35f;

            camera->position = glm::vec3(std::cos(angle) * 8.0f, 2.0f, std::sin(angle) * 8.0f);
        } else if (!cursorReleased) {
            glm::vec3 moveDir(0.0f);

            const glm::vec3 front = camera->getFront();
            const glm::vec3 right = camera->getRight();

            if (keyStates[knot::ScanCode::W])
                moveDir += front;

            if (keyStates[knot::ScanCode::S])
                moveDir -= front;

            if (keyStates[knot::ScanCode::A])
                moveDir -= right;

            if (keyStates[knot::ScanCode::D])
                moveDir += right;

            if (glm::length(moveDir) > 0.0f)
                camera->move(glm::normalize(moveDir), deltaTime);
        }

        // The whole point of the origin: new drops are born around the camera,
        // so the rain never thins out when the camera moves. Live particles
        // keep their world positions and simply fall out of the box.
        rain->setOrigin(camera->position + glm::vec3(0.0f, kSpawnLift, 0.0f));
    });

    engine.setRenderLoopCallback([&](float) {
        ++renderedFrames;

        if (opt.frames > 0 && renderedFrames >= opt.frames) {
            if (!opt.screenshot.empty()) {
                const bool saved = engine.saveScreenshot(opt.screenshot, opt.width, opt.height);

                std::cout << "[Rain] frame " << renderedFrames << " alive=" << rain->aliveCount() << " screenshot=" << (saved ? "ok" : "failed")
                          << " path=" << opt.screenshot << "\n";

                if (!saved)
                    std::cerr << "[Error] Could not write " << opt.screenshot << "\n";
            } else {
                std::cout << "[Rain] frame " << renderedFrames << " alive=" << rain->aliveCount() << "\n";
            }

            engine.quit();
        }
    });

    engine.setScene(scene);

    engine.setMakingMusic(false);

    return engine.run();
}
