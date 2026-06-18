// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#include <functional>
#include <string>

#include <knot/renderer.h>
#include <knot/window.h>
#include <knot/scene.h>

namespace knot {
class Engine {
public:
    bool init(int width, int height, const std::string& title, const std::string& assetRoot = "");
    int run();
    
    Window& getWindow();
    float getDeltaTime() const {
        return deltaTime;
    }
    float getAspectRatio() const;

    bool setScene(Scene& s);

private:
    Window window;
    Renderer renderer;

    int width = 0;
    int height = 0;
    std::string title;

    bool initialized = false;
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    Scene* scene = nullptr;

    void update();
    void render();
};
} // namespace knot
