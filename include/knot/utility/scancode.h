#include <array>
#include <GLFW/glfw3.h>

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
namespace detail {

inline const std::array<ScanCode, 351>& getGlfwToScanCodeTable() {
    static const std::array<ScanCode, 351> table = []() {
        std::array<ScanCode, 351> t{};
        t.fill(ScanCode::UNKNOWN);

        // --- 알파벳 (물리적 QWERTY 위치 기준) ---
        t[GLFW_KEY_A] = ScanCode::A;
        t[GLFW_KEY_B] = ScanCode::B;
        t[GLFW_KEY_C] = ScanCode::C;
        t[GLFW_KEY_D] = ScanCode::D;
        t[GLFW_KEY_E] = ScanCode::E;
        t[GLFW_KEY_F] = ScanCode::F;
        t[GLFW_KEY_G] = ScanCode::G;
        t[GLFW_KEY_H] = ScanCode::H;
        t[GLFW_KEY_I] = ScanCode::I;
        t[GLFW_KEY_J] = ScanCode::J;
        t[GLFW_KEY_K] = ScanCode::K;
        t[GLFW_KEY_L] = ScanCode::L;
        t[GLFW_KEY_M] = ScanCode::M;
        t[GLFW_KEY_N] = ScanCode::N;
        t[GLFW_KEY_O] = ScanCode::O;
        t[GLFW_KEY_P] = ScanCode::P;
        t[GLFW_KEY_Q] = ScanCode::Q;
        t[GLFW_KEY_R] = ScanCode::R;
        t[GLFW_KEY_S] = ScanCode::S;
        t[GLFW_KEY_T] = ScanCode::T;
        t[GLFW_KEY_U] = ScanCode::U;
        t[GLFW_KEY_V] = ScanCode::V;
        t[GLFW_KEY_W] = ScanCode::W;
        t[GLFW_KEY_X] = ScanCode::X;
        t[GLFW_KEY_Y] = ScanCode::Y;
        t[GLFW_KEY_Z] = ScanCode::Z;

        // --- 숫자 행 ---
        t[GLFW_KEY_0] = ScanCode::NUM_0;
        t[GLFW_KEY_1] = ScanCode::NUM_1;
        t[GLFW_KEY_2] = ScanCode::NUM_2;
        t[GLFW_KEY_3] = ScanCode::NUM_3;
        t[GLFW_KEY_4] = ScanCode::NUM_4;
        t[GLFW_KEY_5] = ScanCode::NUM_5;
        t[GLFW_KEY_6] = ScanCode::NUM_6;
        t[GLFW_KEY_7] = ScanCode::NUM_7;
        t[GLFW_KEY_8] = ScanCode::NUM_8;
        t[GLFW_KEY_9] = ScanCode::NUM_9;

        // --- 제어 및 특수 문자 키 ---
        t[GLFW_KEY_ENTER] = ScanCode::RETURN;
        t[GLFW_KEY_ESCAPE] = ScanCode::ESCAPE;
        t[GLFW_KEY_BACKSPACE] = ScanCode::BACKSPACE;
        t[GLFW_KEY_TAB] = ScanCode::TAB;
        t[GLFW_KEY_SPACE] = ScanCode::SPACE;
        t[GLFW_KEY_MINUS] = ScanCode::MINUS;
        t[GLFW_KEY_EQUAL] = ScanCode::EQUALS;
        t[GLFW_KEY_LEFT_BRACKET] = ScanCode::LEFTBRACKET;
        t[GLFW_KEY_RIGHT_BRACKET] = ScanCode::RIGHTBRACKET;
        t[GLFW_KEY_BACKSLASH] = ScanCode::BACKSLASH;
        t[GLFW_KEY_SEMICOLON] = ScanCode::SEMICOLON;
        t[GLFW_KEY_APOSTROPHE] = ScanCode::APOSTROPHE;
        t[GLFW_KEY_GRAVE_ACCENT] = ScanCode::GRAVE;
        t[GLFW_KEY_COMMA] = ScanCode::COMMA;
        t[GLFW_KEY_PERIOD] = ScanCode::PERIOD;
        t[GLFW_KEY_SLASH] = ScanCode::SLASH;
        t[GLFW_KEY_CAPS_LOCK] = ScanCode::CAPSLOCK;

        // --- Function Keys ---
        t[GLFW_KEY_F1] = ScanCode::F1;
        t[GLFW_KEY_F2] = ScanCode::F2;
        t[GLFW_KEY_F3] = ScanCode::F3;
        t[GLFW_KEY_F4] = ScanCode::F4;
        t[GLFW_KEY_F5] = ScanCode::F5;
        t[GLFW_KEY_F6] = ScanCode::F6;
        t[GLFW_KEY_F7] = ScanCode::F7;
        t[GLFW_KEY_F8] = ScanCode::F8;
        t[GLFW_KEY_F9] = ScanCode::F9;
        t[GLFW_KEY_F10] = ScanCode::F10;
        t[GLFW_KEY_F11] = ScanCode::F11;
        t[GLFW_KEY_F12] = ScanCode::F12;
        t[GLFW_KEY_F13] = ScanCode::F13;
        t[GLFW_KEY_F14] = ScanCode::F14;
        t[GLFW_KEY_F15] = ScanCode::F15;
        t[GLFW_KEY_F16] = ScanCode::F16;
        t[GLFW_KEY_F17] = ScanCode::F17;
        t[GLFW_KEY_F18] = ScanCode::F18;
        t[GLFW_KEY_F19] = ScanCode::F19;
        t[GLFW_KEY_F20] = ScanCode::F20;
        t[GLFW_KEY_F21] = ScanCode::F21;
        t[GLFW_KEY_F22] = ScanCode::F22;
        t[GLFW_KEY_F23] = ScanCode::F23;
        t[GLFW_KEY_F24] = ScanCode::F24;

        // --- 시스템 / 이동 키 ---
        t[GLFW_KEY_PRINT_SCREEN] = ScanCode::PRINTSCREEN;
        t[GLFW_KEY_SCROLL_LOCK] = ScanCode::SCROLLLOCK;
        t[GLFW_KEY_PAUSE] = ScanCode::PAUSE;
        t[GLFW_KEY_INSERT] = ScanCode::INSERT;
        t[GLFW_KEY_HOME] = ScanCode::HOME;
        t[GLFW_KEY_PAGE_UP] = ScanCode::PAGEUP;
        t[GLFW_KEY_DELETE] = ScanCode::DELETE;
        t[GLFW_KEY_END] = ScanCode::END;
        t[GLFW_KEY_PAGE_DOWN] = ScanCode::PAGEDOWN;
        t[GLFW_KEY_RIGHT] = ScanCode::RIGHT;
        t[GLFW_KEY_LEFT] = ScanCode::LEFT;
        t[GLFW_KEY_DOWN] = ScanCode::DOWN;
        t[GLFW_KEY_UP] = ScanCode::UP;
        t[GLFW_KEY_NUM_LOCK] = ScanCode::NUMLOCKCLEAR;

        // --- Numpad (키패드) ---
        t[GLFW_KEY_KP_DIVIDE] = ScanCode::KP_DIVIDE;
        t[GLFW_KEY_KP_MULTIPLY] = ScanCode::KP_MULTIPLY;
        t[GLFW_KEY_KP_SUBTRACT] = ScanCode::KP_MINUS;
        t[GLFW_KEY_KP_ADD] = ScanCode::KP_PLUS;
        t[GLFW_KEY_KP_ENTER] = ScanCode::KP_ENTER;
        t[GLFW_KEY_KP_1] = ScanCode::KP_1;
        t[GLFW_KEY_KP_2] = ScanCode::KP_2;
        t[GLFW_KEY_KP_3] = ScanCode::KP_3;
        t[GLFW_KEY_KP_4] = ScanCode::KP_4;
        t[GLFW_KEY_KP_5] = ScanCode::KP_5;
        t[GLFW_KEY_KP_6] = ScanCode::KP_6;
        t[GLFW_KEY_KP_7] = ScanCode::KP_7;
        t[GLFW_KEY_KP_8] = ScanCode::KP_8;
        t[GLFW_KEY_KP_9] = ScanCode::KP_9;
        t[GLFW_KEY_KP_0] = ScanCode::KP_0;
        t[GLFW_KEY_KP_DECIMAL] = ScanCode::KP_PERIOD;
        t[GLFW_KEY_KP_EQUAL] = ScanCode::KP_EQUALS;

        // --- Modifier Keys ---
        t[GLFW_KEY_LEFT_CONTROL] = ScanCode::LCTRL;
        t[GLFW_KEY_LEFT_SHIFT] = ScanCode::LSHIFT;
        t[GLFW_KEY_LEFT_ALT] = ScanCode::LALT;
        t[GLFW_KEY_LEFT_SUPER] = ScanCode::LGUI;
        t[GLFW_KEY_RIGHT_CONTROL] = ScanCode::RCTRL;
        t[GLFW_KEY_RIGHT_SHIFT] = ScanCode::RSHIFT;
        t[GLFW_KEY_RIGHT_ALT] = ScanCode::RALT;
        t[GLFW_KEY_RIGHT_SUPER] = ScanCode::RGUI;
        t[GLFW_KEY_MENU] = ScanCode::MENU;

        return t;
    }();

    return table;
}

} // namespace detail

inline ScanCode convertGlfwToKnotScancode(int glfwKey) {
    if (glfwKey < 0 || glfwKey > 350) {
        return ScanCode::UNKNOWN;
    }
    return detail::getGlfwToScanCodeTable()[glfwKey];
}
} // namespace knot
