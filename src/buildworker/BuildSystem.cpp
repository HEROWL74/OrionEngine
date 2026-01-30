#include "BuildSystem.hpp"

#include <Windows.h>
#include <filesystem>
#include <format>
#include <cstdlib>
#include <iostream>

namespace Editor::Build
{
    namespace fs = std::filesystem;

    // =========================================================
    // 設定
    // =========================================================
    static constexpr const char* DIST_CONFIG = "Release";
    static constexpr const char* RUNTIME_TARGET = "OrionGame";

    // =========================================================
    // Editor 実行ファイルのディレクトリ
    // =========================================================
    static fs::path GetEditorExeDir()
    {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        return fs::path(path).parent_path();
    }

    // =========================================================
    // build ディレクトリ取得
    // build/x64-release/editor/Release/OrionEditor.exe
    // → build
    // =========================================================
    static fs::path GetBuildRootFromEditor()
    {
        auto exeDir = GetEditorExeDir();
        return exeDir.parent_path()   // editor
            .parent_path()   // x64-release
            .parent_path();  // build
    }

    // =========================================================
    // Progress
    // =========================================================
    void BuildSystem::setProgressCallback(ProgressCallback callback)
    {
        m_progressCallback = std::move(callback);
    }

    void BuildSystem::updateProgress(BuildStatus status,
        const std::string& message,
        float progress)
    {
        auto editorDir = GetEditorExeDir();

        m_currentResult.status = status;
        m_currentResult.message = message;
        m_currentResult.progress = progress;
        m_currentResult.outputPath =
            (editorDir / "dist" / DIST_CONFIG).string();

        if (m_progressCallback)
            m_progressCallback(m_currentResult);
    }

    // =========================================================
    // Build entry
    // =========================================================
    bool BuildSystem::build()
    {
        m_cancelled = false;

        updateProgress(BuildStatus::Preparing, "Preparing build...", 0.0f);
        if (!prepareOutputDirectory()) return false;

        updateProgress(BuildStatus::Building, "Building Runtime...", 0.2f);
        if (!buildRuntimeExecutable()) return false;

        updateProgress(BuildStatus::Building, "Copying executable...", 0.6f);
        if (!copyExecutable()) return false;

        updateProgress(BuildStatus::CopyingAssets, "Copying assets...", 0.75f);
        if (!copyAssets()) return false;

        updateProgress(BuildStatus::CopyingAssets, "Copying engine assets...", 0.9f);
        if (!copyEngineAssets()) return false;

        updateProgress(BuildStatus::Success, "Build completed successfully", 1.0f);
        return true;
    }

    bool BuildSystem::cancel()
    {
        if (m_cancelled)
            return false;

        m_cancelled = true;
        updateProgress(BuildStatus::Failed, "Build cancelled", 0.0f);
        return true;
    }

    // =========================================================
    // dist/Release 準備
    // =========================================================
    bool BuildSystem::prepareOutputDirectory()
    {
        try
        {
            auto editorDir = GetEditorExeDir();
            auto outDir = editorDir / "dist" / DIST_CONFIG;
            fs::create_directories(outDir);
            return true;
        }
        catch (...)
        {
            updateProgress(BuildStatus::Failed,
                "Failed to prepare output directory", 0.0f);
            return false;
        }
    }

    // =========================================================
    // Runtime build
    // =========================================================
    bool BuildSystem::buildRuntimeExecutable()
    {
        auto buildRoot = GetBuildRootFromEditor();
        auto releaseDir = buildRoot / "x64-release";

        if (!fs::exists(releaseDir / "CMakeCache.txt"))
        {
            updateProgress(
                BuildStatus::Failed,
                "Release build not configured.",
                0.2f
            );
            return false;
        }

        std::string cmd =
            std::format(
                "cmake --build \"{}\" --target {} --config {}",
                releaseDir.string(),
                RUNTIME_TARGET,
                DIST_CONFIG
            );

        return std::system(cmd.c_str()) == 0;
    }

    // =========================================================
    // Runtime exe コピー
    // =========================================================
    bool BuildSystem::copyExecutable()
    {
        auto editorDir = GetEditorExeDir();
        auto buildRoot = GetBuildRootFromEditor();

        fs::path exe =
            buildRoot / "x64-release" / "runtime" / DIST_CONFIG /
            (std::string(RUNTIME_TARGET) + ".exe");

        if (!fs::exists(exe))
        {
            updateProgress(
                BuildStatus::Failed,
                std::format("Runtime executable not found: {}", exe.string()),
                0.6f
            );
            return false;
        }

        fs::path dst =
            editorDir / "dist" / DIST_CONFIG / exe.filename();

        fs::copy_file(
            exe, dst,
            fs::copy_options::overwrite_existing
        );

        copyDependencyDLLs(dst.parent_path(), exe.parent_path());
        return true;
    }

    // =========================================================
    // Assets（Editorで編集したもの）
    // =========================================================
    bool BuildSystem::copyAssets()
    {
        auto editorDir = GetEditorExeDir();
        auto src = editorDir / "assets";
        auto dst = editorDir / "dist" / DIST_CONFIG / "assets";
        return copyDirectory(src, dst);
    }

    bool BuildSystem::copyEngineAssets()
    {
        auto editorDir = GetEditorExeDir();
        return copyDirectory(
            editorDir / "engine-assets",
            editorDir / "dist" / DIST_CONFIG / "engine-assets"
        );
    }

    // =========================================================
    // DLL コピー
    // =========================================================
    void BuildSystem::copyDependencyDLLs(
        const fs::path& outputDir,
        const fs::path& sourceDir)
    {
        if (!fs::exists(sourceDir)) return;

        try
        {
            for (auto& f : fs::directory_iterator(sourceDir))
            {
                if (f.path().extension() == ".dll")
                {
                    fs::copy_file(
                        f.path(),
                        outputDir / f.path().filename(),
                        fs::copy_options::overwrite_existing
                    );
                }
            }
        }
        catch (...)
        {
            updateProgress(
                BuildStatus::Warning,
                "Warning: Failed to copy some DLLs",
                0.0f
            );
        }
    }

    // =========================================================
    // ディレクトリ再帰コピー
    // =========================================================
    bool BuildSystem::copyDirectory(
        const fs::path& source,
        const fs::path& dest)
    {
        if (!fs::exists(source))
            return true;

        try
        {
            fs::create_directories(dest);

            for (auto& e : fs::recursive_directory_iterator(source))
            {
                auto rel = fs::relative(e.path(), source);
                auto dst = dest / rel;

                if (e.is_directory())
                    fs::create_directories(dst);
                else
                    fs::copy_file(
                        e.path(), dst,
                        fs::copy_options::overwrite_existing
                    );
            }
            return true;
        }
        catch (...)
        {
            updateProgress(
                BuildStatus::Failed,
                std::format("Failed to copy directory: {}", source.string()),
                0.0f
            );
            return false;
        }
    }
}
