// src/editor/UI/BuildWindow.cpp

#include "BuildWindow.hpp"
#include "imgui.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>

namespace Editor::UI
{
    BuildWindow::BuildWindow() = default;

    void BuildWindow::initialize(Build::BuildSystem* buildSystem)
    {
        m_buildSystem = buildSystem;

        if (m_buildSystem)
        {
            m_buildSystem->setProgressCallback(
                [this](const Build::BuildResult& result)
                {
                    onBuildProgressUpdate(result);
                }
            );
        }
    }

    void BuildWindow::draw()
    {
        if (!m_isVisible) return;

        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Build Project", &m_isVisible))
        {
            drawBuildProgress();
            ImGui::Separator();

            ImGui::BeginChild("BuildLog", ImVec2(0, -40), true);
            for (const auto& entry : m_buildLog)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, getLogColor(entry.type));
                ImGui::TextWrapped("[%s] %s", entry.timestamp.c_str(), entry.message.c_str());
                ImGui::PopStyleColor();
            }
            ImGui::EndChild();

            ImGui::Separator();

            ImGui::BeginDisabled(m_isBuilding);
            if (ImGui::Button("Build", ImVec2(120, 30)))
                startBuild();
            ImGui::EndDisabled();

            ImGui::SameLine();

            ImGui::BeginDisabled(!m_isBuilding);
            if (ImGui::Button("Cancel", ImVec2(120, 30)))
            {
                if (m_buildSystem)
                {
                    m_buildSystem->cancel();
                    addLogEntry(BuildLogEntry::Type::Warning, "Build cancelled by user");
                }
                m_isBuilding = false;
            }
            ImGui::EndDisabled();

            ImGui::SameLine();
            if (ImGui::Button("Clear Log", ImVec2(120, 30)))
                clearLog();
        }
        ImGui::End();
    }

    void BuildWindow::startBuild()
    {
        if (!m_buildSystem)
        {
            addLogEntry(BuildLogEntry::Type::Error, "BuildSystem not initialized");
            return;
        }

        clearLog();
        m_isBuilding = true;
        m_buildProgress = 0.0f;
        m_currentStatus = "Starting build...";

        addLogEntry(BuildLogEntry::Type::Info, "Build started");

        std::thread([this]()
            {
                bool result = m_buildSystem->build();
                m_isBuilding = false;

                if (result)
                    addLogEntry(BuildLogEntry::Type::Success, "Build finished successfully");
                else
                    addLogEntry(BuildLogEntry::Type::Error, "Build failed");

            }).detach();
    }

    void BuildWindow::drawBuildProgress()
    {
        if (m_isBuilding)
        {
            ImGui::ProgressBar(m_buildProgress, ImVec2(-1, 0), m_currentStatus.c_str());
        }
        else if (m_lastResult.status == Build::BuildStatus::Success)
        {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Build completed!");
            ImGui::Text("Output: %s", m_lastResult.outputPath.c_str());
        }
        else if (m_lastResult.status == Build::BuildStatus::Failed)
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Build failed");
            ImGui::TextWrapped("%s", m_lastResult.message.c_str());
        }
    }

    void BuildWindow::onBuildProgressUpdate(const Build::BuildResult& result)
    {
        m_lastResult = result;
        m_buildProgress = result.progress;
        m_currentStatus = result.message;

        addLogEntry(
            result.status == Build::BuildStatus::Failed
            ? BuildLogEntry::Type::Error
            : BuildLogEntry::Type::Info,
            result.message
        );
    }

    void BuildWindow::clearLog()
    {
        m_buildLog.clear();
    }

    void BuildWindow::addLogEntry(BuildLogEntry::Type type, const std::string& message)
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        std::tm tm{};
        localtime_s(&tm, &time);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%H:%M:%S");

        m_buildLog.push_back({ type, message, oss.str() });
    }

    ImVec4 BuildWindow::getLogColor(BuildLogEntry::Type type)
    {
        switch (type)
        {
        case BuildLogEntry::Type::Warning: return { 1, 1, 0, 1 };
        case BuildLogEntry::Type::Error:   return { 1, 0, 0, 1 };
        case BuildLogEntry::Type::Success: return { 0, 1, 0, 1 };
        default:                           return { 0.8f, 0.8f, 0.8f, 1 };
        }
    }
}