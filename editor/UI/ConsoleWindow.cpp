// editor/UI/ConsoleWindow.cpp
#include "ConsoleWindow.hpp"
#include <imgui.h>
#include <format>
#include <algorithm>

namespace Editor::UI
{
    // -------------------------------------------------------------------------
    ConsoleWindow::ConsoleWindow()
        : ImGuiWindow("Console", true)
    {
        // LogDispatcher にリスナーを登録する。
        Engine::Utils::LogDispatcher::get().addListener(
            [this](Engine::Utils::LogLevel level, const std::string& msg)
            {
                pushEntry(level, msg);
            }
        );
    }

    // -------------------------------------------------------------------------
    void ConsoleWindow::pushEntry(Engine::Utils::LogLevel level, const std::string& message)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // 直前と同じメッセージならカウントアップするだけ（折りたたみ）
        if (!m_entries.empty() &&
            m_entries.back().level == level &&
            m_entries.back().message == message)
        {
            m_entries.back().repeatCount++;
        }
        else
        {
            ConsoleEntry entry;
            entry.level = level;
            entry.message = message;
            m_entries.push_back(entry);
        }

        switch (level)
        {
        case Engine::Utils::LogLevel::Log:     m_logCount++;     break;
        case Engine::Utils::LogLevel::Warning: m_warningCount++; break;
        case Engine::Utils::LogLevel::Error:   m_errorCount++;   break;
        }

        if (m_autoScroll) m_scrollToBottom = true;
    }

    // -------------------------------------------------------------------------
    void ConsoleWindow::clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_entries.clear();
        m_logCount = m_warningCount = m_errorCount = 0;
    }

    // -------------------------------------------------------------------------
    void ConsoleWindow::draw()
    {
        if (!m_visible) return;

        if (ImGui::Begin(m_title.c_str(), &m_visible))
        {
            drawToolbar();
            ImGui::Separator();
            drawEntryList();
        }
        ImGui::End();
    }

    // -------------------------------------------------------------------------
    void ConsoleWindow::drawToolbar()
    {
        // Clear ボタン
        if (ImGui::SmallButton("Clear"))
        {
            clear();
        }

        ImGui::SameLine();

        // Auto-scroll トグル
        ImGui::Checkbox("Auto-scroll", &m_autoScroll);

        ImGui::SameLine(0, 20);

        // --- フィルタートグル（Unity 風: アイコン + カウント）---

        // Log
        {
            ImVec4 col = m_showLog
                ? levelColor(Engine::Utils::LogLevel::Log)
                : ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, col);
            std::string label = std::format("[Log] {}", m_logCount);
            if (ImGui::Selectable(label.c_str(), m_showLog,
                ImGuiSelectableFlags_None, ImVec2(80, 0)))
            {
                m_showLog = !m_showLog;
            }
            ImGui::PopStyleColor();
        }

        ImGui::SameLine();

        // Warning
        {
            ImVec4 col = m_showWarning
                ? levelColor(Engine::Utils::LogLevel::Warning)
                : ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, col);
            std::string label = std::format("[Warn] {}", m_warningCount);
            if (ImGui::Selectable(label.c_str(), m_showWarning,
                ImGuiSelectableFlags_None, ImVec2(80, 0)))
            {
                m_showWarning = !m_showWarning;
            }
            ImGui::PopStyleColor();
        }

        ImGui::SameLine();

        // Error
        {
            ImVec4 col = m_showError
                ? levelColor(Engine::Utils::LogLevel::Error)
                : ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, col);
            std::string label = std::format("[Err] {}", m_errorCount);
            if (ImGui::Selectable(label.c_str(), m_showError,
                ImGuiSelectableFlags_None, ImVec2(80, 0)))
            {
                m_showError = !m_showError;
            }
            ImGui::PopStyleColor();
        }

        ImGui::SameLine(0, 20);

        // 文字列フィルター
        ImGui::SetNextItemWidth(200);
        ImGui::InputText("Filter", m_filterBuf, sizeof(m_filterBuf));
    }

    // -------------------------------------------------------------------------
    void ConsoleWindow::drawEntryList()
    {
        if (!ImGui::BeginChild("ConsoleScrollArea", ImVec2(0, 0), false,
            ImGuiWindowFlags_HorizontalScrollbar))
        {
            ImGui::EndChild();
            return;
        }

        // スナップショットをロックせずに描画できるようコピー
        std::vector<ConsoleEntry> snapshot;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            snapshot = m_entries;
        }

        std::string filterStr = m_filterBuf;
        // 小文字に統一してフィルタリング
        std::string lowerFilter = filterStr;
        std::transform(lowerFilter.begin(), lowerFilter.end(),
            lowerFilter.begin(), ::tolower);

        for (const auto& entry : snapshot)
        {
            // レベルフィルター
            if (entry.level == Engine::Utils::LogLevel::Log && !m_showLog)     continue;
            if (entry.level == Engine::Utils::LogLevel::Warning && !m_showWarning) continue;
            if (entry.level == Engine::Utils::LogLevel::Error && !m_showError)   continue;

            // 文字列フィルター
            if (!lowerFilter.empty())
            {
                std::string lowerMsg = entry.message;
                std::transform(lowerMsg.begin(), lowerMsg.end(),
                    lowerMsg.begin(), ::tolower);
                if (lowerMsg.find(lowerFilter) == std::string::npos) continue;
            }

            // 色と接頭辞
            ImGui::PushStyleColor(ImGuiCol_Text, levelColor(entry.level));

            std::string prefix = levelLabel(entry.level);
            std::string displayText = entry.repeatCount > 1
                ? std::format("{} [x{}] {}", prefix, entry.repeatCount, entry.message)
                : std::format("{} {}", prefix, entry.message);

            // 長いメッセージは折り返す
            ImGui::TextWrapped("%s", displayText.c_str());

            ImGui::PopStyleColor();
        }

        // Auto-scroll
        if (m_scrollToBottom && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 4.0f)
        {
            ImGui::SetScrollHereY(1.0f);
            m_scrollToBottom = false;
        }
        else if (m_scrollToBottom)
        {
            ImGui::SetScrollHereY(1.0f);
            m_scrollToBottom = false;
        }

        ImGui::EndChild();
    }

    // -------------------------------------------------------------------------
    ImVec4 ConsoleWindow::levelColor(Engine::Utils::LogLevel level)
    {
        switch (level)
        {
        case Engine::Utils::LogLevel::Log:     return ImVec4(0.85f, 0.85f, 0.85f, 1.0f); // 白
        case Engine::Utils::LogLevel::Warning: return ImVec4(1.0f, 0.85f, 0.0f, 1.0f); // 黄
        case Engine::Utils::LogLevel::Error:   return ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // 赤
        default:                               return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    const char* ConsoleWindow::levelLabel(Engine::Utils::LogLevel level)
    {
        switch (level)
        {
        case Engine::Utils::LogLevel::Log:     return "[Log]";
        case Engine::Utils::LogLevel::Warning: return "[Warn]";
        case Engine::Utils::LogLevel::Error:   return "[Err]";
        default:                               return "[?]";
        }
    }

}