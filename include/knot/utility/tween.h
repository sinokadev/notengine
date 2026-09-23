// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <functional>
#include <algorithm>
#include <knot/resources.h>


namespace knot {
inline Transform mixTransform(const Transform& start, const Transform& target, float t) {
    Transform result;

    result.position = glm::mix(start.position, target.position, t);
    result.rotation = glm::slerp(start.rotation, target.rotation, t);
    result.scale = glm::mix(start.scale, target.scale, t);
    
    return result;
}

struct Tween {
    Transform start;
    Transform target;
    int duration = 0;  // ms
    int elapsed_time = 0;
    float millisecondRemainder = 0.0f;
    bool is_finished = true;

    std::function<float(float)> ease_func = [](float t) { return t; };

    void update(Object& obj, float dt) {
        if (is_finished) return;

        millisecondRemainder += dt * 1000.0f;
        const int deltaTimeMs = static_cast<int>(millisecondRemainder);
        millisecondRemainder -= static_cast<float>(deltaTimeMs);
        elapsed_time += deltaTimeMs;

        float progress = duration > 0 ? std::clamp(static_cast<float>(elapsed_time) / static_cast<float>(duration), 0.0f, 1.0f) : 1.0f;
        float t = ease_func ? ease_func(progress) : progress;

        Transform current = mixTransform(start, target, t);
        obj.position = current.position;
        obj.rotation = current.rotation;
        obj.scale = current.scale;

        if (progress >= 1.0f) {
            is_finished = true;
        }
    }
};

}
