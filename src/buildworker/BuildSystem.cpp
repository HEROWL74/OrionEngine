#include "BuildSystem.hpp"

#include <Windows.h>
#include <filesystem>
#include <format>
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace Editor::Build
{
    namespace fs = std::filesystem;

    // =========================================================
    // 設定
    // =========================================================
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


    static BuildConfig DetectCurrentBuildConfig()
    {
        auto exeDir = GetEditorExeDir();

        // 実行ファイルのパスから構成を推測
        // build/x64-debug/editor/Debug/OrionEditor.exe
        // または
        // build/x64-release/editor/Release/OrionEditor.exe

        std::string exePath = exeDir.string();
        std::cout << "[DEBUG] Editor exe dir: " << exePath << std::endl;

        BuildConfig config;

        // パスに "x64-release" が含まれているか確認
        if (exePath.find("x64-release") != std::string::npos)
        {
            config.buildDir = "x64-release";
            config.config = "Release";
        }
        // パスに "x64-debug" が含まれているか確認
        else if (exePath.find("x64-debug") != std::string::npos)
        {
            config.buildDir = "x64-debug";
            config.config = "Debug";
        }
        // フォールバック: ディレクトリ名から判定
        else
        {
            auto configDir = exeDir.filename().string();
            if (configDir == "Release")
            {
                config.buildDir = "x64-release";
                config.config = "Release";
            }
            else // "Debug" or その他
            {
                config.buildDir = "x64-debug";
                config.config = "Debug";
            }
        }

        std::cout << "[INFO] Detected build config: " << config.buildDir
            << " (" << config.config << ")" << std::endl;

        return config;
    }

    // =========================================================
    // build ディレクトリ取得
    // =========================================================
    static fs::path GetBuildRootFromEditor()
    {
        auto exeDir = GetEditorExeDir();
        std::cout << "[DEBUG] Editor exe dir: " << exeDir.string() << std::endl;

        // build/x64-{debug|release}/editor/{Debug|Release}/OrionEditor.exe の場合
        auto current = exeDir;

        // プロジェクトルートまで遡る
        while (current.has_parent_path())
        {
            // build ディレクトリを探す
            if (fs::exists(current / "build"))
            {
                auto buildRoot = current / "build";
                std::cout << "[DEBUG] Found build root: " << buildRoot.string() << std::endl;
                return buildRoot;
            }

            // CMakeLists.txt があればプロジェクトルート
            if (fs::exists(current / "CMakeLists.txt"))
            {
                auto buildRoot = current / "build";
                std::cout << "[DEBUG] Build root (from project root): " << buildRoot.string() << std::endl;
                return buildRoot;
            }

            current = current.parent_path();
        }

        // フォールバック: 元のロジック
        auto buildRoot = exeDir.parent_path()
            .parent_path()
            .parent_path()
            .parent_path();

        std::cout << "[DEBUG] Build root (fallback): " << buildRoot.string() << std::endl;
        return buildRoot;
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
            (editorDir / "dist" / m_currentConfig.config).string();

        // デバッグ出力
        std::cout << "[BuildSystem] " << message
            << " (Progress: " << (progress * 100.0f) << "%)" << std::endl;

        if (m_progressCallback)
            m_progressCallback(m_currentResult);
    }

    // =========================================================
    // Build entry
    // =========================================================
    bool BuildSystem::build()
    {
        m_cancelled = false;

        // 現在の構成を検出
        m_currentConfig = DetectCurrentBuildConfig();

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
    // dist/{Config} 準備
    // =========================================================
    bool BuildSystem::prepareOutputDirectory()
    {
        try
        {
            auto editorDir = GetEditorExeDir();
            auto outDir = editorDir / "dist" / m_currentConfig.config;

            std::cout << "[DEBUG] Preparing output directory: " << outDir.string() << std::endl;

            fs::create_directories(outDir);
            return true;
        }
        catch (const std::exception& e)
        {
            updateProgress(BuildStatus::Failed,
                std::format("Failed to prepare output directory: {}", e.what()), 0.0f);
            return false;
        }
    }

    // =========================================================
    // Runtime build
    // =========================================================
    bool BuildSystem::buildRuntimeExecutable()
    {
        auto buildRoot = GetBuildRootFromEditor();
        auto buildDir = buildRoot / m_currentConfig.buildDir;

        std::cout << "[DEBUG] Build dir: " << buildDir.string() << std::endl;

        if (!fs::exists(buildDir / "CMakeCache.txt"))
        {
            updateProgress(
                BuildStatus::Failed,
                std::format("{} build not configured. Expected CMakeCache.txt at: {}",
                    m_currentConfig.config,
                    (buildDir / "CMakeCache.txt").string()),
                0.2f
            );
            return false;
        }

        // runtimeディレクトリの存在確認
        auto runtimeDir = buildDir / "runtime";
        if (!fs::exists(runtimeDir))
        {
            updateProgress(
                BuildStatus::Failed,
                std::format("Runtime directory not found: {}", runtimeDir.string()),
                0.2f
            );
            return false;
        }

        std::string cmd = std::format(
            "cmake --build \"{}\" --target {} --config {} 2>&1",
            (buildDir / "runtime").string(),
            RUNTIME_TARGET,
            m_currentConfig.config
        );

        std::cout << "[DEBUG] Executing: " << cmd << std::endl;

        // パイプを使ってエラー出力をキャプチャ
        FILE* pipe = _popen(cmd.c_str(), "r");
        if (!pipe)
        {
            updateProgress(
                BuildStatus::Failed,
                "Failed to execute build command",
                0.2f
            );
            return false;
        }

        std::stringstream output;
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            output << buffer;
            std::cout << buffer; // リアルタイム出力
        }

        int exitCode = _pclose(pipe);

        if (exitCode != 0)
        {
            updateProgress(
                BuildStatus::Failed,
                std::format("Build failed with exit code {}. Output:\n{}",
                    exitCode, output.str()),
                0.2f
            );
            return false;
        }

        std::cout << "[DEBUG] Build succeeded" << std::endl;
        return true;
    }

    // =========================================================
    // Runtime exe コピー
    // =========================================================
    bool BuildSystem::copyExecutable()
    {
        auto editorDir = GetEditorExeDir();
        auto buildRoot = GetBuildRootFromEditor();

        fs::path exe =
            buildRoot / m_currentConfig.buildDir / "runtime" / m_currentConfig.config /
            (std::string(RUNTIME_TARGET) + ".exe");

        std::cout << "[DEBUG] Looking for exe at: " << exe.string() << std::endl;

        if (!fs::exists(exe))
        {
            updateProgress(
                BuildStatus::Failed,
                std::format("Runtime executable not found at: {}", exe.string()),
                0.6f
            );
            return false;
        }

        fs::path dst =
            editorDir / "dist" / m_currentConfig.config / exe.filename();

        std::cout << "[DEBUG] Copying exe to: " << dst.string() << std::endl;

        try
        {
            fs::copy_file(
                exe, dst,
                fs::copy_options::overwrite_existing
            );

            copyDependencyDLLs(dst.parent_path(), exe.parent_path());
            return true;
        }
        catch (const std::exception& e)
        {
            updateProgress(
                BuildStatus::Failed,
                std::format("Failed to copy executable: {}", e.what()),
                0.6f
            );
            return false;
        }
    }

    // =========================================================
    // Assets(Editorで編集したもの)
    // =========================================================
    bool BuildSystem::copyAssets()
    {
        auto editorDir = GetEditorExeDir();
        auto src = editorDir / "assets";
        auto dst = editorDir / "dist" / m_currentConfig.config / "assets";

        std::cout << "[DEBUG] Copying assets from: " << src.string()
            << " to: " << dst.string() << std::endl;

        return copyDirectory(src, dst);
    }

    bool BuildSystem::copyEngineAssets()
    {
        auto editorDir = GetEditorExeDir();
        auto src = editorDir / "engine-assets";
        auto dst = editorDir / "dist" / m_currentConfig.config / "engine-assets";

        std::cout << "[DEBUG] Copying engine assets from: " << src.string()
            << " to: " << dst.string() << std::endl;

        return copyDirectory(src, dst);
    }

    // =========================================================
    // DLL コピー
    // =========================================================
    void BuildSystem::copyDependencyDLLs(
        const fs::path& outputDir,
        const fs::path& sourceDir)
    {
        if (!fs::exists(sourceDir))
        {
            std::cout << "[DEBUG] Source dir for DLLs does not exist: "
                << sourceDir.string() << std::endl;
            return;
        }

        try
        {
            for (auto& f : fs::directory_iterator(sourceDir))
            {
                if (f.path().extension() == ".dll")
                {
                    std::cout << "[DEBUG] Copying DLL: " << f.path().filename().string() << std::endl;

                    fs::copy_file(
                        f.path(),
                        outputDir / f.path().filename(),
                        fs::copy_options::overwrite_existing
                    );
                }
            }
        }
        catch (const std::exception& e)
        {
            updateProgress(
                BuildStatus::Warning,
                std::format("Warning: Failed to copy some DLLs: {}", e.what()),
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
        {
            std::cout << "[DEBUG] Source directory does not exist (skipping): "
                << source.string() << std::endl;
            return true;
        }

        try
        {
            fs::create_directories(dest);

            for (auto& e : fs::recursive_directory_iterator(source))
            {
                auto rel = fs::relative(e.path(), source);
                auto dst = dest / rel;

                if (e.is_directory())
                {
                    fs::create_directories(dst);
                }
                else
                {
                    fs::copy_file(
                        e.path(), dst,
                        fs::copy_options::overwrite_existing
                    );
                }
            }
            return true;
        }
        catch (const std::exception& e)
        {
            updateProgress(
                BuildStatus::Failed,
                std::format("Failed to copy directory: {} - {}", source.string(), e.what()),
                0.0f
            );
            return false;
        }
    }
}