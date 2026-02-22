#include "BuildSystem.hpp"

#include <Windows.h>
#include <filesystem>
#include <format>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>

namespace Editor::Build
{
    namespace fs = std::filesystem;

    bool BuildSystem::RunCommandWithOutput(
        const std::string& command,
        std::function<void(const std::string&)> onOutput)
    {
        FILE* pipe = _popen(command.c_str(), "r");
        if (!pipe)
            return false;

        char buffer[4096];

        while (fgets(buffer, sizeof(buffer), pipe))
        {
            if (m_cancelled)
            {
                _pclose(pipe);
                return false;
            }

            onOutput(buffer);
        }

        int result = _pclose(pipe);

        return result == 0;
    }

    // =========================================================
    // Returns the directory of the currently running executable.
    // When running as OrionBuildWorker, this is tools/{Config}/.
    // =========================================================
    static fs::path GetWorkerExeDir()
    {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        return fs::path(path).parent_path();
    }

    // =========================================================
    // Returns the source root directory (contains CMakeLists.txt).
    // =========================================================
    static fs::path GetSourceRootDir()
    {
        auto current = fs::current_path();
        while (current.has_parent_path())
        {
            if (fs::exists(current / "CMakeLists.txt"))
                return current;
            current = current.parent_path();
        }
        return {};
    }

    // =========================================================
    // Returns the editor's CMake build directory
    // (the one containing CMakeCache.txt).
    // e.g. build/x64-release-local/
    // =========================================================
    static fs::path GetEditorBuildDir()
    {
        auto current = fs::current_path();
        while (current.has_parent_path())
        {
            if (fs::exists(current / "CMakeCache.txt"))
                return current;
            current = current.parent_path();
        }
        return {};
    }

    // =========================================================
    // Returns the editor's executable output directory.
    // build/<preset>/editor/{Config}/
    // buildDir を明示指定することで fs::current_path() 依存を避ける
    // =========================================================
    static fs::path GetEditorOutputDir(const BuildConfig& cfg, const fs::path& buildDir)
    {
        return buildDir / "editor" / cfg.config;
    }

    // =========================================================
    // Detects the current build configuration from the exe path.
    // =========================================================
    static BuildConfig DetectCurrentBuildConfig()
    {
        auto exeDir = GetWorkerExeDir();
        std::string exePath = exeDir.string();

        BuildConfig config;

        if (exePath.find("x64-release") != std::string::npos)
        {
            config.buildDir = "x64-release";
            config.config = "Release";
        }
        else if (exePath.find("x64-debug") != std::string::npos)
        {
            config.buildDir = "x64-debug";
            config.config = "Debug";
        }
        else
        {
            auto configDir = exeDir.filename().string();
            config.buildDir = (configDir == "Release") ? "x64-release" : "x64-debug";
            config.config = (configDir == "Release") ? "Release" : "Debug";
        }

        std::cout << "[INFO] Detected build config: " << config.buildDir
            << " (" << config.config << ")" << std::endl;
        return config;
    }

    // =========================================================
    // Returns the runtime executable name from ProjectSettings.
    // =========================================================
    static std::string GetRuntimeExeName()
    {
        const auto& name = Engine::Core::ProjectSettings::get().getProjectName();

        // テンプレートのプレースホルダ未置換 or 空の場合
        if (name.empty() || name.find("__") != std::string::npos)
            return "OrionGame";

        return name;
    }

    // =========================================================
    // Resolves the project-templates/3d path.
    // =========================================================
    static fs::path GetProjectTemplatesRoot(const fs::path& editorOutputDir)
    {
        auto& settings = Engine::Core::ProjectSettings::get();
        auto projectRoot = settings.getProjectRootDir();

        if (!projectRoot.empty())
        {
            if (projectRoot.is_absolute() && fs::exists(projectRoot))
                return projectRoot;

            auto resolved = editorOutputDir / projectRoot;
            if (fs::exists(resolved))
                return fs::weakly_canonical(resolved);
        }

        return editorOutputDir / "project-templates" / "3d";
    }

    // =========================================================
    // Read a variable from CMakeCache.txt.
    // Matches lines of the form:  VAR_NAME:TYPE=value
    // =========================================================
    static std::string ReadCMakeCache(const fs::path& buildDir, const std::string& varName)
    {
        fs::path cacheFile = buildDir / "CMakeCache.txt";
        if (!fs::exists(cacheFile))
            return {};

        std::ifstream cache(cacheFile);
        std::string line;
        while (std::getline(cache, line))
        {
            // Match "VARNAME:" or "VARNAME=" at the start
            if (line.rfind(varName, 0) != 0)
                continue;
            if (line.size() <= varName.size())
                continue;
            char sep = line[varName.size()];
            if (sep != ':' && sep != '=')
                continue;

            auto eq = line.find('=');
            if (eq == std::string::npos)
                continue;

            std::string val = line.substr(eq + 1);
            while (!val.empty() && (val.back() == '\r' || val.back() == '\n' || val.back() == ' '))
                val.pop_back();
            return val;
        }
        return {};
    }

