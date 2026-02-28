// src/editor/UI/BuildWindow.hpp
#pragma once

#include "buildworker/BuildSystem.hpp"
#include <vector>
#include <string>
#include <mutex>
#include <queue>
#include <atomic>
#include "../UI/ImGuiManager.hpp"

namespace Editor::UI
{
    struct BuildLogEntry
    {
        enum class Type { Info, Warning, Error, Success };
        Type type;
        std::string message;
        std::string timestamp;
    };

    class BuildWindow
    {
    public:
        BuildWindow();
        ~BuildWindow() = default;

        void initialize();
        void draw();

        void show() { m_isVisible = true; }
        void hide() { m_isVisible = false; }
        bool isVisible() const { return m_isVisible; }

    private:
        void startBuild();
        void drawBuildProgress();
        void clearLog();
        void addLogEntry(BuildLogEntry::Type type, const std::string& message);
        void onBuildProgressUpdate(const Build::BuildResult& result);
        static ImVec4 getLogColor(BuildLogEntry::Type type);

        std::filesystem::path getExecutableDir()
        {
            wchar_t path[MAX_PATH];
            GetModuleFileNameW(nullptr, path, MAX_PATH);
            return std::filesystem::path(path).parent_path();
        }

        bool m_isVisible = false;
        std::shared_ptr<Build::BuildSystem> m_buildSystem;
        Build::BuildResult m_lastResult;
        std::vector<BuildLogEntry> m_buildLog;
        std::atomic<bool> m_isBuilding = false;
        float m_buildProgress = 0.0f;
        std::string m_currentStatus;

        std::mutex m_resultMutex;
        std::queue<Build::BuildResult> m_resultQueue;
    };
}