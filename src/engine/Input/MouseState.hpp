// src/Input/MouseState.hpp
#pragma once

#include <Windows.h>
#include <cstdint>

namespace Engine::Input
{
    // マウスボタンの状態を表す列挙型
    enum class MouseButton : uint8_t
    {
        Left = 0,
        Right = 1,
        Middle = 2,
        X1 = 3,     // サイドボタン1
        X2 = 4,     // サイドボタン2
        Count = 5   // ボタン数
    };

    // マウスの現在状態を保持する構造体
    struct MouseState
    {
        // 現在の画面座標位置
        int x = 0;
        int y = 0;

        // 前フレームからの移動量
        int deltaX = 0;
        int deltaY = 0;

        // ホイールの回転量（垂直）
        float wheelDelta = 0.0f;

        // 水平ホイールの回転量（対応マウスのみ）
        float horizontalWheelDelta = 0.0f;

        // 各ボタンの押下状態
        bool buttonStates[static_cast<size_t>(MouseButton::Count)] = { false };

        // 前フレームの各ボタンの押下状態
        bool prevButtonStates[static_cast<size_t>(MouseButton::Count)] = { false };

        // マウスがウィンドウ内にあるかどうか
        bool isInWindow = false;

        // マウスキャプチャ中かどうか
        bool isCaptured = false;

        // マウスが相対モード（FPSゲーム用）かどうか
        bool isRelativeMode = false;

        // 初回更新フラグ（delta計算用）
        bool firstUpdate = true;

        // デフォルトコンストラクタ
        MouseState() = default;

        // コピーコンストラクタとコピー代入演算子
        MouseState(const MouseState& other) = default;
        MouseState& operator=(const MouseState& other) = default;

        // ムーブコンストラクタとムーブ代入演算子
        MouseState(MouseState&& other) noexcept = default;
        MouseState& operator=(MouseState&& other) noexcept = default;

        // 指定したボタンが押されているかチェック
        bool isButtonDown(MouseButton button) const
        {
            size_t index = static_cast<size_t>(button);
            return index < static_cast<size_t>(MouseButton::Count) && buttonStates[index];
        }

        // 指定したボタンが今フレームで押されたかチェック
        bool isButtonPressed(MouseButton button) const
        {
            size_t index = static_cast<size_t>(button);
            return index < static_cast<size_t>(MouseButton::Count) &&
                buttonStates[index] && !prevButtonStates[index];
        }

        // 指定したボタンが今フレームで離されたかチェック
        bool isButtonReleased(MouseButton button) const
        {
            size_t index = static_cast<size_t>(button);
            return index < static_cast<size_t>(MouseButton::Count) &&
                !buttonStates[index] && prevButtonStates[index];
        }

        // 前フレームの状態を保存
        void savePreviousState()
        {
            for (size_t i = 0; i < static_cast<size_t>(MouseButton::Count); ++i)
            {
                prevButtonStates[i] = buttonStates[i];
            }
        }

        // ボタン状態を設定
        void setButtonState(MouseButton button, bool pressed)
        {
            size_t index = static_cast<size_t>(button);
            if (index < static_cast<size_t>(MouseButton::Count))
            {
                buttonStates[index] = pressed;
            }
        }

        // 位置を設定してデルタを計算
        void setPosition(int newX, int newY)
        {
            if (firstUpdate)
            {
                deltaX = 0;
                deltaY = 0;
                firstUpdate = false;
            }
            else
            {
                deltaX = newX - x;
                deltaY = newY - y;
            }

            x = newX;
            y = newY;
        }

        // ホイール値を設定
        void setWheelDelta(float vertical, float horizontal = 0.0f)
        {
            wheelDelta = vertical;
            horizontalWheelDelta = horizontal;
        }

        // 状態をリセット
        void reset()
        {
            deltaX = 0;
            deltaY = 0;
            wheelDelta = 0.0f;
            horizontalWheelDelta = 0.0f;

            for (size_t i = 0; i < static_cast<size_t>(MouseButton::Count); ++i)
            {
                buttonStates[i] = false;
                prevButtonStates[i] = false;
            }

            isInWindow = false;
            isCaptured = false;
            isRelativeMode = false;
            firstUpdate = true;
        }
    };

    // マウスボタンを文字列に変換（デバッグ用）
    inline const char* mouseButtonToString(MouseButton button)
    {
        switch (button)
        {
        case MouseButton::Left: return "Left";
        case MouseButton::Right: return "Right";
        case MouseButton::Middle: return "Middle";
        case MouseButton::X1: return "X1";
        case MouseButton::X2: return "X2";
        default: return "Unknown";
        }
    }

    // Win32のマウスボタンメッセージをMouseButtonに変換
    inline MouseButton win32ToMouseButton(UINT message, WPARAM wParam = 0)
    {
        switch (message)
        {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            return MouseButton::Left;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            return MouseButton::Right;
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            return MouseButton::Middle;
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
            // wParamのHIWORDでX1/X2を判定
            return (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? MouseButton::X1 : MouseButton::X2;
        default:
            return static_cast<MouseButton>(255);
        }
    }
}
