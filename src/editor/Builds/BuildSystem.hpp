#pragma once

#include <string>
#include <functional>
#include <filesystem>

namespace Editor::Build
{
    enum class BuildStatus
    {
        Idle,
        Preparing,
        Building,
        CopyingAssets,
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

    class BuildSystem
    {
    public:
        using ProgressCallback = std::function<void(const BuildResult&)>;

        BuildSystem() = default;

        void setProgressCallback(ProgressCallback callback);
        bool build();
        void cancel();

    private:
        bool prepareOutputDirectory();
        bool buildRuntimeExecutable();
        bool copyExecutable();
        bool copyAssets();
        bool copyEngineAssets();

        void updateProgress(BuildStatus status, const std::string& message, float progress);
        void copyDependencyDLLs(const std::filesystem::path& outputDir, const std::filesystem::path& sourceDir);
        bool copyDirectory(const std::filesystem::path& source, const std::filesystem::path& dest);
        std::filesystem::path findProjectRoot();

    private:
        BuildResult m_currentResult{};
        ProgressCallback m_progressCallback = nullptr;
        bool m_cancelled = false;
    };
}