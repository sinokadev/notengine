#include <knot/renderer.h>
#include <knot/engine.h>
#include <knot/window.h>
#include <fstream>
#include <iostream>
#include <stdexcept>

static void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}
int main(int argc, char** argv) {
    auto& renderer = knot::Renderer::get();
    GLFWwindow* window = nullptr;
    try {
        require(glfwInit(), "GLFW init failed");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        window = glfwCreateWindow(640, 480, "Vulkan smoke", nullptr, nullptr);
        require(window, "Window creation failed");
        const std::string screenshot = argc > 1 ? argv[1] : "vulkan-smoke.ppm";
        // Exercise complete shutdown/reinitialization while client resources still exist.
        for (int cycle = 0; cycle < 2; ++cycle) {
            require(renderer.init(window), "Renderer initialization failed");
            knot::Scene scene;
            require(scene.loadSeno(knot::getAssetRoot() + "assets/scene.seno"), "Seno scene loading failed");
            auto camera = std::make_shared<knot::PerspectiveCamera>(glm::vec3(6, 4, 8));
            camera->rotation = glm::quatLookAt(glm::normalize(glm::vec3(0, 1, 0) - camera->position), glm::vec3(0, 1, 0));
            scene.setCamera(camera);
            // At least four instances exercise a real instanced indexed draw.
            auto model = scene.getObjectManager().getObject(100)->model;
            for (int i = 0; i < 4; ++i) {
                auto object = std::make_shared<knot::Object>(model);
                object->position = {float(i * 2 - 3), 1, -3};
                scene.getObjectManager().registerObject(object);
            }
            require(!renderer.beginFrame(0, 0), "Minimized frame should be skipped");
            int rendered = 0;
            for (int attempt = 0; attempt < 100 && rendered < 8; ++attempt) {
                glfwPollEvents();
                int w, h;
                glfwGetFramebufferSize(window, &w, &h);
                if (!renderer.beginFrame(w, h))
                    continue;
                if (rendered == 1) {
                    auto image = renderer.renderSceneToTexture(&scene, 320, 240);
                    require(image.imageView != VK_NULL_HANDLE, "Missing offscreen image");
                }
                if (rendered == 2) {
                    auto image = renderer.renderSceneToTexture(&scene, 400, 300);
                    require(image.imageView != VK_NULL_HANDLE, "Offscreen resize failed");
                }
                require(renderer.renderScene(scene, float(w) / h), "Scene render failed");
                if (rendered == 7)
                    renderer.requestScreenshot(screenshot);
                renderer.endFrame();
                ++rendered;
                if (rendered == 3)
                    glfwSetWindowSize(window, 800, 600);
                if (rendered == 5)
                    renderer.setVsync(false);
            }
            require(rendered == 8, "No frames presented");
            // Exercise an orthographic camera with the same clip-space conversion.
            auto ortho = std::make_shared<knot::OrthographicCamera>(camera->position);
            ortho->rotation = camera->rotation;
            ortho->size = 12;
            scene.setCamera(ortho);
            int w, h;
            glfwGetFramebufferSize(window, &w, &h);
            if (renderer.beginFrame(w, h)) {
                renderer.renderScene(scene, float(w) / h);
                renderer.endFrame();
            }
            // A pixel assertion distinguishes real mesh/depth/material rendering from a sky-only frame.
            knot::Scene colors;
            auto shader = colors.getResourceManager().getShader("alphaShader");
            auto nearMaterial = std::make_shared<knot::PbrMaterial>(shader, glm::vec3(1, 0, 0));
            auto farMaterial = std::make_shared<knot::AlphaMaterial>(shader, glm::vec3(0, 0, 1));
            auto geometry = knot::createCube();
            auto nearObject = std::make_shared<knot::Object>(std::make_shared<knot::Model>(geometry, nearMaterial));
            auto farObject = std::make_shared<knot::Object>(std::make_shared<knot::Model>(geometry, farMaterial));
            farObject->position.z = -1;
            colors.getObjectManager().registerObject(nearObject);
            colors.getObjectManager().registerObject(farObject);
            const std::string colorShot = screenshot + ".material.ppm";
            for (int color = 0; color < 2; ++color) {
                if (color)
                    nearMaterial->setAlbedoColor({0, 1, 0});
                bool captured = false;
                for (int attempt = 0; attempt < 20 && !captured; ++attempt) {
                    glfwPollEvents();
                    glfwGetFramebufferSize(window, &w, &h);
                    if (!renderer.beginFrame(w, h))
                        continue;
                    renderer.renderScene(colors, float(w) / h);
                    renderer.requestScreenshot(colorShot);
                    renderer.endFrame();
                    captured = true;
                }
                require(captured, "Material test did not render");
                std::ifstream shot(colorShot, std::ios::binary);
                std::string magic;
                int sw, sh, maximum;
                shot >> magic >> sw >> sh >> maximum;
                shot.get();
                require(magic == "P6" && sw == w && sh == h, "Material capture dimensions incorrect");
                shot.seekg((static_cast<std::streamoff>(sh / 2) * sw + sw / 2) * 3, std::ios::cur);
                unsigned char pixel[3]{};
                shot.read(reinterpret_cast<char*>(pixel), 3);
                require(bool(shot), "Missing material pixel");
                require(pixel[color] > 240 && pixel[1 - color] < 10 && pixel[2] < 10, "Mesh depth or material color update failed");
            }
            renderer.waitIdle();
            renderer.shutdown();
            require(renderer.getValidationErrorCount() == 0, "Vulkan validation errors detected");
        }
        std::ifstream input(screenshot, std::ios::binary);
        std::string magic;
        int w, h, max;
        input >> magic >> w >> h >> max;
        input.get();
        require(magic == "P6" && w > 0 && h > 0 && max == 255, "Invalid screenshot");
        std::vector<unsigned char> pixels(size_t(w) * h * 3);
        input.read(reinterpret_cast<char*>(pixels.data()), pixels.size());
        require(bool(input), "Incomplete screenshot");
        size_t different = 0;
        for (size_t i = 3; i < pixels.size(); i += 3)
            if (pixels[i] != pixels[0] || pixels[i + 1] != pixels[1] || pixels[i + 2] != pixels[2])
                ++different;
        require(different > size_t(w) * h / 10, "Rendered image is blank");
        glfwDestroyWindow(window);
        window = nullptr;
        glfwTerminate();
        require(glfwInit(), "Engine test GLFW init failed");
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        knot::Engine engine;
        require(engine.init(320, 240, "Vulkan engine smoke"), "Engine init failed");
        knot::Scene engineScene;
        int updates = 0;
        engineScene.setUpdateCallback([&](knot::Scene&, float) {
            if (++updates == 3)
                engine.quit();
        });
        engine.setScene(engineScene);
        require(engine.run() == 0 && updates == 3, "Engine frame loop failed");
        require(renderer.getValidationErrorCount() == 0, "Engine validation errors detected");
        std::cout << "Vulkan smoke passed: scene, instancing, HDR/PBR, shadows, resize, offscreen, orthographic, reinit, capture\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        renderer.shutdown();
        if (window)
            glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
}
