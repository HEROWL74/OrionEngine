// engine/Utils/LogDispatcher.hpp
#pragma once
#include <functional>
#include <string>
#include <vector>
#include <mutex>

namespace Engine::Utils
{
    // =========================================================================
    // ログの種別
    // =========================================================================
    enum class LogLevel
    {
        Log,
        Warning,
        Error
    };

    // =========================================================================
    // LogDispatcher
    //
    // Engine::Utils::log_info / log_warning / log_error と
    // Lua の print() から呼ばれ、登録されたリスナーへ転送する。
    //
    // 使い方:
    //   // リスナー登録（EditorApp::initD3D 等で行う）
    //   LogDispatcher::get().addListener([](LogLevel lv, const std::string& msg) {
    //       myConsole->log(lv, msg);
    //   });
    //
    //   // ログ送信（既存の log_info/log_warning/log_error から呼ぶ）
    //   LogDispatcher::get().dispatch(LogLevel::Log, "hello");
    // =========================================================================
    class LogDispatcher
    {
    public:
        using Listener = std::function<void(LogLevel, const std::string&)>;

        static LogDispatcher& get()
        {
            static LogDispatcher instance;
            return instance;
        }

        // リスナーを追加（複数可）
        void addListener(Listener listener)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_listeners.push_back(std::move(listener));
        }

        // 全リスナーへ転送
        void dispatch(LogLevel level, const std::string& message)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (auto& listener : m_listeners)
            {
                listener(level, message);
            }
        }

        // リスナーを全クリア（リサイズ/再初期化時に使用）
        void clearListeners()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_listeners.clear();
        }

    private:
        LogDispatcher() = default;
        std::vector<Listener> m_listeners;
        std::mutex m_mutex;
    };

}