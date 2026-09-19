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
    bool is_finished = false;

    std::function<float(float)> ease_func = [](float t) { return t; };

    void update(Object& obj, int dt) {
        if (is_finished) return;

        elapsed_time += dt;

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

// 일단 duration을 복제한 것에서 델타타임*1000을 뺴고
// duration동안 움직일 떄 델타타임*1000 만큼 움직이면 얼마나 움직였을지 알아내야함(아마도 이걸 보간이라 부르는 듯)
// target에서 원래위치만큼 빼면 이동해야할 거리가 나오는데
// 그러면 그 이동해야할 거리를 duration으로 나누면 1ms에 얼마나 움직어야할지 알수있겠네
// 어.........