// editor/UI/ConsoleWindow.hpp
#pragma once
#include "ImGuiManager.hpp"
#include "../engine/Utils/LogDispatcher.hpp"
#include <vector>
#include <string>
#include <mutex>
#include <cstdint>

namespace Editor::UI
{
    // =========================================================================
    // コンソールに表示する1件のログエントリ
    // =========================================================================
    struct ConsoleEntry
    {
        Engine::Utils::LogLevel level;
        std::string             message;
        uint64_t                frameIndex = 0; // 将来的にフレーム番号を付けるため

        // 同一メッセージの折りたたみ用
        int repeatCount = 1;
    };

    // =========================================================================
    // ConsoleWindow
    //
    // - Log / Warning / Error の3タブ切り替え表示
    // - 各カテゴリのカウンタをタブに表示（Unity風）
    // - Clear ボタン / Auto-scroll トグル
    // - LogDispatcher::addListener() で自動接続する
    // =========================================================================
    class ConsoleWindow : public ImGuiWindow
    {
    public:
        ConsoleWindow();
        ~ConsoleWindow() = default;

        ConsoleWindow(const ConsoleWindow&) = delete;
        ConsoleWindow& operator=(const ConsoleWindow&) = delete;

        void draw() override;

        // LogDispatcher 経由で呼ばれる（スレッドセーフ）
        void pushEntry(Engine::Utils::LogLevel level, const std::string& message);

        // 全クリア
        void clear();

        // カウンタ取得（ToolbarWindow などで使える）
        int logCount()     const { return m_logCount; }
        int warningCount() const { return m_warningCount; }
        int errorCount()   const { return m_errorCount; }

    private:
        mutable std::mutex          m_mutex;
        std::vector<ConsoleEntry>   m_entries;

        int m_logCount = 0;
        int m_warningCount = 0;
        int m_errorCount = 0;

        bool m_autoScroll = true;
        bool m_scrollToBottom = false;

        // タブフィルター: 各フラグが true のときそのレベルを表示
        bool m_showLog = true;
        bool m_showWarning = true;
        bool m_showError = true;

        // 文字列フィルター
        char m_filterBuf[256]{};

        void drawToolbar();
        void drawEntryList();

        static ImVec4 levelColor(Engine::Utils::LogLevel level);
        static const char* levelLabel(Engine::Utils::LogLevel level);
    };

}