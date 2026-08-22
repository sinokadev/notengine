// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#pragma once

#include <knot/key.h>
#include <any>

namespace knot {

/** @brief Categories of input, window, and application events. */
enum EventType { None = 0, KeyInput, MouseMoved, MouseButton, WindowResize, User };

/** @brief Data delivered to an Engine event callback.
 *
 * Only the fields relevant to type are populated. Set handled to true to let
 * application code mark an event as consumed. */
struct Event {
    /** @brief Category that determines which data fields are meaningful. */
    EventType type = EventType::None;
    /** @brief Application-managed flag indicating that the event was consumed. */
    bool handled = false; // 이벤트 버블링 차단용 플래그

    /** @brief Keyboard scancode for EventType::KeyInput. */
    ScanCode key = ScanCode::UNKNOWN;
    /** @brief Press, release, or repeat state for keyboard and mouse events. */
    KeyState action = KeyState::RELEASE;

    /** @brief Cursor x coordinate for EventType::MouseMoved. */
    double x = 0.0;
    /** @brief Cursor y coordinate for EventType::MouseMoved. */
    double y = 0.0;

    /** @brief Mouse button for EventType::MouseButton. */
    MouseKey mouseButton = MouseKey::BUTTON_LAST;

    /** @brief New framebuffer width for EventType::WindowResize. */
    int width = 0;
    /** @brief New framebuffer height for EventType::WindowResize. */
    int height = 0;

    /** @brief Application-defined code for EventType::User. */
    uint32_t userCode = 0;
    /** @brief Application-defined payload for EventType::User. */
    std::any userData;
};

} // namespace knot
