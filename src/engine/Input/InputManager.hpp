// src/Input/InputManager.hpp
#pragma once

#include <Windows.h>
#include <windowsx.h>
#include <array>
#include <memory>
#include <functional>
#include <unordered_map>
#include <string>
#include "KeyCode.hpp"
#include "MouseState.hpp"


namespace Engine::Input
{
    // 入力イベントのコールバック型定義
    using KeyPressedCallback = std::function<void(KeyCode keyCode)>;
    using KeyReleasedCallback = std::function<void(KeyCode keyCode)>;
    using MouseButtonCallback = std::function<void(MouseButton button, int x, int y)>;
    using MouseMoveCallback = std::function<void(int x, int y, int deltaX, int deltaY)>;
    using MouseWheelCallback = std::function<void(float delta, int x, int y)>;

    // 入力システムの中核クラス
    // キーボードとマウスの入力を統合的に管理
    class InputManager
    {
    public:
        // 最大同時押し可能キー数
        static constexpr size_t MAX_KEYS = 256;

        InputManager();
        ~InputManager();

        // コピー・ムーブ禁止（シングルトン的な使用を想定）
        InputManager(const InputManager&) = delete;
        InputManager& operator=(const InputManager&) = delete;
        InputManager(InputManager&&) = delete;
        InputManager& operator=(InputManager&&) = delete;

        // 初期化と終了処理
        void initialize(HWND windowHandle);
        void shutdown();

        // フレーム更新処理
        void update();

        // キーボード入力関連
        bool isKeyDown(KeyCode keyCode) const;
        bool isKeyPressed(KeyCode keyCode) const;
        bool isKeyReleased(KeyCode keyCode) const;

        // 修飾キーの状態取得
        bool isShiftDown() const;
        bool isCtrlDown() const;
        bool isAltDown() const;

        // マウス入力関連
        const MouseState& getMouseState() const { return m_mouseState; }
        bool isMouseButtonDown(MouseButton button) const;
        bool isMouseButtonPressed(MouseButton button) const;
        bool isMouseButtonReleased(MouseButton button) const;

        // マウス位置取得
        int getMouseX() const { return m_mouseState.x; }
        int getMouseY() const { return m_mouseState.y; }
        int getMouseDeltaX() const { return m_mouseState.deltaX; }
        int getMouseDeltaY() const { return m_mouseState.deltaY; }

        // マウスホイール取得
        float getMouseWheelDelta() const { return m_mouseState.wheelDelta; }

        // マウス制御
        void setMousePosition(int x, int y);
        void showCursor(bool show);
        void captureMouse(bool capture);
        void setRelativeMouseMode(bool relative);

        // イベントコールバック設定
        void setKeyPressedCallback(KeyPressedCallback callback) { m_keyPressedCallback = callback; }
        void setKeyReleasedCallback(KeyReleasedCallback callback) { m_keyReleasedCallback = callback; }
        void setMouseButtonPressedCallback(MouseButtonCallback callback) { m_mouseButtonPressedCallback = callback; }
        void setMouseButtonReleasedCallback(MouseButtonCallback callback) { m_mouseButtonReleasedCallback = callback; }
        void setMouseMoveCallback(MouseMoveCallback callback) { m_mouseMoveCallback = callback; }
        void setMouseWheelCallback(MouseWheelCallback callback) { m_mouseWheelCallback = callback; }

        // Win32メッセージハンドラ（Window.cppから呼び出される）
        bool handleWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

        // デバッグ情報取得
        std::string getDebugInfo() const;

        // 設定
        void setMouseSensitivity(float sensitivity) { m_mouseSensitivity = sensitivity; }
        float getMouseSensitivity() const { return m_mouseSensitivity; }

        // 有効性チェック
        bool isInitialized() const { return m_initialized; }

    private:
        // 内部状態
        bool m_initialized = false;
        HWND m_windowHandle = nullptr;
        float m_mouseSensitivity = 1.0f;

        // キーボード状態
        std::array<bool, MAX_KEYS> m_keyStates = { false };
        std::array<bool, MAX_KEYS> m_prevKeyStates = { false };

        // マウス状態
        MouseState m_mouseState;

        // カーソル制御
        bool m_cursorVisible = true;
        bool m_mouseCaptured = false;

        // 相対マウスモード用
        POINT m_windowCenter = { 0, 0 };
        bool m_relativeMode = false;

        // イベントコールバック
        KeyPressedCallback m_keyPressedCallback;
        KeyReleasedCallback m_keyReleasedCallback;
        MouseButtonCallback m_mouseButtonPressedCallback;
        MouseButtonCallback m_mouseButtonReleasedCallback;
        MouseMoveCallback m_mouseMoveCallback;
        MouseWheelCallback m_mouseWheelCallback;

        // 内部メソッド
        void updateKeyboardState();
        void updateMouseState();
        void resetFrameData();
        void calculateWindowCenter();
        void setRawMouseInput(bool enable);

        // Win32メッセージハンドラ（内部）
        bool handleKeyboardMessage(UINT message, WPARAM wParam, LPARAM lParam);
        bool handleMouseMessage(UINT message, WPARAM wParam, LPARAM lParam);
        bool handleRawInput(LPARAM lParam);

        // ユーティリティメソッド
        bool isValidKeyCode(KeyCode keyCode) const;
        size_t keyCodeToIndex(KeyCode keyCode) const;
        KeyCode virtualKeyToKeyCode(WPARAM vkCode) const;
    };
}
