#pragma once

#include <string>
#include <functional>
#include <filesystem>
#include "engine/Utils/Common.hpp"

namespace Editor::Build
{
    enum class BuildStatus
    {
        Idle,
        Preparing,
        Building,
        CopyingAssets,
        Warning,
        Success,
        Failed
    };

    struct BuildResult
    {
        BuildStatus status = BuildStatus::Idle;
        std::string message;
        float progress = 0.0f;
        std::string outputPath;
    };

    // ビルド設定情報
    struct BuildConfig
    {
        std::string buildDir;    // "x64-debug" or "x64-release"
        std::string config;      // "Debug" or "Release"
    };

    class BuildSystem
    {
    public:
        using ProgressCallback = std::function<void(const BuildResult&)>;

        BuildSystem() = default;

        void setProgressCallback(ProgressCallback callback);
        bool build();
        bool cancel();
        const BuildResult& getResult() const { return m_currentResult; }
    private:
        bool prepareOutputDirectory();
        bool buildRuntimeExecutable();
        bool copyExecutable();
        bool copyAssets();
        bool copyEngineAssets();

        void updateProgress(BuildStatus status, const std::string& message, float progress);
        void copyDependencyDLLs(const std::filesystem::path& outputDir, const std::filesystem::path& sourceDir);
        bool copyDirectory(const std::filesystem::path& source, const std::filesystem::path& dest);

    private:
        BuildResult m_currentResult{};
        ProgressCallback m_progressCallback = nullptr;
        bool m_cancelled = false;
        BuildConfig m_currentConfig{};  // 現在実行中の構成
    };
}