// src/Core/Window.hpp
#pragma once

#include <Windows.h>
#include <string>
#include <functional>
#include <memory>
#include "../Utils/Common.hpp"
#include "../Input/InputManager.hpp"
namespace Engine::UI {
    class ImGuiManager;
}

namespace Engine::Core {
    // =============================================================================
   // ウィンドウ設定構造体
   // =============================================================================

    // ウィンドウの作成設定
    struct WindowSettings
    {
        std::wstring title = L"Game Engine"; // ウィンドウタイトル
        int width = 1280;// 幅
        int height = 720;// 高さ
        int x = CW_USEDEFAULT; // x座標
        int y = CW_USEDEFAULT;// y座標
        bool resizable = true; // リサイズできるかできないか
        bool fullScreen = false; // フルスクリーンかそうではないか
    };

    // =============================================================================
 // ウィンドウイベントのコールバック型
 // =============================================================================

    // ウィンドウがリサイズされた時のコールバック
    using ResizeCallback = std::function<void(int width, int height)>;

    // ウィンドウが閉じられる時のコールバック
    using CloseCallback = std::function<void()>;

    // =============================================================================
   // Windowクラス
   // =============================================================================

    // ウィンドウの作成と管理を実装するクラス
    class Window
    {
    public:
        // コンストラクタ
        Window() = default;

        // デストラクタ
        ~Window();

        // コピー禁止
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        //  ムーブはできるようにする
        Window(Window&&) noexcept;
        Window& operator=(Window&&) noexcept;

        // ウィンドウを作成
        // アプリケーションインスタンス
        // ウィンドウ設定
        // 成功時はvoid、失敗時はError
        [[nodiscard]] Utils::VoidResult create(HINSTANCE hInstance, const WindowSettings& settings);

        // ウィンドウを表示
        // nCmdShow表示
        void show(int nCmdShow) const noexcept;

        // ウィンドウメッセージを処理
        // ウィンドウハンドル
        [[nodiscard]] bool processMessages() const noexcept;

        // ウィンドウハンドルを取得
        // ウィンドウハンドル
        [[nodiscard]] HWND getHandle() const noexcept { return m_handle; }

        [[nodiscard]] std::pair<int, int> getClientSize() const noexcept;

        [[nodiscard]] bool isValid() const noexcept { return m_handle != nullptr; }

        void setTitle(std::wstring_view title)const noexcept;

        void setResizeCallback(ResizeCallback callback) noexcept { m_resizeCallback = std::move(callback); }

        void setCloseCallback(CloseCallback callback) noexcept { m_closeCallback = std::move(callback); }

        void setImGuiManager(UI::ImGuiManager* manager) { m_imguiManager = manager; }

        // 入力システムへのアクセス
        [[nodiscard]] Input::InputManager* getInputManager() const noexcept;

    private:
        HWND m_handle = nullptr;
        std::wstring m_className;
        HINSTANCE m_hInstance = nullptr;

        // 入力システム
        std::unique_ptr<Input::InputManager> m_inputManager;
        UI::ImGuiManager* m_imguiManager = nullptr;

        // イベントコールバック
        ResizeCallback m_resizeCallback;
        CloseCallback m_closeCallback;

        [[nodiscard]] Utils::VoidResult registerWindowClass(HINSTANCE hInstance);

        static LRESULT CALLBACK staticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        LRESULT windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        void destroy() noexcept;

    };
}

