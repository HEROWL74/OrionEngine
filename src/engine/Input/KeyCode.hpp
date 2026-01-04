// src/Input/KeyCode.hpp
#pragma once

#include <Windows.h>
#include <cstdint>

namespace Engine::Input
{
    // キーコード定義
    // Win32 Virtual Key Codesをベースとしているが、
    // エンジン独自の拡張も可能
    enum class KeyCode : uint32_t
    {
        // 文字キー
        A = 0x41, B = 0x42, C = 0x43, D = 0x44, E = 0x45, F = 0x46,
        G = 0x47, H = 0x48, I = 0x49, J = 0x4A, K = 0x4B, L = 0x4C,
        M = 0x4D, N = 0x4E, O = 0x4F, P = 0x50, Q = 0x51, R = 0x52,
        S = 0x53, T = 0x54, U = 0x55, V = 0x56, W = 0x57, X = 0x58,
        Y = 0x59, Z = 0x5A,

        // 数字キー（メイン）
        Key0 = 0x30, Key1 = 0x31, Key2 = 0x32, Key3 = 0x33, Key4 = 0x34,
        Key5 = 0x35, Key6 = 0x36, Key7 = 0x37, Key8 = 0x38, Key9 = 0x39,

        // 数字キー（テンキー）
        Numpad0 = VK_NUMPAD0, Numpad1 = VK_NUMPAD1, Numpad2 = VK_NUMPAD2,
        Numpad3 = VK_NUMPAD3, Numpad4 = VK_NUMPAD4, Numpad5 = VK_NUMPAD5,
        Numpad6 = VK_NUMPAD6, Numpad7 = VK_NUMPAD7, Numpad8 = VK_NUMPAD8,
        Numpad9 = VK_NUMPAD9,

        // 機能キー
        F1 = VK_F1, F2 = VK_F2, F3 = VK_F3, F4 = VK_F4,
        F5 = VK_F5, F6 = VK_F6, F7 = VK_F7, F8 = VK_F8,
        F9 = VK_F9, F10 = VK_F10, F11 = VK_F11, F12 = VK_F12,

        // 矢印キー
        Up = VK_UP, Down = VK_DOWN, Left = VK_LEFT, Right = VK_RIGHT,

        // 修飾キー
        LeftShift = VK_LSHIFT, RightShift = VK_RSHIFT,
        LeftCtrl = VK_LCONTROL, RightCtrl = VK_RCONTROL,
        LeftAlt = VK_LMENU, RightAlt = VK_RMENU,
        LeftWin = VK_LWIN, RightWin = VK_RWIN,

        // 特殊キー
        Space = VK_SPACE, Enter = VK_RETURN, Escape = VK_ESCAPE,
        Tab = VK_TAB, Backspace = VK_BACK, Delete = VK_DELETE,
        Insert = VK_INSERT, Home = VK_HOME, End = VK_END,
        PageUp = VK_PRIOR, PageDown = VK_NEXT,

        // 区切り文字
        Semicolon = VK_OEM_1,        // ; :
        Plus = VK_OEM_PLUS,          // + =
        Comma = VK_OEM_COMMA,        // , <
        Minus = VK_OEM_MINUS,        // - _
        Period = VK_OEM_PERIOD,      // . >
        Slash = VK_OEM_2,            // / ?
        Tilde = VK_OEM_3,            // ` ~
        LeftBracket = VK_OEM_4,      // [ {
        Backslash = VK_OEM_5,        // \ |
        RightBracket = VK_OEM_6,     // ] }
        Quote = VK_OEM_7,            // ' "

        // マウスボタン（入力システムの統一性のため）
        MouseLeft = VK_LBUTTON,
        MouseRight = VK_RBUTTON,
        MouseMiddle = VK_MBUTTON,
        MouseX1 = VK_XBUTTON1,
        MouseX2 = VK_XBUTTON2,

        // 無効値
        None = 0
    };