    // =========================================================
    // Callbacks
    // =========================================================
    void BuildSystem::setProgressCallback(ProgressCallback callback)
    {
        m_progressCallback = std::move(callback);
    }

    void BuildSystem::updateProgress(BuildStatus status,
        const std::string& message,
        float progress)
    {
        m_currentResult.status = status;
        m_currentResult.message = message;
        m_currentResult.progress = progress;
        m_currentResult.outputPath =
            (GetEditorOutputDir(m_currentConfig, m_cachedEditorBuildDir) / "dist" / m_currentConfig.config).string();

        std::cout << "[BuildSystem] " << message
            << " (Progress: " << (progress * 100.0f) << "%)" << std::endl;

        if (m_progressCallback)
            m_progressCallback(m_currentResult);
    }

    // =========================================================
    // Build entry point
    // =========================================================
    bool BuildSystem::build()
    {
        m_cancelled = false;

        m_currentConfig = DetectCurrentBuildConfig();
        m_cachedEditorBuildDir = GetEditorBuildDir();
        m_cachedSourceRootDir = GetSourceRootDir();

        auto& settings = Engine::Core::ProjectSettings::get();
        settings.loadForEditor();

        // ステップ実行ヘルパー: キャンセルチェック→ステップ実行→失敗時にFailed通知
        // 各サブ関数が自前でupdateProgressしない場合もここでカバーする
        auto runStep = [this](auto stepFn, const std::string& failMsg, float failProgress) -> bool {
            if (m_cancelled) {
                updateProgress(BuildStatus::Failed, "Build cancelled by user", failProgress);
                return false;
            }
            if (!stepFn()) {
                // サブ関数がすでにFailed通知済みの場合も、まだの場合もここで通知する。
                // 二重通知になっても表示上は問題ない。
                if (!m_cancelled) {
                    updateProgress(BuildStatus::Failed, failMsg, failProgress);
                }
                else {
                    updateProgress(BuildStatus::Failed, "Build cancelled by user", failProgress);
                }
                return false;
            }
            return true;
            };

        updateProgress(BuildStatus::Preparing, "Preparing build...", 0.00f);
        if (!runStep([this] { return prepareOutputDirectory(); },
            "Failed to prepare output directory", 0.00f)) return false;

        updateProgress(BuildStatus::Building, "Building Runtime...", 0.20f);
        if (!runStep([this] { return buildRuntimeExecutable(); },
            "Runtime build failed", 0.20f)) return false;

        updateProgress(BuildStatus::Building, "Copying executable...", 0.60f);
        if (!runStep([this] { return copyExecutable(); },
            "Failed to copy executable", 0.60f)) return false;

        updateProgress(BuildStatus::CopyingAssets, "Copying assets...", 0.70f);
        if (!runStep([this] { return copyAssets(); },
            "Failed to copy assets", 0.70f)) return false;

        updateProgress(BuildStatus::CopyingAssets, "Copying ProjectSettings...", 0.80f);
        if (!runStep([this] { return copyProjectSettings(); },
            "Failed to copy ProjectSettings.json", 0.80f)) return false;

        updateProgress(BuildStatus::CopyingAssets, "Copying engine assets...", 0.85f);
        if (!runStep([this] { return copyEngineAssets(); },
            "Failed to copy engine assets", 0.85f)) return false;

        updateProgress(BuildStatus::Success, "Build completed successfully", 1.00f);
        return true;
    }


    bool BuildSystem::cancel()
    {
        if (m_cancelled) return false;
        m_cancelled = true;
        // updateProgress はビルドスレッド側から呼ばれるコールバックを使うため、
        // UIスレッドからここで呼ぶとm_resultMutexのデッドロックを引き起こす。
        // フラグのみ立て、ビルドスレッドのループが次のチェックで
        // Failedステータスをキューに積む。
        return true;
    }

