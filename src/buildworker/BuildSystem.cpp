// src/editor/buildworker/BuildSystem.cpp
#include "BuildSystem.hpp"

#include <Windows.h>
#include <filesystem>
#include <format>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace Editor::Build
{
    namespace fs = std::filesystem;

    // =========================================================
    // 内部ユーティリティ
    // =========================================================

    static std::string NowTimestamp(const char* fmt)
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_s(&tm, &time);
        std::ostringstream oss;
        oss << std::put_time(&tm, fmt);
        return oss.str();
    }

    static fs::path GetWorkerExeDir()
    {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        return fs::path(path).parent_path();
    }

    // exeDir から上へ辿り "editor" フォルダを返す。
    // CMakeCache.txt に依存しない。
    //
    // 探索順:
    //   1. exeDir から上へ辿って "editor" という名前のフォルダを探す（開発ビルド向け）
    //   2. exeDir 自体に player/ か engine-assets/ があればそこをルートとする
    //      （リリース配布: exe と同階層にリソースを置いた場合）
    //   3. exeDir の1つ上（VS Multi-Config の Release/ / Debug/ サブフォルダ対応）
    static fs::path FindEditorRootDir()
    {
        auto exeDir = GetWorkerExeDir();

        // 1. 上へ辿って "editor" フォルダを探す
        auto search = exeDir;
        while (search.has_parent_path() && search != search.parent_path())
        {
            if (search.filename() == "editor")
                return search;
            search = search.parent_path();
        }

        // 2. exeDir 直下にリソースフォルダがあればここがルート
        if (fs::exists(exeDir / "player") || fs::exists(exeDir / "engine-assets"))
            return exeDir;

        // 3. VS Multi-Config 配布: exe は Release/ 等のサブフォルダにある場合
        auto parent = exeDir.parent_path();
        if (fs::exists(parent / "player") || fs::exists(parent / "engine-assets"))
            return parent;

        return {};
    }

    // exeDir から上へ辿り CMakeLists.txt があるディレクトリを返す。
    // CMakeCache.txt ではなく CMakeLists.txt を使用。
    static fs::path FindSourceRootDir()
    {
        auto current = GetWorkerExeDir();
        while (current.has_parent_path() && current != current.parent_path())
        {
            if (fs::exists(current / "CMakeLists.txt"))
                return current;
            current = current.parent_path();
        }
        return {};
    }

    // CMakeCache.txt が存在すれば読み取る（存在しない場合は空文字を返す）。
    // 存在しないこと自体はエラーではない。
    static std::string ReadCMakeCacheIfExists(const fs::path& buildDir,
        const std::string& varName)
    {
        fs::path cacheFile = buildDir / "CMakeCache.txt";
        if (!fs::exists(cacheFile)) return {};

        std::ifstream cache(cacheFile);
        std::string   line;
        while (std::getline(cache, line))
        {
            if (line.rfind(varName, 0) != 0) continue;
            if (line.size() <= varName.size()) continue;
            char sep = line[varName.size()];
            if (sep != ':' && sep != '=') continue;
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string val = line.substr(eq + 1);
            while (!val.empty() &&
                (val.back() == '\r' || val.back() == '\n' || val.back() == ' '))
                val.pop_back();
            return val;
        }
        return {};
    }

    // VS/Ninja をフォルダ構造から判定する。CMakeCache.txt 不使用。
    static BuildConfig DetectBuildConfig(const fs::path& editorRootDir)
    {
        auto exeDir = GetWorkerExeDir();
        auto exePath = exeDir.string();
        BuildConfig cfg;

        // パス文字列によるプリセット判定（最優先）
        if (exePath.find("x64-release-ci") != std::string::npos)
        {
            cfg.presetName = "x64-release-ci"; cfg.config = "Release"; cfg.isMultiConfig = false;
            return cfg;
        }
        if (exePath.find("x64-release-local") != std::string::npos)
        {
            cfg.presetName = "x64-release-local"; cfg.config = "Release"; cfg.isMultiConfig = true;
            return cfg;
        }
        if (exePath.find("x64-debug") != std::string::npos)
        {
            cfg.presetName = "x64-debug"; cfg.config = "Debug"; cfg.isMultiConfig = true;
            return cfg;
        }

        // フォルダ構造による VS/Ninja 判別
        // VS (Multi-Config): editor/Release/ または editor/Debug/ が存在する
        // Ninja (Single-Config): editor/ 直下に exe がある
        bool hasReleaseSubdir = fs::exists(editorRootDir / "Release");
        bool hasDebugSubdir = fs::exists(editorRootDir / "Debug");

        if (hasReleaseSubdir || hasDebugSubdir)
        {
            cfg.isMultiConfig = true;
            cfg.config = hasDebugSubdir ? "Debug" : "Release";
            cfg.presetName = (cfg.config == "Debug") ? "x64-debug" : "x64-release-local";
        }
        else
        {
            cfg.isMultiConfig = false;
            cfg.config = "Release";
            cfg.presetName = "x64-release-ci";
        }
        return cfg;
    }

    static std::string GetProjectName()
    {
        const auto& name = Engine::Core::ProjectSettings::get().getProjectName();
        if (name.empty() || name.find("__") != std::string::npos)
            return "OrionGame";
        return name;
    }

    // ProjectSettings が保持する絶対パスをそのまま返す。
    // loadForEditor() が exeDir ベースで解決済みなので追加探索は不要。
    static fs::path GetProjectRoot()
    {
        auto& settings = Engine::Core::ProjectSettings::get();
        auto projectRoot = settings.getProjectRootDir();
        if (!projectRoot.empty() && fs::exists(projectRoot))
            return projectRoot;

        // フォールバック: editorRootDir / project
        wchar_t exePath[MAX_PATH]{};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        auto current = fs::path(exePath).parent_path();

        while (current.has_parent_path() && current != current.parent_path())
        {
            auto candidate = current / "project";
            if (fs::exists(candidate / "ProjectSettings.json"))
                return fs::weakly_canonical(candidate);
            current = current.parent_path();
        }

        return {};
    }

    static void ReplaceAll(std::string& str,
        const std::string& from,
        const std::string& to)
    {
        size_t pos = 0;
        while ((pos = str.find(from, pos)) != std::string::npos)
        {
            str.replace(pos, from.length(), to);
            pos += to.length();
        }
    }

    // =========================================================
    // デストラクタ
    // =========================================================
    BuildSystem::~BuildSystem()
    {
        closeLogFile();
    }

    // =========================================================
    // ログファイル管理
    // =========================================================

    // ログ出力先: editor/logs/build_{YYYYMMDD_HHMMSS}.txt
    fs::path BuildSystem::getLogDir() const
    {
        return m_cachedEditorRootDir / "logs";
    }

    void BuildSystem::openLogFile()
    {
        std::lock_guard<std::mutex> lock(m_logMutex);

        try
        {
            fs::create_directories(getLogDir());
            std::string timestamp = NowTimestamp("%Y%m%d_%H%M%S");
            m_logFilePath = getLogDir() / std::format("build_{}.txt", timestamp);
            m_logFile.open(m_logFilePath, std::ios::out | std::ios::trunc);
            if (!m_logFile.is_open())
            {
                std::cerr << "[BuildSystem] WARNING: Could not open log file: "
                    << m_logFilePath.string() << std::endl;
                return;
            }

            // ヘッダ書き込み
            m_logFile << "========================================\n";
            m_logFile << "  Orion Engine Build Log\n";
            m_logFile << "  " << NowTimestamp("%Y-%m-%d %H:%M:%S") << "\n";
            m_logFile << "  Project : " << m_cachedProjectName << "\n";
            m_logFile << "  Preset  : " << m_currentConfig.presetName << "\n";
            m_logFile << "  Config  : " << m_currentConfig.config << "\n";
            m_logFile << "  MultiCfg: " << (m_currentConfig.isMultiConfig ? "true" : "false") << "\n";
            m_logFile << "========================================\n\n";
            m_logFile.flush();
        }
        catch (const std::exception& e)
        {
            std::cerr << "[BuildSystem] Failed to open log file: " << e.what() << std::endl;
        }
    }

    void BuildSystem::closeLogFile()
    {
        std::lock_guard<std::mutex> lock(m_logMutex);
        if (m_logFile.is_open())
        {
            m_logFile << "\n========================================\n";
            m_logFile << "  End of log\n";
            m_logFile << "========================================\n";
            m_logFile.close();
        }
    }

    // level: "INFO" / "WARN" / "ERROR" / "DEBUG" / "CMD"
    void BuildSystem::writeLog(const std::string& level, const std::string& message)
    {
        std::string line = std::format("[{}] [{}] {}", NowTimestamp("%H:%M:%S"), level, message);

        // コンソールへも出力（既存の std::cout を置き換える）
        std::cout << line << std::endl;

        std::lock_guard<std::mutex> lock(m_logMutex);
        if (m_logFile.is_open())
        {
            m_logFile << line << "\n";
            m_logFile.flush();  // クラッシュ時にも残るよう毎行フラッシュ
        }
    }

    // =========================================================
    // パス解決ヘルパー
    // =========================================================

    fs::path BuildSystem::getEditorRootDir() const { return m_cachedEditorRootDir; }

    fs::path BuildSystem::getPlayerDir() const
    {
        return m_cachedEditorRootDir / "player";
    }

    fs::path BuildSystem::getRuntimeExeDir() const
    {
        if (m_currentConfig.isMultiConfig)
            return m_cachedBuildDir / "runtime" / m_currentConfig.config;
        else
            return m_cachedBuildDir / "runtime";
    }

    fs::path BuildSystem::getDistOutputDir() const
    {
        return m_cachedEditorRootDir / "dist" / m_cachedProjectName;
    }

    // =========================================================
    // setProgressCallback / updateProgress
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
        m_currentResult.outputPath = getDistOutputDir().string();

        // ログレベルを status から決定
        std::string level = "INFO";
        if (status == BuildStatus::Failed)  level = "ERROR";
        if (status == BuildStatus::Warning) level = "WARN";
        if (status == BuildStatus::Success) level = "INFO";

        writeLog(level, std::format("[{:.0f}%] {}", progress * 100.0f, message));

        if (m_progressCallback)
            m_progressCallback(m_currentResult);
    }

    // =========================================================
    // RunCommandWithOutput
    // コマンドの出力も全てログファイルへ書き出す
    // =========================================================
    bool BuildSystem::RunCommandWithOutput(
        const std::string& command,
        std::function<void(const std::string&)> onOutput)
    {
        writeLog("CMD", "> " + command);

        FILE* pipe = _popen(command.c_str(), "r");
        if (!pipe)
        {
            writeLog("ERROR", "Failed to open pipe for command: " + command);
            return false;
        }

        char buffer[4096];
        while (fgets(buffer, sizeof(buffer), pipe))
        {
            if (m_cancelled) { _pclose(pipe); return false; }

            // 末尾の改行を除去してからログへ
            std::string line(buffer);
            while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
                line.pop_back();

            if (!line.empty())
                writeLog("CMD", "  " + line);

            onOutput(buffer);
        }

        int result = _pclose(pipe);
        writeLog(result == 0 ? "INFO" : "ERROR",
            std::format("Command exited with code {}", result));
        return result == 0;
    }

    // =========================================================
    // build()
    // =========================================================
    bool BuildSystem::build(bool forceRebuild)
    {
        m_cancelled = false;

        // ---- editorRootDir を確定 ----
        m_cachedEditorRootDir = FindEditorRootDir();
        if (m_cachedEditorRootDir.empty() || !fs::exists(m_cachedEditorRootDir))
        {
            // ログファイルを開く前なので stderr へ直接出力
            std::cerr << "[BuildSystem] ERROR: Could not locate editor/ directory from: "
                << GetWorkerExeDir().string() << std::endl;
            updateProgress(BuildStatus::Failed,
                std::format("Could not locate editor/ directory from: {}",
                    GetWorkerExeDir().string()), 0.0f);
            return false;
        }

        m_cachedBuildDir = m_cachedEditorRootDir.parent_path();
        m_currentConfig = DetectBuildConfig(m_cachedEditorRootDir);
        m_cachedSourceRootDir = FindSourceRootDir();

        m_cachedProjectName = GetProjectName();

        // ---- ログファイルを開く ----
        openLogFile();

        // ---- パス情報をログへ ----
        writeLog("INFO", "EditorRootDir : " + m_cachedEditorRootDir.string());
        writeLog("INFO", "BuildDir      : " + m_cachedBuildDir.string());
        writeLog("INFO", "PlayerDir     : " + getPlayerDir().string());
        writeLog("INFO", "RuntimeExeDir : " + getRuntimeExeDir().string());
        writeLog("INFO", "DistOutputDir : " + getDistOutputDir().string());
        writeLog("INFO", "SourceRootDir : " + m_cachedSourceRootDir.string());
        writeLog("INFO", "LogFile       : " + m_logFilePath.string());
        writeLog("INFO", "ForceRebuild  : " + std::string(forceRebuild ? "true" : "false"));

        // ---- ステップ実行ヘルパー ----
        auto runStep = [this](auto stepFn,
            const std::string& failMsg,
            float failProgress) -> bool
            {
                if (m_cancelled)
                {
                    updateProgress(BuildStatus::Failed, "Build cancelled by user", failProgress);
                    return false;
                }
                if (!stepFn())
                {
                    if (!m_cancelled)
                        updateProgress(BuildStatus::Failed, failMsg, failProgress);
                    else
                        updateProgress(BuildStatus::Failed, "Build cancelled by user", failProgress);
                    return false;
                }
                return true;
            };

        // ---- 各ステップ ----
        updateProgress(BuildStatus::Preparing, "Preparing output directory...", 0.00f);
        if (!runStep([this] { return prepareOutputDirectory(); },
            "Failed to prepare output directory", 0.00f))
        {
            closeLogFile(); return false;
        }

        updateProgress(BuildStatus::Building, "Preparing player cache...", 0.10f);
        if (!runStep([this, forceRebuild] { return preparePlayerCache(forceRebuild); },
            "Failed to prepare player cache", 0.10f))
        {
            closeLogFile(); return false;
        }

        updateProgress(BuildStatus::Building, "Copying player executable...", 0.50f);
        if (!runStep([this] { return copyPlayerExecutable(); },
            "Failed to copy player executable", 0.50f))
        {
            closeLogFile(); return false;
        }

        updateProgress(BuildStatus::CopyingAssets, "Copying assets...", 0.65f);
        if (!runStep([this] { return copyAssets(); },
            "Failed to copy assets", 0.65f))
        {
            closeLogFile(); return false;
        }

        updateProgress(BuildStatus::CopyingAssets, "Copying ProjectSettings...", 0.80f);
        if (!runStep([this] { return copyProjectSettings(); },
            "Failed to copy ProjectSettings.json", 0.80f))
        {
            closeLogFile(); return false;
        }

        updateProgress(BuildStatus::CopyingAssets, "Copying engine assets...", 0.90f);
        if (!runStep([this] { return copyEngineAssets(); },
            "Failed to copy engine assets", 0.90f))
        {
            closeLogFile(); return false;
        }

        updateProgress(BuildStatus::Success,
            std::format("Build completed! Output: {}", getDistOutputDir().string()), 1.00f);

        writeLog("INFO", std::format("Log saved to: {}", m_logFilePath.string()));
        closeLogFile();
        return true;
    }

    bool BuildSystem::cancel()
    {
        if (m_cancelled) return false;
        m_cancelled = true;
        writeLog("WARN", "Build cancelled by user.");
        return true;
    }

    // =========================================================
    // prepareOutputDirectory
    // =========================================================
    bool BuildSystem::prepareOutputDirectory()
    {
        try
        {
            auto outDir = getDistOutputDir();
            if (fs::exists(outDir))
            {
                fs::remove_all(outDir);
                writeLog("DEBUG", "Cleaned previous output: " + outDir.string());
            }
            fs::create_directories(outDir);
            writeLog("DEBUG", "DistOutputDir created: " + outDir.string());
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
    // preparePlayerCache
    bool BuildSystem::preparePlayerCache(bool forceRebuild)
    {
        fs::path playerDir = getPlayerDir();
        fs::path exeDir = GetWorkerExeDir(); // エディタ/Workerの実行ファイルがある階層

        // ランチャーは player/、DLL は exe の真横にある前提
        bool hasLauncher = fs::exists(playerDir / "OrionGame.exe");
        bool hasGameDll = fs::exists(exeDir / "OrionRuntime.dll");

        if (!forceRebuild && hasLauncher && hasGameDll)
        {
            writeLog("INFO", "Cache exists (Launcher in player/, DLL in exe root), skipping cmake build.");
            return true;
        }

        // エラーメッセージも実態に合わせて修正
        updateProgress(BuildStatus::Failed,
            "Missing OrionGame.exe (in player/) or OrionRuntime.dll (in exe root).\n"
            "Please check your installation.",
            0.1f);
        return false;
    }

    // =========================================================
    // copyPlayerExecutable
    //
    // DLL 方式の配置:
    //   OrionGame.exe        → dist/{ProjectName}/{ProjectName}.exe  (リネーム)
    //   {ProjectName}.dll    → dist/{ProjectName}/OrionRuntime.dll   (固定名)
    //   *.dll (依存DLL)      → dist/{ProjectName}/
    //
    // なぜ exe をリネームして dll を固定名にするか:
    //   ユーザーが起動するのは {ProjectName}.exe なので exe 側を見せ名にする。
    //   DLL 名は GameMain.cpp の LoadLibrary("OrionRuntime.dll") に合わせて固定。
    // =========================================================
    bool BuildSystem::copyPlayerExecutable()
    {
        fs::path playerDir = getPlayerDir();
        fs::path exeDir = GetWorkerExeDir();
        fs::path outDir = getDistOutputDir();

        // ---- ランチャー EXE: OrionGame.exe → {ProjectName}.exe にリネームしてコピー ----
        fs::path launcherSrc = playerDir / "OrionGame.exe";
        if (!fs::exists(launcherSrc))
        {
            updateProgress(BuildStatus::Failed,
                std::format("Launcher not found: {}", launcherSrc.string()), 0.5f);
            return false;
        }

        std::string dstExeName = m_cachedProjectName + ".exe";
        try
        {
            fs::create_directories(outDir);
            fs::copy_file(launcherSrc, outDir / dstExeName,
                fs::copy_options::overwrite_existing);
            writeLog("DEBUG", "Launcher: OrionGame.exe -> " + dstExeName);
        }
        catch (const std::exception& e)
        {
            updateProgress(BuildStatus::Failed,
                std::format("Failed to copy launcher: {}", e.what()), 0.5f);
            return false;
        }

        // GameMain.cpp の LoadLibrary("OrionRuntime.dll") に合わせた固定名
        const std::string dllName = (m_cachedProjectName == "OrionGame")
            ? "OrionRuntime"
            : m_cachedProjectName;
        fs::path gameDllSrc = exeDir / "OrionRuntime.dll";

        if (!fs::exists(gameDllSrc))
        {
            updateProgress(BuildStatus::Failed,
                std::format("Game DLL not found at exe root: {}", gameDllSrc.string()), 0.5f);
            return false;
        }

        try
        {
            fs::copy_file(gameDllSrc, outDir / "OrionRuntime.dll",
                fs::copy_options::overwrite_existing);
            writeLog("DEBUG", "GameDLL copied from exe root to dist.");
        }
        catch (const std::exception& e)
        {
            updateProgress(BuildStatus::Failed,
                std::format("Failed to copy game DLL: {}", e.what()), 0.5f);
            return false;
        }

        // 3. 依存DLLも exe の横から優先的にコピー
        copyDependencyDLLs(outDir, playerDir);
        return true;
    }

    // =========================================================
    // copyAssets
    // =========================================================
    bool BuildSystem::copyAssets()
    {
        auto  projectRoot = GetProjectRoot();

        // Assets に統一
        fs::path src = projectRoot / "Assets";
        fs::path dst = getDistOutputDir() / "Assets";

        writeLog("DEBUG", "Assets src: " + src.string());
        writeLog("DEBUG", "Assets dst: " + dst.string());

        if (!fs::exists(src))
        {
            updateProgress(BuildStatus::Warning,
                std::format("Warning: Asset folder not found: {}", src.string()), 0.65f);
            return true;
        }
        return copyDirectory(src, dst);
    }

    // =========================================================
    // copyProjectSettings
    // =========================================================
    bool BuildSystem::copyProjectSettings()
    {
        auto projectRoot = GetProjectRoot();

        fs::path src = projectRoot / "ProjectSettings.json";
        fs::path dst = getDistOutputDir() / "Assets" / "ProjectSettings.json";  // Assets に統一

        writeLog("DEBUG", "ProjectSettings src: " + src.string());
        writeLog("DEBUG", "ProjectSettings dst: " + dst.string());

        if (!fs::exists(src))
        {
            updateProgress(BuildStatus::Warning,
                std::format("Warning: ProjectSettings.json not found: {}", src.string()), 0.80f);
            return true;
        }

        try
        {
            std::ifstream inFile(src);
            if (!inFile.is_open())
            {
                updateProgress(BuildStatus::Warning,
                    std::format("Warning: Cannot open ProjectSettings.json: {}", src.string()), 0.80f);
                return true;
            }

            std::ostringstream oss;
            oss << inFile.rdbuf();
            std::string content = oss.str();
            inFile.close();

            ReplaceAll(content, "__PROJECT_NAME__", m_cachedProjectName);
            ReplaceAll(content, "__ENGINE_VERSION__", std::string("0.0.0"));

            fs::create_directories(dst.parent_path());
            std::ofstream outFile(dst);
            if (!outFile.is_open())
            {
                updateProgress(BuildStatus::Warning,
                    std::format("Warning: Cannot write ProjectSettings.json: {}", dst.string()), 0.80f);
                return true;
            }
            outFile << content;
            outFile.close();

            writeLog("DEBUG", "ProjectSettings written: ProjectName=" + m_cachedProjectName);
            return true;
        }
        catch (const std::exception& e)
        {
            updateProgress(BuildStatus::Warning,
                std::format("Warning: Failed to process ProjectSettings.json: {}", e.what()), 0.80f);
            return true;
        }
    }

    // =========================================================
    // copyEngineAssets
    // =========================================================
    bool BuildSystem::copyEngineAssets()
    {
        auto& settings = Engine::Core::ProjectSettings::get();
        fs::path src = settings.getEngineAssetPath();
        fs::path dst = getDistOutputDir() / "engine-assets";

        writeLog("DEBUG", "Engine assets src: " + src.string());
        writeLog("DEBUG", "Engine assets dst: " + dst.string());

        if (!fs::exists(src))
        {
            updateProgress(BuildStatus::Warning,
                std::format("Warning: engine-assets not found at: {}", src.string()), 0.90f);
            return true;
        }
        return copyDirectory(src, dst);
    }

    // =========================================================
    // copyDependencyDLLs
    // =========================================================
    void BuildSystem::copyDependencyDLLs(
        const fs::path& outputDir,
        const fs::path& sourceDir)
    {
        auto copyFrom = [&](const fs::path& dir) {
            if (!fs::exists(dir)) return;
            try {
                for (auto& f : fs::directory_iterator(dir))
                {
                    if (f.path().extension() != ".dll") continue;
                    auto fname = f.path().filename().string();
                    if (fname == m_cachedProjectName + ".dll") continue;
                    if (fname == "OrionRuntime.dll") continue;
                    fs::copy_file(f.path(), outputDir / f.path().filename(),
                        fs::copy_options::overwrite_existing);
                    writeLog("DEBUG", "DLL: " + fname);
                }
            }
            catch (const std::exception& e) {
                updateProgress(BuildStatus::Warning,
                    std::format("Warning: Failed to copy some DLLs: {}", e.what()), 0.0f);
            }
            };

        copyFrom(sourceDir); // player/

        copyFrom(getEditorRootDir());

        // vcpkg の依存DLLも追加
        fs::path vcpkgBin = m_cachedBuildDir / "vcpkg_installed" / "x64-windows" / "bin";
        copyFrom(vcpkgBin);
    }

    // =========================================================
    // copyDirectory
    // =========================================================
    bool BuildSystem::copyDirectory(
        const fs::path& source,
        const fs::path& dest)
    {
        if (!fs::exists(source))
        {
            writeLog("DEBUG", "Skipping (not found): " + source.string());
            return true;
        }

        try
        {
            fs::create_directories(dest);
            int count = 0;
            for (auto& e : fs::recursive_directory_iterator(source))
            {
                auto rel = fs::relative(e.path(), source);
                auto dst = dest / rel;
                if (e.is_directory())
                    fs::create_directories(dst);
                else
                {
                    fs::copy_file(e.path(), dst, fs::copy_options::overwrite_existing);
                    ++count;
                }
            }
            writeLog("DEBUG", std::format("Copied {} files: {} -> {}",
                count, source.string(), dest.string()));
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

