// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#pragma once

#include <knot/key.h>
#include <any>

namespace knot {

enum EventType { None = 0, KeyInput, MouseMoved, MouseButton, WindowResize, User };

struct Event {
    EventType type = EventType::None;
    bool handled = false; // 이벤트 버블링 차단용 플래그

    // 키보드 이벤트 데이터
    ScanCode key = ScanCode::UNKNOWN;
    KeyState action = KeyState::RELEASE;

    // 마우스 이동 이벤트 데이터
    double x = 0.0;
    double y = 0.0;

    // 마우스 버튼 이벤트 데이터
    MouseKey mouseButton = MouseKey::BUTTON_LAST;

    // 윈도우 리사이즈 데이터 (나중에 확장용)
    int width = 0;
    int height = 0;

    uint32_t userCode = 0;
    std::any userData;
};

} // namespace knot