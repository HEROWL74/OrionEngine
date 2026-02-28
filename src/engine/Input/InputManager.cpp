// src/Input/InputManager.cpp
#include "InputManager.hpp"
#include "../Utils/Common.hpp"
#include <format>
#include <algorithm>

namespace Engine::Input
{
    InputManager::InputManager() = default;

    InputManager::~InputManager()
    {
        shutdown();
    }

    void InputManager::initialize(HWND windowHandle)
    {
        if (m_initialized)
        {
            Utils::log_warning("InputManager already initialized");
            return;
        }

        m_windowHandle = windowHandle;
        if (!m_windowHandle)
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "Invalid window handle"));
            return;
        }

        m_keyStates.fill(false);
        m_prevKeyStates.fill(false);
        m_mouseState.reset();

        calculateWindowCenter();

        setRawMouseInput(true);

        updateKeyboardState();
        m_prevKeyStates = m_keyStates;  // 初回はすべて「変化なし」とする

        m_initialized = true;
        Utils::log_info("InputManager initialized successfully");
    }

    void InputManager::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        setRelativeMouseMode(false);
        setRawMouseInput(false);

        if (m_mouseCaptured)
        {
            ReleaseCapture();
            m_mouseCaptured = false;
        }

        showCursor(true);

        m_initialized = false;
        m_windowHandle = nullptr;

        Utils::log_info("InputManager shutdown complete");
    }

    void InputManager::update()
    {
        if (!m_initialized)
        {
            return;
        }

        m_prevKeyStates = m_keyStates;
        m_mouseState.savePreviousState();

        updateKeyboardState();
        updateMouseState();

        resetFrameData();
    }

    void InputManager::updateKeyboardState()
    {
        for (size_t i = 0; i < MAX_KEYS; ++i)
        {
            SHORT keyState = GetAsyncKeyState(static_cast<int>(i));
            m_keyStates[i] = (keyState & 0x8000) != 0;
        }
    }

    bool InputManager::isKeyDown(KeyCode keyCode) const
    {
        if (!isValidKeyCode(keyCode))
        {
            return false;
        }

        size_t index = keyCodeToIndex(keyCode);
        return m_keyStates[index];
    }

    bool InputManager::isKeyPressed(KeyCode keyCode) const
    {
        if (!isValidKeyCode(keyCode))
        {
            return false;
        }

        size_t index = keyCodeToIndex(keyCode);
        return m_keyStates[index] && !m_prevKeyStates[index];
    }

    bool InputManager::isKeyReleased(KeyCode keyCode) const
    {
        if (!isValidKeyCode(keyCode))
        {
            return false;
        }

        size_t index = keyCodeToIndex(keyCode);
        return !m_keyStates[index] && m_prevKeyStates[index];
    }

    bool InputManager::isShiftDown() const
    {
        return isKeyDown(KeyCode::LeftShift) || isKeyDown(KeyCode::RightShift);
    }

    bool InputManager::isCtrlDown() const
    {
        return isKeyDown(KeyCode::LeftCtrl) || isKeyDown(KeyCode::RightCtrl);
    }

    bool InputManager::isAltDown() const
    {
        return isKeyDown(KeyCode::LeftAlt) || isKeyDown(KeyCode::RightAlt);
    }

    bool InputManager::isMouseButtonDown(MouseButton button) const
    {
        return m_mouseState.isButtonDown(button);
    }

    bool InputManager::isMouseButtonPressed(MouseButton button) const
    {
        return m_mouseState.isButtonPressed(button);
    }

    bool InputManager::isMouseButtonReleased(MouseButton button) const
    {
        return m_mouseState.isButtonReleased(button);
    }

    void InputManager::setMousePosition(int x, int y)
    {
        if (!m_initialized)
        {
            return;
        }

        POINT screenPoint = { x, y };
        ClientToScreen(m_windowHandle, &screenPoint);
        SetCursorPos(screenPoint.x, screenPoint.y);
    }

    void InputManager::showCursor(bool show)
    {
        if (m_cursorVisible == show)
        {
            return;
        }

        m_cursorVisible = show;

        if (show)
        {
            int count = ShowCursor(TRUE);
            while (count < 0)
            {
                count = ShowCursor(TRUE);
            }
        }
        else
        {
            int count = ShowCursor(FALSE);
            while (count >= 0)
            {
                count = ShowCursor(FALSE);
            }
        }
    }

    void InputManager::captureMouse(bool capture)
    {
        if (!m_initialized)
        {
            return;
        }

        if (m_mouseCaptured == capture)
        {
            return;
        }

        if (capture)
        {
            SetCapture(m_windowHandle);
            m_mouseCaptured = true;
        }
        else
        {
            ReleaseCapture();
            m_mouseCaptured = false;
        }
    }

    void InputManager::setRelativeMouseMode(bool relative)
    {
        if (!m_initialized)
        {
            Utils::log_warning("setRelativeMouseMode called but InputManager not initialized");
            return;
        }

        if (m_relativeMode == relative)
        {
            return;
        }

        Utils::log_info(std::format("### setRelativeMouseMode: {} -> {}", m_relativeMode, relative));

        m_relativeMode = relative;
        m_mouseState.isRelativeMode = relative;

        if (relative)
        {
            calculateWindowCenter();
            captureMouse(true);
            setRawMouseInput(true);

            // 相対モード開始時に delta をリセット
            m_mouseState.deltaX = 0;
            m_mouseState.deltaY = 0;

            setMousePosition(m_windowCenter.x, m_windowCenter.y);
            showCursor(false);

            Utils::log_info("=== Relative mouse mode ENABLED ===");
        }
        else
        {
            showCursor(true);
            captureMouse(false);

            // 相対モード終了時に delta をリセット
            m_mouseState.deltaX = 0;
            m_mouseState.deltaY = 0;

            Utils::log_info("=== Relative mouse mode DISABLED ===");
        }
    }

    bool InputManager::handleWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (!m_initialized || hWnd != m_windowHandle)
        {
            return false;
        }

        switch (message)
        {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
            return handleKeyboardMessage(message, wParam, lParam);

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_MOUSEMOVE:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
            return handleMouseMessage(message, wParam, lParam);

        case WM_INPUT:
            return handleRawInput(lParam);

        default:
            return false;
        }
    }

    std::string InputManager::getDebugInfo() const
    {
        std::string info = "InputManager Debug Info:\n";
        info += std::format("Initialized: {}\n", m_initialized);
        info += std::format("Mouse Position: ({}, {})\n", m_mouseState.x, m_mouseState.y);
        info += std::format("Mouse Delta: ({}, {})\n", m_mouseState.deltaX, m_mouseState.deltaY);
        info += std::format("Mouse Captured: {}\n", m_mouseCaptured);
        info += std::format("Relative Mode: {}\n", m_relativeMode);
        info += std::format("Cursor Visible: {}\n", m_cursorVisible);

        info += "Pressed Keys: ";
        for (size_t i = 0; i < MAX_KEYS; ++i)
        {
            if (m_keyStates[i])
            {
                info += std::format("{} ", static_cast<int>(i));
            }
        }
        info += "\n";

        return info;
    }

    void InputManager::updateMouseState()
    {
        if (!m_initialized)
        {
            return;
        }

        if (!m_relativeMode)
        {
            POINT cursorPos;
            if (GetCursorPos(&cursorPos))
            {
                ScreenToClient(m_windowHandle, &cursorPos);
                m_mouseState.setPosition(cursorPos.x, cursorPos.y);
            }
        }

        RECT clientRect;
        if (GetClientRect(m_windowHandle, &clientRect))
        {
            m_mouseState.isInWindow = (m_mouseState.x >= clientRect.left &&
                m_mouseState.x < clientRect.right &&
                m_mouseState.y >= clientRect.top &&
                m_mouseState.y < clientRect.bottom);
        }
    }

    void InputManager::resetFrameData()
    {
        m_mouseState.wheelDelta = 0.0f;
        m_mouseState.horizontalWheelDelta = 0.0f;
    }

    void InputManager::calculateWindowCenter()
    {
        if (!m_initialized)
        {
            return;
        }

        RECT clientRect;
        if (GetClientRect(m_windowHandle, &clientRect))
        {
            m_windowCenter.x = (clientRect.right - clientRect.left) / 2;
            m_windowCenter.y = (clientRect.bottom - clientRect.top) / 2;
        }
    }

    void InputManager::setRawMouseInput(bool enable)
    {
        RAWINPUTDEVICE rid;
        rid.usUsagePage = 0x01;
        rid.usUsage = 0x02;

        if (enable)
        {
            rid.dwFlags = RIDEV_INPUTSINK;
            rid.hwndTarget = m_windowHandle;
        }
        else
        {
            rid.dwFlags = RIDEV_REMOVE;
            rid.hwndTarget = nullptr;
        }

        if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
        {
            DWORD error = GetLastError();
            Utils::log_warning(std::format("Failed to register raw input device: {}", error));
        }
    }

    bool InputManager::handleKeyboardMessage(UINT message, WPARAM wParam, LPARAM lParam)
    {
        KeyCode keyCode = virtualKeyToKeyCode(wParam);
        if (keyCode == KeyCode::None)
        {
            return false;
        }

        bool isPressed = (message == WM_KEYDOWN || message == WM_SYSKEYDOWN);
        bool wasPressed = isKeyDown(keyCode);

        size_t index = keyCodeToIndex(keyCode);
        m_keyStates[index] = isPressed;

        if (isPressed && !wasPressed && m_keyPressedCallback)
        {
            m_keyPressedCallback(keyCode);
        }
        else if (!isPressed && wasPressed && m_keyReleasedCallback)
        {
            m_keyReleasedCallback(keyCode);
        }

        return true;
    }

    bool InputManager::handleMouseMessage(UINT message, WPARAM wParam, LPARAM lParam)
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

        switch (message)
        {
        case WM_MOUSEMOVE:
            if (m_mouseMoveCallback)
            {
                m_mouseMoveCallback(x, y, m_mouseState.deltaX, m_mouseState.deltaY);
            }
            return true;

        case WM_MOUSEWHEEL:
        {
            float delta = GET_WHEEL_DELTA_WPARAM(wParam) / 120.0f;
            m_mouseState.setWheelDelta(delta);
            if (m_mouseWheelCallback)
            {
                m_mouseWheelCallback(delta, x, y);
            }
        }
        return true;

        case WM_MOUSEHWHEEL:
        {
            float delta = GET_WHEEL_DELTA_WPARAM(wParam) / 120.0f;
            m_mouseState.setWheelDelta(0.0f, delta);
        }
        return true;

        default:
            MouseButton button = win32ToMouseButton(message, wParam);
            if (button != static_cast<MouseButton>(255))
            {
                bool isPressed = (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN ||
                    message == WM_MBUTTONDOWN || message == WM_XBUTTONDOWN);

                bool wasPressed = m_mouseState.isButtonDown(button);
                m_mouseState.setButtonState(button, isPressed);

                if (isPressed && !wasPressed && m_mouseButtonPressedCallback)
                {
                    m_mouseButtonPressedCallback(button, x, y);
                }
                else if (!isPressed && wasPressed && m_mouseButtonReleasedCallback)
                {
                    m_mouseButtonReleasedCallback(button, x, y);
                }
                return true;
            }
            break;
        }

        return false;
    }

    bool InputManager::handleRawInput(LPARAM lParam)
    {
        UINT dwSize = sizeof(RAWINPUT);
        static RAWINPUT raw;

        UINT result = GetRawInputData(
            reinterpret_cast<HRAWINPUT>(lParam),
            RID_INPUT,
            &raw,
            &dwSize,
            sizeof(RAWINPUTHEADER)
        );

        if (result == static_cast<UINT>(-1))
        {
            return false;
        }

        if (raw.header.dwType == RIM_TYPEMOUSE && m_relativeMode)
        {
            int deltaX = raw.data.mouse.lLastX;
            int deltaY = raw.data.mouse.lLastY;

            // 感度を適用
            float adjustedDeltaX = deltaX * m_mouseSensitivity;
            float adjustedDeltaY = deltaY * m_mouseSensitivity;

            // 相対モードでは delta を累積（上書きではなく加算）
            // これにより複数のRAW INPUT イベントのデルタが失われない
            m_mouseState.deltaX += static_cast<int>(adjustedDeltaX);
            m_mouseState.deltaY += static_cast<int>(adjustedDeltaY);

            // デバッグログ（デルタがある場合のみ）
            if (deltaX != 0 || deltaY != 0)
            {
                Utils::log_info(std::format(
                    "RAW INPUT: Raw Delta=({}, {}), Adjusted Delta=({}, {}), Accumulated=({}, {})",
                    deltaX, deltaY,
                    static_cast<int>(adjustedDeltaX), static_cast<int>(adjustedDeltaY),
                    m_mouseState.deltaX, m_mouseState.deltaY
                ));
            }

            return true;
        }

        return false;
    }


    bool InputManager::isValidKeyCode(KeyCode keyCode) const
    {
        uint32_t code = static_cast<uint32_t>(keyCode);
        return code > 0 && code < MAX_KEYS;
    }

    size_t InputManager::keyCodeToIndex(KeyCode keyCode) const
    {
        return static_cast<size_t>(keyCode);
    }

    KeyCode InputManager::virtualKeyToKeyCode(WPARAM vkCode) const
    {
        if (vkCode >= 0 && vkCode < MAX_KEYS)
        {
            return static_cast<KeyCode>(vkCode);
        }
        return KeyCode::None;
    }
}

