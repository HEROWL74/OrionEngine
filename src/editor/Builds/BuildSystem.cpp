#include "BuildSystem.hpp"
#include <format>
#include <cstdlib>
#include <iostream>
#include <Windows.h>

namespace Editor::Build
{
    void BuildSystem::setProgressCallback(ProgressCallback callback)
    {
        m_progressCallback = std::move(callback);
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

    void BuildSystem::cancel()
    {
        m_cancelled = true;
    }

    bool BuildSystem::prepareOutputDirectory()
    {
        try
        {
            std::filesystem::path root = findProjectRoot();
            std::filesystem::path out = root / "dist" / "OrionGame";

            if (std::filesystem::exists(out))
                std::filesystem::remove_all(out);

            std::filesystem::create_directories(out);
            return true;
        }
        catch (...)
        {
            updateProgress(BuildStatus::Failed, "Failed to prepare output directory", 0.0f);
            return false;
        }
    }

    bool BuildSystem::buildRuntimeExecutable()
    {
        std::filesystem::path root = findProjectRoot();
        std::filesystem::path buildDir = root / "build" / "runtime-release";
        std::filesystem::path runtimeDir = root / "runtime";

        // コンソールウィンドウを開く
        AllocConsole();
        FILE* consoleOut;
        freopen_s(&consoleOut, "CONOUT$", "w", stdout);
        freopen_s(&consoleOut, "CONOUT$", "w", stderr);

        SetConsoleTitle(L"Orion Build Console");

        // 既存のビルドディレクトリを削除
        if (std::filesystem::exists(buildDir))
        {
            updateProgress(BuildStatus::Preparing, "Cleaning build directory...", 0.15f);
            std::filesystem::remove_all(buildDir);
        }

        std::filesystem::create_directories(buildDir);

        // Configure
        std::string configureCmd = std::format(
            "cd /d \"{}\" && cmake -S \"{}\" -B \"{}\" "
            "-G \"Visual Studio 18 2026\" -A x64",
            root.string(),
            runtimeDir.string(),
            buildDir.string()
        );

        if (std::system(configureCmd.c_str()) != 0)
        {
            updateProgress(BuildStatus::Failed, "CMake configure failed", 0.3f);
            return false;
        }

        // Build
        std::string buildCmd = std::format(
            "cd /d \"{}\" && cmake --build \"{}\" --config Release --target OrionGame",
            root.string(),
            buildDir.string()
        );

        if (std::system(buildCmd.c_str()) != 0)
        {
            updateProgress(BuildStatus::Failed, "Runtime build failed", 0.5f);
            return false;
        }

        // ビルド完了後、コンソールに完了メッセージ
        std::cout << "\n=================================\n";
        std::cout << "Build completed successfully!\n";
        std::cout << "Press any key to close...\n";
        std::cout << "=================================\n";

        return true;
    }

    bool BuildSystem::copyExecutable()
    {
        std::filesystem::path root = findProjectRoot();
        std::filesystem::path exe = root / "build" / "runtime-release" / "Release" / "OrionGame.exe";

        if (!std::filesystem::exists(exe))
        {
            updateProgress(BuildStatus::Failed, "OrionGame.exe not found", 0.6f);
            return false;
        }

        std::filesystem::path dst = root / "dist" / "OrionGame" / "OrionGame.exe";

        std::filesystem::copy_file(exe, dst, std::filesystem::copy_options::overwrite_existing);
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

    bool BuildSystem::copyDirectory(const std::filesystem::path& source, const std::filesystem::path& dest)
    {
        if (!std::filesystem::exists(source)) return true;

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
        catch (...)
        {
            updateProgress(BuildStatus::Failed, "Failed to copy directory: " + source.string(), 0.0f);
            return false;
        }
    }

    std::filesystem::path BuildSystem::findProjectRoot()
    {
        auto cur = std::filesystem::current_path();
        while (cur.has_parent_path())
        {
            if (std::filesystem::exists(cur / "CMakeLists.txt"))
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