    // =========================================================
    // Prepare the dist/{Config} output directory.
    // =========================================================
    bool BuildSystem::prepareOutputDirectory()
    {
        try
        {
            auto outDir = GetEditorOutputDir(m_currentConfig, m_cachedEditorBuildDir) / "dist" / m_currentConfig.config;
            std::cout << "[DEBUG] Output dir: " << outDir.string() << std::endl;
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

    bool BuildSystem::buildRuntimeExecutable()
    {
        try
        {
            fs::path root = GetSourceRootDir();
            if (root.empty()) return false;

            std::string projectName = GetRuntimeExeName();

            auto cacheProject =
                ReadCMakeCache(m_cachedEditorBuildDir, "RUNTIME_PROJECT_NAME");

            bool needReconfigure = (cacheProject != projectName);

            std::string configurePreset =
                (m_currentConfig.config == "Debug")
                ? "x64-debug"
                : "x64-release-local";

            std::string buildPreset =
                (m_currentConfig.config == "Debug")
                ? "runtime-debug"
                : "runtime-release-local";

            // カレントディレクトリを変更する前に保存し、
            // スコープを抜けたら必ず元に戻す（RAIIガード）
            fs::path prevPath = fs::current_path();
            struct RestorePath {
                fs::path saved;
                ~RestorePath() { try { fs::current_path(saved); } catch (...) {} }
            } restorePath{ prevPath };

            fs::current_path(root);

            if (needReconfigure)
            {
                std::string configureCmd =
                    "cmake --preset " + configurePreset +
                    " -DRUNTIME_PROJECT_NAME=" + projectName;

                if (!RunCommandWithOutput(configureCmd,
                    [this](const std::string& line)
                    {
                        if (m_progressCallback)
                        {
                            BuildResult r;
                            r.status = BuildStatus::Building;
                            r.progress = 0.3f;
                            r.message = line;
                            m_progressCallback(r);
                        }
                    }))
                {
                    updateProgress(BuildStatus::Failed,
                        "CMake reconfigure failed", 0.3f);
                    return false;
                }
            }

            std::string buildCmd =
                "cmake --build --preset " + buildPreset +
                " --target " + projectName;

            bool result = RunCommandWithOutput(buildCmd,
                [this](const std::string& line)
                {
                    if (m_progressCallback)
                    {
                        BuildResult r;
                        r.status = BuildStatus::Building;
                        r.progress = 0.5f;
                        r.message = line;
                        m_progressCallback(r);
                    }
                });

            // restorePathのデストラクタでcurrent_pathが元に戻ってから return
            return result;
        }
        catch (...)
        {
            updateProgress(BuildStatus::Failed, "Runtime build failed (exception)", 0.5f);
            return false;
        }
    }

    // =========================================================
    // Copy the built exe from buildDir/runtime/{Config}/
    // to editor/{Config}/dist/{Config}/.
    // =========================================================
    bool BuildSystem::copyExecutable()
    {
        fs::path buildDir = GetEditorBuildDir();
        fs::path runtimeOut = m_cachedEditorBuildDir / "runtime" / m_currentConfig.config;
        std::string exeName = GetRuntimeExeName() + ".exe";
        fs::path exeSrc = runtimeOut / exeName;

        std::cout << "[DEBUG] Looking for exe at: " << exeSrc.string() << std::endl;

        if (!fs::exists(exeSrc))
        {
            updateProgress(BuildStatus::Failed,
                std::format("Runtime executable not found: {}", exeSrc.string()), 0.6f);
            return false;
        }

        fs::path outDir = GetEditorOutputDir(m_currentConfig, m_cachedEditorBuildDir) / "dist" / m_currentConfig.config;

        try
        {
            fs::create_directories(outDir);
            fs::copy_file(exeSrc, outDir / exeName, fs::copy_options::overwrite_existing);
            std::cout << "[DEBUG] Copied exe to: " << (outDir / exeName).string() << std::endl;
        }
        catch (const std::exception& e)
        {
            updateProgress(BuildStatus::Failed,
                std::format("Failed to copy executable: {}", e.what()), 0.6f);
            return false;
        }

        copyDependencyDLLs(outDir, runtimeOut);
        return true;
    }

    // =========================================================
    // Copy assets.
    // =========================================================
    bool BuildSystem::copyAssets()
    {
        fs::path editorDir = GetEditorOutputDir(m_currentConfig, m_cachedEditorBuildDir);
        auto projectRoot = GetProjectTemplatesRoot(editorDir);
        auto& settings = Engine::Core::ProjectSettings::get();

        fs::path src = projectRoot / settings.getAssetRoot();
        fs::path dst = editorDir / "dist" / m_currentConfig.config / "assets";

        std::cout << "[DEBUG] Assets: " << src.string() << " -> " << dst.string() << std::endl;

        if (!fs::exists(src))
        {
            updateProgress(BuildStatus::Warning,
                std::format("Warning: Asset folder not found: {}", src.string()), 0.70f);
            return true;
        }

        return copyDirectory(src, dst);
    }

    // =========================================================
    // Copy ProjectSettings.json to dist/{Config}/assets/.
    // __PROJECT_NAME__ プレースホルダを実際のプロジェクト名に置換する
    // =========================================================
    bool BuildSystem::copyProjectSettings()
    {
        fs::path editorDir = GetEditorOutputDir(m_currentConfig, m_cachedEditorBuildDir);
        auto projectRoot = GetProjectTemplatesRoot(editorDir);

        fs::path src = projectRoot / "ProjectSettings.json";
        fs::path dst = editorDir / "dist" / m_currentConfig.config / "assets" / "ProjectSettings.json";

        std::cout << "[DEBUG] ProjectSettings: " << src.string() << " -> " << dst.string() << std::endl;

        if (!fs::exists(src))
        {
            updateProgress(BuildStatus::Warning,
                std::format("Warning: ProjectSettings.json not found: {}", src.string()), 0.80f);
            return true;
        }

        try
        {
            // ファイルを文字列として読み込み、プレースホルダを置換してから書き出す
            std::ifstream inFile(src);
            if (!inFile.is_open())
            {
                updateProgress(BuildStatus::Warning,
                    std::format("Warning: Cannot open ProjectSettings.json: {}", src.string()), 0.80f);
                return true;
            }

            std::string content((std::istreambuf_iterator<char>(inFile)),
                std::istreambuf_iterator<char>());
            inFile.close();

            // __PROJECT_NAME__ を実際のプロジェクト名で置換
            std::string projectName = GetRuntimeExeName();
            auto replaceAll = [](std::string& str, const std::string& from, const std::string& to) {
                size_t pos = 0;
                while ((pos = str.find(from, pos)) != std::string::npos) {
                    str.replace(pos, from.length(), to);
                    pos += to.length();
                }
                };

            replaceAll(content, "__PROJECT_NAME__", projectName);
            // __ENGINE_VERSION__ は現状固定値で置換（将来的にCMakeから取得可にする）
            replaceAll(content, "__ENGINE_VERSION__", "0.0.0");

            fs::create_directories(dst.parent_path());
            std::ofstream outFile(dst);
            if (!outFile.is_open())
            {
                updateProgress(BuildStatus::Warning,
                    std::format("Warning: Cannot write ProjectSettings.json to: {}", dst.string()), 0.80f);
                return true;
            }
            outFile << content;
            outFile.close();

            std::cout << "[DEBUG] ProjectSettings written with ProjectName=" << projectName << std::endl;
            return true;
        }
        catch (const std::exception& e)
        {
            updateProgress(BuildStatus::Warning,
                std::format("Warning: Failed to copy ProjectSettings.json: {}", e.what()), 0.80f);
            return true;
        }
    }

    // =========================================================
    // Copy engine-assets.
    // =========================================================
    bool BuildSystem::copyEngineAssets()
    {
        fs::path editorDir = GetEditorOutputDir(m_currentConfig, m_cachedEditorBuildDir);
        auto src = editorDir / "engine-assets";
        auto dst = editorDir / "dist" / m_currentConfig.config / "engine-assets";

        std::cout << "[DEBUG] Engine assets: " << src.string() << std::endl;
        return copyDirectory(src, dst);
    }

    // =========================================================
    // Copy dependency DLLs from sourceDir to outputDir.
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
                    std::cout << "[DEBUG] DLL: " << f.path().filename().string() << std::endl;
                    fs::copy_file(f.path(), outputDir / f.path().filename(),
                        fs::copy_options::overwrite_existing);
                }
            }
        }
        catch (const std::exception& e)
        {
            updateProgress(BuildStatus::Warning,
                std::format("Warning: Failed to copy some DLLs: {}", e.what()), 0.0f);
        }
    }

    // =========================================================
    // Recursively copy a directory.
    // =========================================================
    bool BuildSystem::copyDirectory(
        const fs::path& source,
        const fs::path& dest)
    {
        if (!fs::exists(source))
        {
            std::cout << "[DEBUG] Skipping (not found): " << source.string() << std::endl;
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
                    fs::create_directories(dst);
                else
                    fs::copy_file(e.path(), dst, fs::copy_options::overwrite_existing);
            }
            return true;
        }
        catch (const std::exception& e)
        {
            updateProgress(BuildStatus::Failed,
                std::format("Failed to copy directory: {} - {}", source.string(), e.what()), 0.0f);
            return false;
        }
    }
}