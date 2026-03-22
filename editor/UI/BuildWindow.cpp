// editor/UI/BuildWindow.cpp

#include "BuildWindow.hpp"
#include <imgui.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>

namespace Editor::UI
{
    BuildWindow::BuildWindow() = default;

    void BuildWindow::initialize()
    {

    }

    void BuildWindow::draw()
    {
        if (!m_isVisible) return;

        // ロック範囲を最小限にしてキューをローカルにスワップし、
        // ロック外で処理することでデッドロックを防ぐ
        std::queue<Build::BuildResult> localQueue;
        {
            std::lock_guard<std::mutex> lock(m_resultMutex);
            std::swap(localQueue, m_resultQueue);
        }

        while (!localQueue.empty())
        {
            auto result = localQueue.front();
            localQueue.pop();

            onBuildProgressUpdate(result);

            if (result.status == Build::BuildStatus::Success ||
                result.status == Build::BuildStatus::Failed)
            {
                m_isBuilding = false;
            }
        }

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
                    // cancel()はm_cancelledフラグを立てるだけ。
                    // ビルドスレッドがFailed結果をキューに積んだ後、
                    // draw()のキュー処理でm_isBuildingがfalseになる。
                    // UIスレッドで即座にfalseにするとスレッドがまだ動いていても
                    // 完了扱いになってしまうのでここでは変更しない。
                    m_buildSystem->cancel();
                    addLogEntry(BuildLogEntry::Type::Warning, "Cancelling build...");
                }
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
        clearLog();
        m_isBuilding = true;
        m_buildProgress = 0.0f;
        m_currentStatus = "Starting build...";

        // shared_ptrで管理することで、スレッドがBuildWindowより
        // 長生きしてもUse-After-Freeにならない
        m_buildSystem = std::make_shared<Build::BuildSystem>();

        m_buildSystem->setProgressCallback(
            [this](const Build::BuildResult& result)
            {
                std::lock_guard<std::mutex> lock(m_resultMutex);
                m_resultQueue.push(result);
            });

        // スレッドにshared_ptrのコピーを渡して所有権を共有する。
        // こうするとBuildWindowが先に破棄されてもスレッドは安全に動作する。
        auto buildSystemCopy = m_buildSystem;
        std::thread([buildSystemCopy]() {
            buildSystemCopy->build();
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

