#pragma once

namespace knot {
// Derived from SDL3 SDL_scancode.h:
// https://github.com/libsdl-org/SDL/blob/72771ac0b41812065acda4b5c782000293d77ef5/include/SDL3/SDL_scancode.h
/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
enum class ScanCode : unsigned int {
    UNKNOWN = 0,
    A = 4,
    B = 5,
    C = 6,
    D = 7,
    E = 8,
    F = 9,
    G = 10,
    H = 11,
    I = 12,
    J = 13,
    K = 14,
    L = 15,
    M = 16,
    N = 17,
    O = 18,
    P = 19,
    Q = 20,
    R = 21,
    S = 22,
    T = 23,
    U = 24,
    V = 25,
    W = 26,
    X = 27,
    Y = 28,
    Z = 29,
    NUM_1 = 30,
    NUM_2 = 31,
    NUM_3 = 32,
    NUM_4 = 33,
    NUM_5 = 34,
    NUM_6 = 35,
    NUM_7 = 36,
    NUM_8 = 37,
    NUM_9 = 38,
    NUM_0 = 39,
    RETURN = 40,
    ESCAPE = 41,
    BACKSPACE = 42,
    TAB = 43,
    SPACE = 44,
    MINUS = 45,
    EQUALS = 46,
    LEFTBRACKET = 47,
    RIGHTBRACKET = 48,
    BACKSLASH = 49,
    NONUSHASH = 50,
    SEMICOLON = 51,
    APOSTROPHE = 52,
    GRAVE = 53,
    COMMA = 54,
    PERIOD = 55,
    SLASH = 56,
    CAPSLOCK = 57,
    F1 = 58,
    F2 = 59,
    F3 = 60,
    F4 = 61,
    F5 = 62,
    F6 = 63,
    F7 = 64,
    F8 = 65,
    F9 = 66,
    F10 = 67,
    F11 = 68,
    F12 = 69,
    PRINTSCREEN = 70,
    SCROLLLOCK = 71,
    PAUSE = 72,
    INSERT = 73,
    HOME = 74,
    PAGEUP = 75,
    DELETE = 76,
    END = 77,
    PAGEDOWN = 78,
    RIGHT = 79,
    LEFT = 80,
    DOWN = 81,
    UP = 82,
    NUMLOCKCLEAR = 83,
    KP_DIVIDE = 84,
    KP_MULTIPLY = 85,
    KP_MINUS = 86,
    KP_PLUS = 87,
    KP_ENTER = 88,
    KP_1 = 89,
    KP_2 = 90,
    KP_3 = 91,
    KP_4 = 92,
    KP_5 = 93,
    KP_6 = 94,
    KP_7 = 95,
    KP_8 = 96,
    KP_9 = 97,
    KP_0 = 98,
    KP_PERIOD = 99,
    NONUSBACKSLASH = 100,
    APPLICATION = 101,
    POWER = 102,
    KP_EQUALS = 103,
    F13 = 104,
    F14 = 105,
    F15 = 106,
    F16 = 107,
    F17 = 108,
    F18 = 109,
    F19 = 110,
    F20 = 111,
    F21 = 112,
    F22 = 113,
    F23 = 114,
    F24 = 115,
    EXECUTE = 116,
    HELP = 117,
    MENU = 118,
    SELECT = 119,
    STOP = 120,
    AGAIN = 121,
    UNDO = 122,
    CUT = 123,
    COPY = 124,
    PASTE = 125,
    FIND = 126,
    MUTE = 127,
    VOLUMEUP = 128,
    VOLUMEDOWN = 129,
    LOCKINGCAPSLOCK = 130,
    LOCKINGNUMLOCK = 131,
    LOCKINGSCROLLLOCK = 132,
    KP_COMMA = 133,
    KP_EQUALSAS400 = 134,
    INTERNATIONAL1 = 135,
    INTERNATIONAL2 = 136,
    INTERNATIONAL3 = 137,
    INTERNATIONAL4 = 138,
    INTERNATIONAL5 = 139,
    INTERNATIONAL6 = 140,
    INTERNATIONAL7 = 141,
    INTERNATIONAL8 = 142,
    INTERNATIONAL9 = 143,
    LANG1 = 144,
    LANG2 = 145,
    LANG3 = 146,
    LANG4 = 147,
    LANG5 = 148,
    LANG6 = 149,
    LANG7 = 150,
    LANG8 = 151,
    LANG9 = 152,
    ALTERASE = 153,
    SYSREQ = 154,
    CANCEL = 155,
    CLEAR = 156,
    PRIOR = 157,
    RETURN2 = 158,
    SEPARATOR = 159,
    OUT = 160,
    OPER = 161,
    CLEARAGAIN = 162,
    CRSEL = 163,
    EXSEL = 164,
    KP_00 = 176,
    KP_000 = 177,
    THOUSANDSSEPARATOR = 178,
    DECIMALSEPARATOR = 179,
    CURRENCYUNIT = 180,
    CURRENCYSUBUNIT = 181,
    KP_LEFTPAREN = 182,
    KP_RIGHTPAREN = 183,
    KP_LEFTBRACE = 184,
    KP_RIGHTBRACE = 185,
    KP_TAB = 186,
    KP_BACKSPACE = 187,
    KP_A = 188,
    KP_B = 189,
    KP_C = 190,
    KP_D = 191,
    KP_E = 192,
    KP_F = 193,
    KP_XOR = 194,
    KP_POWER = 195,
    KP_PERCENT = 196,
    KP_LESS = 197,
    KP_GREATER = 198,
    KP_AMPERSAND = 199,
    KP_DBLAMPERSAND = 200,
    KP_VERTICALBAR = 201,
    KP_DBLVERTICALBAR = 202,
    KP_COLON = 203,
    KP_HASH = 204,
    KP_SPACE = 205,
    KP_AT = 206,
    KP_EXCLAM = 207,
    KP_MEMSTORE = 208,
    KP_MEMRECALL = 209,
    KP_MEMCLEAR = 210,
    KP_MEMADD = 211,
    KP_MEMSUBTRACT = 212,
    KP_MEMMULTIPLY = 213,
    KP_MEMDIVIDE = 214,
    KP_PLUSMINUS = 215,
    KP_CLEAR = 216,
    KP_CLEARENTRY = 217,
    KP_BINARY = 218,
    KP_OCTAL = 219,
    KP_DECIMAL = 220,
    KP_HEXADECIMAL = 221,
    LCTRL = 224,
    LSHIFT = 225,
    LALT = 226,
    LGUI = 227,
    RCTRL = 228,
    RSHIFT = 229,
    RALT = 230,
    RGUI = 231,
    MODE = 257,
    SLEEP = 258,
    WAKE = 259,
    CHANNEL_INCREMENT = 260,
    CHANNEL_DECREMENT = 261,
    MEDIA_PLAY = 262,
    MEDIA_PAUSE = 263,
    MEDIA_RECORD = 264,
    MEDIA_FAST_FORWARD = 265,
    MEDIA_REWIND = 266,
    MEDIA_NEXT_TRACK = 267,
    MEDIA_PREVIOUS_TRACK = 268,
    MEDIA_STOP = 269,
    MEDIA_EJECT = 270,
    MEDIA_PLAY_PAUSE = 271,
    MEDIA_SELECT = 272,
    AC_NEW = 273,
    AC_OPEN = 274,
    AC_CLOSE = 275,
    AC_EXIT = 276,
    AC_SAVE = 277,
    AC_PRINT = 278,
    AC_PROPERTIES = 279,
    AC_SEARCH = 280,
    AC_HOME = 281,
    AC_BACK = 282,
    AC_FORWARD = 283,
    AC_STOP = 284,
    AC_REFRESH = 285,
    AC_BOOKMARKS = 286,
    SOFTLEFT = 287,
    SOFTRIGHT = 288,
    CALL = 289,
    ENDCALL = 290,
    RESERVED = 400,
    COUNT = 512,
};

// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

inline ScanCode convertGlfwToKnotScancode(int glfwKey) {
    switch (glfwKey) {
    // --- 알파벳 ---
    case GLFW_KEY_A:
        return ScanCode::A;
    case GLFW_KEY_B:
        return ScanCode::B;
    case GLFW_KEY_C:
        return ScanCode::C;
    case GLFW_KEY_D:
        return ScanCode::D;
    case GLFW_KEY_E:
        return ScanCode::E;
    case GLFW_KEY_F:
        return ScanCode::F;
    case GLFW_KEY_G:
        return ScanCode::G;
    case GLFW_KEY_H:
        return ScanCode::H;
    case GLFW_KEY_I:
        return ScanCode::I;
    case GLFW_KEY_J:
        return ScanCode::J;
    case GLFW_KEY_K:
        return ScanCode::K;
    case GLFW_KEY_L:
        return ScanCode::L;
    case GLFW_KEY_M:
        return ScanCode::M;
    case GLFW_KEY_N:
        return ScanCode::N;
    case GLFW_KEY_O:
        return ScanCode::O;
    case GLFW_KEY_P:
        return ScanCode::P;
    case GLFW_KEY_Q:
        return ScanCode::Q;
    case GLFW_KEY_R:
        return ScanCode::R;
    case GLFW_KEY_S:
        return ScanCode::S;
    case GLFW_KEY_T:
        return ScanCode::T;
    case GLFW_KEY_U:
        return ScanCode::U;
    case GLFW_KEY_V:
        return ScanCode::V;
    case GLFW_KEY_W:
        return ScanCode::W;
    case GLFW_KEY_X:
        return ScanCode::X;
    case GLFW_KEY_Y:
        return ScanCode::Y;
    case GLFW_KEY_Z:
        return ScanCode::Z;

    // --- 숫자 ---
    case GLFW_KEY_0:
        return ScanCode::NUM_0;
    case GLFW_KEY_1:
        return ScanCode::NUM_1;
    case GLFW_KEY_2:
        return ScanCode::NUM_2;
    case GLFW_KEY_3:
        return ScanCode::NUM_3;
    case GLFW_KEY_4:
        return ScanCode::NUM_4;
    case GLFW_KEY_5:
        return ScanCode::NUM_5;
    case GLFW_KEY_6:
        return ScanCode::NUM_6;
    case GLFW_KEY_7:
        return ScanCode::NUM_7;
    case GLFW_KEY_8:
        return ScanCode::NUM_8;
    case GLFW_KEY_9:
        return ScanCode::NUM_9;

    // --- 시스템 제어 및 주요 특수키 ---
    case GLFW_KEY_ENTER:
        return ScanCode::RETURN;
    case GLFW_KEY_ESCAPE:
        return ScanCode::ESCAPE;
    case GLFW_KEY_BACKSPACE:
        return ScanCode::BACKSPACE;
    case GLFW_KEY_TAB:
        return ScanCode::TAB;
    case GLFW_KEY_SPACE:
        return ScanCode::SPACE;
    case GLFW_KEY_MINUS:
        return ScanCode::MINUS;
    case GLFW_KEY_EQUAL:
        return ScanCode::EQUALS;
    case GLFW_KEY_LEFT_BRACKET:
        return ScanCode::LEFTBRACKET;
    case GLFW_KEY_RIGHT_BRACKET:
        return ScanCode::RIGHTBRACKET;
    case GLFW_KEY_BACKSLASH:
        return ScanCode::BACKSLASH;
    case GLFW_KEY_SEMICOLON:
        return ScanCode::SEMICOLON;
    case GLFW_KEY_APOSTROPHE:
        return ScanCode::APOSTROPHE;
    case GLFW_KEY_GRAVE_ACCENT:
        return ScanCode::GRAVE;
    case GLFW_KEY_COMMA:
        return ScanCode::COMMA;
    case GLFW_KEY_PERIOD:
        return ScanCode::PERIOD;
    case GLFW_KEY_SLASH:
        return ScanCode::SLASH;
    case GLFW_KEY_CAPS_LOCK:
        return ScanCode::CAPSLOCK;

    // --- 기능키 (F1 ~ F12) ---
    case GLFW_KEY_F1:
        return ScanCode::F1;
    case GLFW_KEY_F2:
        return ScanCode::F2;
    case GLFW_KEY_F3:
        return ScanCode::F3;
    case GLFW_KEY_F4:
        return ScanCode::F4;
    case GLFW_KEY_F5:
        return ScanCode::F5;
    case GLFW_KEY_F6:
        return ScanCode::F6;
    case GLFW_KEY_F7:
        return ScanCode::F7;
    case GLFW_KEY_F8:
        return ScanCode::F8;
    case GLFW_KEY_F9:
        return ScanCode::F9;
    case GLFW_KEY_F10:
        return ScanCode::F10;
    case GLFW_KEY_F11:
        return ScanCode::F11;
    case GLFW_KEY_F12:
        return ScanCode::F12;

    // --- 에디팅 및 네비게이션 ---
    case GLFW_KEY_PRINT_SCREEN:
        return ScanCode::PRINTSCREEN;
    case GLFW_KEY_SCROLL_LOCK:
        return ScanCode::SCROLLLOCK;
    case GLFW_KEY_PAUSE:
        return ScanCode::PAUSE;
    case GLFW_KEY_INSERT:
        return ScanCode::INSERT;
    case GLFW_KEY_HOME:
        return ScanCode::HOME;
    case GLFW_KEY_PAGE_UP:
        return ScanCode::PAGEUP;
    case GLFW_KEY_DELETE:
        return ScanCode::DELETE;
    case GLFW_KEY_END:
        return ScanCode::END;
    case GLFW_KEY_PAGE_DOWN:
        return ScanCode::PAGEDOWN;
    case GLFW_KEY_RIGHT:
        return ScanCode::RIGHT;
    case GLFW_KEY_LEFT:
        return ScanCode::LEFT;
    case GLFW_KEY_DOWN:
        return ScanCode::DOWN;
    case GLFW_KEY_UP:
        return ScanCode::UP;
    case GLFW_KEY_NUM_LOCK:
        return ScanCode::NUMLOCKCLEAR;

    // --- 키패드 (Numpad) ---
    case GLFW_KEY_KP_DIVIDE:
        return ScanCode::KP_DIVIDE;
    case GLFW_KEY_KP_MULTIPLY:
        return ScanCode::KP_MULTIPLY;
    case GLFW_KEY_KP_SUBTRACT:
        return ScanCode::KP_MINUS;
    case GLFW_KEY_KP_ADD:
        return ScanCode::KP_PLUS;
    case GLFW_KEY_KP_ENTER:
        return ScanCode::KP_ENTER;
    case GLFW_KEY_KP_1:
        return ScanCode::KP_1;
    case GLFW_KEY_KP_2:
        return ScanCode::KP_2;
    case GLFW_KEY_KP_3:
        return ScanCode::KP_3;
    case GLFW_KEY_KP_4:
        return ScanCode::KP_4;
    case GLFW_KEY_KP_5:
        return ScanCode::KP_5;
    case GLFW_KEY_KP_6:
        return ScanCode::KP_6;
    case GLFW_KEY_KP_7:
        return ScanCode::KP_7;
    case GLFW_KEY_KP_8:
        return ScanCode::KP_8;
    case GLFW_KEY_KP_9:
        return ScanCode::KP_9;
    case GLFW_KEY_KP_0:
        return ScanCode::KP_0;
    case GLFW_KEY_KP_DECIMAL:
        return ScanCode::KP_PERIOD;
    case GLFW_KEY_KP_EQUAL:
        return ScanCode::KP_EQUALS;

    // --- 모디파이어 키 ---
    case GLFW_KEY_LEFT_CONTROL:
        return ScanCode::LCTRL;
    case GLFW_KEY_LEFT_SHIFT:
        return ScanCode::LSHIFT;
    case GLFW_KEY_LEFT_ALT:
        return ScanCode::LALT;
    case GLFW_KEY_LEFT_SUPER:
        return ScanCode::LGUI;
    case GLFW_KEY_RIGHT_CONTROL:
        return ScanCode::RCTRL;
    case GLFW_KEY_RIGHT_SHIFT:
        return ScanCode::RSHIFT;
    case GLFW_KEY_RIGHT_ALT:
        return ScanCode::RALT;
    case GLFW_KEY_RIGHT_SUPER:
        return ScanCode::RGUI;

    // --- 미디어 키 ---
    case GLFW_KEY_MENU:
        return ScanCode::MENU;

    default:
        return ScanCode::UNKNOWN;
    }
}
} // namespace knot