    // キーコードを文字列に変換（デバッグ用）
    inline const char* keyCodeToString(KeyCode keyCode)
    {
        switch (keyCode)
        {
        case KeyCode::A: return "A";
        case KeyCode::B: return "B";
        case KeyCode::C: return "C";
        case KeyCode::D: return "D";
        case KeyCode::E: return "E";
        case KeyCode::F: return "F";
        case KeyCode::G: return "G";
        case KeyCode::H: return "H";
        case KeyCode::I: return "I";
        case KeyCode::J: return "J";
        case KeyCode::K: return "K";
        case KeyCode::L: return "L";
        case KeyCode::M: return "M";
        case KeyCode::N: return "N";
        case KeyCode::O: return "O";
        case KeyCode::P: return "P";
        case KeyCode::Q: return "Q";
        case KeyCode::R: return "R";
        case KeyCode::S: return "S";
        case KeyCode::T: return "T";
        case KeyCode::U: return "U";
        case KeyCode::V: return "V";
        case KeyCode::W: return "W";
        case KeyCode::X: return "X";
        case KeyCode::Y: return "Y";
        case KeyCode::Z: return "Z";
        case KeyCode::Key0: return "0";
        case KeyCode::Key1: return "1";
        case KeyCode::Key2: return "2";
        case KeyCode::Key3: return "3";
        case KeyCode::Key4: return "4";
        case KeyCode::Key5: return "5";
        case KeyCode::Key6: return "6";
        case KeyCode::Key7: return "7";
        case KeyCode::Key8: return "8";
        case KeyCode::Key9: return "9";
        case KeyCode::Space: return "Space";
        case KeyCode::Enter: return "Enter";
        case KeyCode::Escape: return "Escape";
        case KeyCode::Tab: return "Tab";
        case KeyCode::LeftShift: return "LeftShift";
        case KeyCode::RightShift: return "RightShift";
        case KeyCode::LeftCtrl: return "LeftCtrl";
        case KeyCode::RightCtrl: return "RightCtrl";
        case KeyCode::LeftAlt: return "LeftAlt";
        case KeyCode::RightAlt: return "RightAlt";
        case KeyCode::Up: return "Up";
        case KeyCode::Down: return "Down";
        case KeyCode::Left: return "Left";
        case KeyCode::Right: return "Right";
        case KeyCode::F1: return "F1";
        case KeyCode::F2: return "F2";
        case KeyCode::F3: return "F3";
        case KeyCode::F4: return "F4";
        case KeyCode::F5: return "F5";
        case KeyCode::F6: return "F6";
        case KeyCode::F7: return "F7";
        case KeyCode::F8: return "F8";
        case KeyCode::F9: return "F9";
        case KeyCode::F10: return "F10";
        case KeyCode::F11: return "F11";
        case KeyCode::F12: return "F12";
        case KeyCode::MouseLeft: return "MouseLeft";
        case KeyCode::MouseRight: return "MouseRight";
        case KeyCode::MouseMiddle: return "MouseMiddle";
        case KeyCode::None: return "None";
        default: return "Unknown";
        }
    }

    // 文字列をキーコードに変換（設定ファイル読み込み用）
    inline KeyCode stringToKeyCode(const char* str)
    {
        if (!str) return KeyCode::None;

        std::string s(str);

        // 文字キー
        if (s == "A") return KeyCode::A;
        if (s == "B") return KeyCode::B;
        if (s == "C") return KeyCode::C;
        if (s == "D") return KeyCode::D;
        if (s == "E") return KeyCode::E;
        if (s == "F") return KeyCode::F;
        if (s == "G") return KeyCode::G;
        if (s == "H") return KeyCode::H;
        if (s == "I") return KeyCode::I;
        if (s == "J") return KeyCode::J;
        if (s == "K") return KeyCode::K;
        if (s == "L") return KeyCode::L;
        if (s == "M") return KeyCode::M;
        if (s == "N") return KeyCode::N;
        if (s == "O") return KeyCode::O;
        if (s == "P") return KeyCode::P;
        if (s == "Q") return KeyCode::Q;
        if (s == "R") return KeyCode::R;
        if (s == "S") return KeyCode::S;
        if (s == "T") return KeyCode::T;
        if (s == "U") return KeyCode::U;
        if (s == "V") return KeyCode::V;
        if (s == "W") return KeyCode::W;
        if (s == "X") return KeyCode::X;
        if (s == "Y") return KeyCode::Y;
        if (s == "Z") return KeyCode::Z;

        // 数字キー
        if (s == "0") return KeyCode::Key0;
        if (s == "1") return KeyCode::Key1;
        if (s == "2") return KeyCode::Key2;
        if (s == "3") return KeyCode::Key3;
        if (s == "4") return KeyCode::Key4;
        if (s == "5") return KeyCode::Key5;
        if (s == "6") return KeyCode::Key6;
        if (s == "7") return KeyCode::Key7;
        if (s == "8") return KeyCode::Key8;
        if (s == "9") return KeyCode::Key9;

        // 特殊キー
        if (s == "Space") return KeyCode::Space;
        if (s == "Enter") return KeyCode::Enter;
        if (s == "Escape") return KeyCode::Escape;
        if (s == "Tab") return KeyCode::Tab;
        if (s == "LeftShift") return KeyCode::LeftShift;
        if (s == "RightShift") return KeyCode::RightShift;
        if (s == "LeftCtrl") return KeyCode::LeftCtrl;
        if (s == "RightCtrl") return KeyCode::RightCtrl;
        if (s == "LeftAlt") return KeyCode::LeftAlt;
        if (s == "RightAlt") return KeyCode::RightAlt;

        // 矢印キー
        if (s == "Up") return KeyCode::Up;
        if (s == "Down") return KeyCode::Down;
        if (s == "Left") return KeyCode::Left;
        if (s == "Right") return KeyCode::Right;

        return KeyCode::None;
    }
}
