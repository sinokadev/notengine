// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#pragma once

#include <knot/key.h> 

namespace knot {

// 1. 이벤트 타입 정의
enum EventType {
    None = 0,
    KeyInput,
    MouseMoved,
    MouseButton,
    WindowResize
};

// 2. 통합 Event 클래스/구조체 정의
struct Event {
    EventType type = EventType::None;
    bool handled = false; // 이벤트 버블링 차단용 플래그

    // 키보드 이벤트 데이터
    ScanCode key = ScanCode::UNKNOWN;
    KeyState action = KeyState::RELEASE; // KeyState::PRESS 등

    // 마우스 이동 이벤트 데이터
    double x = 0.0;
    double y = 0.0;

    // 마우스 버튼 이벤트 데이터
    MouseKey mouseButton = MouseKey::BUTTON_LAST; // 필요에 따라 정의한 타입으로 매핑

    // 윈도우 리사이즈 데이터 (나중에 확장용)
    int width = 0;
    int height = 0;
};

} // namespace knot