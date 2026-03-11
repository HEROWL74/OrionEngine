// src/editor/buildworker/BuildSystem.hpp
#pragma once

#include <string>
#include <functional>
#include <filesystem>
#include <fstream>
#include <mutex>
#include "../engine/Utils/Common.hpp"
#include "../engine/Core/ProjectSettings.hpp"

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

    // ビルド構成情報
    //
    // 実際のディレクトリ構造:
    //
    // VS Multi-Config (x64-release-local):
    //   build/x64-release-local/
    //     editor/                        <- editorRootDir
    //       player/                      <- playerDir (ビルド済みexe+DLLキャッシュ)
    //       engine-assets/
    //       project-templates/
    //       logs/                        <- ビルドログ出力先
    //         build_20260224_223012.txt
    //       Release/
    //         OrionEditor.exe
    //     runtime/
    //       Release/
    //         OrionGame.exe
    //
    // Ninja Single-Config (x64-release-ci):
    //   build/x64-release-ci/
    //     editor/
    //       player/
    //       engine-assets/
    //       project-templates/
    //       logs/
    //       OrionEditor.exe
    //     runtime/
    //       OrionGame.exe
    //
    // CMakeCache.txt への依存を完全に排除済み。
    struct BuildConfig
    {
        std::string presetName;
        std::string config;
        bool        isMultiConfig = true;
    };

    class BuildSystem
    {
    public:
        using ProgressCallback = std::function<void(const BuildResult&)>;

        BuildSystem() = default;
        ~BuildSystem();

        void setProgressCallback(ProgressCallback callback);

        // forceRebuild=true: player/があっても cmake --build を強制実行して更新する
        bool build(bool forceRebuild = false);
        bool cancel();
        const BuildResult& getResult() const { return m_currentResult; }

    private:
        bool prepareOutputDirectory();
        bool preparePlayerCache(bool forceRebuild);
        bool copyPlayerExecutable();
        bool copyAssets();
        bool copyProjectSettings();
        bool copyEngineAssets();

        void updateProgress(BuildStatus status, const std::string& message, float progress);
        void copyDependencyDLLs(const std::filesystem::path& outputDir,
            const std::filesystem::path& sourceDir);
        bool copyDirectory(const std::filesystem::path& source,
            const std::filesystem::path& dest);
        bool RunCommandWithOutput(const std::string& command,
            std::function<void(const std::string&)> onOutput);

        // パス解決 (CMakeCache.txt に一切依存しない)
        std::filesystem::path getEditorRootDir()  const;
        std::filesystem::path getPlayerDir()      const;
        std::filesystem::path getRuntimeExeDir()  const;
        std::filesystem::path getDistOutputDir()  const;
        std::filesystem::path getLogDir()         const;

        // ログファイル管理
        void openLogFile();
        void closeLogFile();
        void writeLog(const std::string& level, const std::string& message);

    private:
        BuildResult      m_currentResult{};
        ProgressCallback m_progressCallback = nullptr;
        bool             m_cancelled = false;
        BuildConfig      m_currentConfig{};

        // build() 開始時に確定してキャッシュ
        std::filesystem::path m_cachedEditorRootDir;
        std::filesystem::path m_cachedBuildDir;
        std::filesystem::path m_cachedSourceRootDir;
        std::string           m_cachedProjectName;

        // ログファイル
        // ビルドスレッドと UIスレッドから同時アクセスされる可能性があるため mutex で保護
        std::ofstream m_logFile;
        std::mutex    m_logMutex;
        std::filesystem::path m_logFilePath;
    };

} // namespace Editor::Build

