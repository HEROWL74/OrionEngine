#include "BuildSystem.hpp"
#include <direct.h>
#include <format>
#include <cstdlib>
#include <iostream>
#include <Windows.h>
#include <thread>

namespace Editor::Build
{
    void BuildSystem::setProgressCallback(ProgressCallback callback)
    {
        m_progressCallback = std::move(callback);
    }

    static void DebugPrintCWD(const char* label)
    {
        auto cwd = std::filesystem::current_path().string();
        std::string msg = std::string("[CWD] ") + label + ": " + cwd + "\n";
        OutputDebugStringA(msg.c_str());
        std::cout << msg;
    }

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

    bool BuildSystem::prepareOutputDirectory()
    {
        try
        {
            std::filesystem::path root = findProjectRoot();
            std::filesystem::path out = root / "dist" / "OrionGame";

            if (std::filesystem::exists(out))
                updateProgress(
                    BuildStatus::Preparing,
                    "Using existing build directory (preset)",
                    0.15f
                );

            std::filesystem::create_directories(out);
            return true;
        }
        catch (const std::exception& e)
        {
            updateProgress(BuildStatus::Failed,
                std::format("Failed to prepare output directory: {}", e.what()), 0.0f);
            return false;
        }
    }

    bool BuildSystem::buildRuntimeExecutable()
    {
        auto root = findProjectRoot();
        auto buildDir = root / "build" / "x64-release";

        if (!std::filesystem::exists(buildDir / "CMakeCache.txt"))
        {
            updateProgress(
                BuildStatus::Failed,
                "Release build not configured. Please build once in Visual Studio.",
                0.2f
            );
            return false;
        }

        updateProgress(BuildStatus::Building, "Building Runtime...", 0.3f);

        std::string cmd =
            "cmake --build \"" + buildDir.string() + "\" --target OrionGame";

        int ret = std::system(cmd.c_str());
        return ret == 0;
    }

    bool BuildSystem::copyExecutable()
    {
        std::filesystem::path root = findProjectRoot();

        // まず今の正解パス
        std::filesystem::path exe =
            root / "build" / "x64-release" / "runtime" / "OrionGame.exe";

        // Visual Studio (multi-config) 用フォールバック
        if (!std::filesystem::exists(exe))
        {
            exe = root / "build" / "x64-release" / "runtime" / "Release" / "OrionGame.exe";
        }

        if (!std::filesystem::exists(exe))
        {
            updateProgress(
                BuildStatus::Failed,
                std::format("OrionGame.exe not found at: {}", exe.string()),
                0.6f
            );
            return false;
        }

        std::filesystem::path dst =
            root / "dist" / "OrionGame" / "OrionGame.exe";

        std::filesystem::copy_file(
            exe, dst,
            std::filesystem::copy_options::overwrite_existing
        );

        copyDependencyDLLs(dst.parent_path(), exe.parent_path());
        return true;
    }



    bool BuildSystem::copyAssets()
    {
        std::filesystem::path root = findProjectRoot();

        // エディタの実行ファイルがある場所のassetsをコピー
        std::filesystem::path editorAssetsPath = root / "build" / "x64-debug" / "editor" / "assets";

        if (!std::filesystem::exists(editorAssetsPath))
        {
            updateProgress(BuildStatus::Failed,
                std::format("Editor assets not found: {}", editorAssetsPath.string()), 0.75f);
            return false;
        }

        bool result = copyDirectory(editorAssetsPath, root / "dist" / "OrionGame" / "assets");

        if (result)
        {
            updateProgress(BuildStatus::CopyingAssets,
                std::format("Assets copied from {}", editorAssetsPath.string()), 0.8f);
        }

        return result;
    }

    bool BuildSystem::copyEngineAssets()
    {
        std::filesystem::path root = findProjectRoot();
        return copyDirectory(root / "engine-assets", root / "dist" / "OrionGame" / "engine-assets");
    }

    void BuildSystem::copyDependencyDLLs(const std::filesystem::path& outputDir, const std::filesystem::path& sourceDir)
    {
        if (!std::filesystem::exists(sourceDir)) return;

        try
        {
            for (auto& f : std::filesystem::directory_iterator(sourceDir))
            {
                if (f.path().extension() == ".dll")
                {
                    std::filesystem::copy_file(
                        f.path(),
                        outputDir / f.path().filename(),
                        std::filesystem::copy_options::overwrite_existing
                    );
                }
            }
        }
        catch (const std::exception& e)
        {
            updateProgress(BuildStatus::Warning,
                std::format("Warning: Failed to copy some DLLs: {}", e.what()), 0.0f);
        }
    }

    bool BuildSystem::copyDirectory(const std::filesystem::path& source, const std::filesystem::path& dest)
    {
        if (!std::filesystem::exists(source))
        {
            updateProgress(BuildStatus::Warning,
                std::format("Source directory not found: {}", source.string()), 0.0f);
            return true;
        }

        try
        {
            std::filesystem::create_directories(dest);

            for (auto& e : std::filesystem::recursive_directory_iterator(source))
            {
                auto rel = std::filesystem::relative(e.path(), source);
                auto dst = dest / rel;

                if (e.is_directory())
                    std::filesystem::create_directories(dst);
                else
                    std::filesystem::copy_file(e.path(), dst, std::filesystem::copy_options::overwrite_existing);
            }
            return true;
        }
        catch (const std::exception& e)
        {
            updateProgress(BuildStatus::Failed,
                std::format("Failed to copy directory {}: {}", source.string(), e.what()), 0.0f);
            return false;
        }
    }

    std::filesystem::path BuildSystem::findProjectRoot()
    {
        auto cur = std::filesystem::current_path();
        while (cur.has_parent_path())
        {
            if (std::filesystem::exists(cur / "CMakePresets.json"))
                return cur;
            cur = cur.parent_path();
        }
        return std::filesystem::current_path();
    }

    void BuildSystem::updateProgress(BuildStatus status, const std::string& message, float progress)
    {
        std::filesystem::path root = findProjectRoot();

        m_currentResult.status = status;
        m_currentResult.message = message;
        m_currentResult.progress = progress;
        m_currentResult.outputPath = (root / "dist" / "OrionGame").string();

        if (m_progressCallback)
            m_progressCallback(m_currentResult);
    }
}