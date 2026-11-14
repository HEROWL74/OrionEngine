// editor/buildworker/BuildSystem.cpp
#include "BuildSystem.hpp"

#include <Windows.h>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace Editor::Build {
namespace fs = std::filesystem;

// =========================================================
// 内部ユーティリティ
// =========================================================

static std::string NowTimestamp(const char *fmt) {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
  localtime_s(&tm, &time);
  std::ostringstream oss;
  oss << std::put_time(&tm, fmt);
  return oss.str();
}

static fs::path GetWorkerExeDir() {
  wchar_t path[MAX_PATH]{};
  GetModuleFileNameW(nullptr, path, MAX_PATH);
  return fs::path(path).parent_path();
}

// exeDir から上へ辿り "editor" フォルダを返す。
// CMakeCache.txt に依存しない。
//
// 探索順:
//   1. exeDir から上へ辿って "editor"
//   という名前のフォルダを探す（開発ビルド向け）
//   2. exeDir 自体に player/ か engine-assets/ があればそこをルートとする
//      （リリース配布: exe と同階層にリソースを置いた場合）
//   3. exeDir の1つ上（VS Multi-Config の Release/ / Debug/ サブフォルダ対応）
static fs::path FindEditorRootDir() {
  auto exeDir = GetWorkerExeDir();

  // 1. 上へ辿って "editor" フォルダを探す
  auto search = exeDir;
  while (search.has_parent_path() && search != search.parent_path()) {
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
static fs::path FindSourceRootDir() {
  auto current = GetWorkerExeDir();
  while (current.has_parent_path() && current != current.parent_path()) {
    if (fs::exists(current / "CMakeLists.txt"))
      return current;
    current = current.parent_path();
  }
  return {};
}

// VS/Ninja をフォルダ構造から判定する。CMakeCache.txt 不使用。
static BuildConfig DetectBuildConfig(const fs::path &editorRootDir) {
  auto exeDir = GetWorkerExeDir();
  auto exePath = exeDir.string();
  BuildConfig cfg;

  // パス文字列によるプリセット判定（最優先）
  if (exePath.find("x64-release-ci") != std::string::npos) {
    cfg.presetName = "x64-release-ci";
    cfg.config = "Release";
    cfg.isMultiConfig = false;
    return cfg;
  }
  if (exePath.find("x64-release-local") != std::string::npos) {
    cfg.presetName = "x64-release-local";
    cfg.config = "Release";
    cfg.isMultiConfig = true;
    return cfg;
  }
  if (exePath.find("x64-debug") != std::string::npos) {
    cfg.presetName = "x64-debug";
    cfg.config = "Debug";
    cfg.isMultiConfig = true;
    return cfg;
  }

  // フォルダ構造による VS/Ninja 判別
  bool hasReleaseSubdir = fs::exists(editorRootDir / "Release");
  bool hasDebugSubdir = fs::exists(editorRootDir / "Debug");

  if (hasReleaseSubdir || hasDebugSubdir) {
    cfg.isMultiConfig = true;
    cfg.config = hasDebugSubdir ? "Debug" : "Release";
    cfg.presetName =
        (cfg.config == "Debug") ? "x64-debug" : "x64-release-local";
  } else {
    cfg.isMultiConfig = false;
    cfg.config = "Release";
    cfg.presetName = "x64-release-ci";
  }
  return cfg;
}

static std::string GetProjectName() {
  const auto &name = Engine::Core::ProjectSettings::get().getProjectName();
  if (name.empty() || name.find("__") != std::string::npos)
    return "OrionGame";
  return name;
}

// ProjectSettings が保持する絶対パスをそのまま返す。
static fs::path GetProjectRoot() {
  auto &settings = Engine::Core::ProjectSettings::get();
  auto projectRoot = settings.getProjectRootDir();
  if (!projectRoot.empty() && fs::exists(projectRoot))
    return projectRoot;

  // フォールバック: editorRootDir / project
  wchar_t exePath[MAX_PATH]{};
  GetModuleFileNameW(nullptr, exePath, MAX_PATH);
  auto current = fs::path(exePath).parent_path();

  while (current.has_parent_path() && current != current.parent_path()) {
    auto candidate = current / "project";
    if (fs::exists(candidate / "ProjectSettings.json"))
      return fs::weakly_canonical(candidate);
    current = current.parent_path();
  }

  return {};
}

// =========================================================
// デストラクタ
// =========================================================
BuildSystem::~BuildSystem() { closeLogFile(); }

// =========================================================
// ログファイル管理
// =========================================================

fs::path BuildSystem::getLogDir() const {
  return m_cachedEditorRootDir / "logs";
}

void BuildSystem::openLogFile() {
  std::lock_guard<std::mutex> lock(m_logMutex);

  try {
    fs::create_directories(getLogDir());
    std::string timestamp = NowTimestamp("%Y%m%d_%H%M%S");
    m_logFilePath = getLogDir() / std::format("build_{}.txt", timestamp);
    m_logFile.open(m_logFilePath, std::ios::out | std::ios::trunc);
    if (!m_logFile.is_open()) {
      std::cerr << "[BuildSystem] WARNING: Could not open log file: "
                << m_logFilePath.string() << std::endl;
      return;
    }

    m_logFile << "========================================\n";
    m_logFile << "  Orion Engine Build Log\n";
    m_logFile << "  " << NowTimestamp("%Y-%m-%d %H:%M:%S") << "\n";
    m_logFile << "  Project : " << m_cachedProjectName << "\n";
    m_logFile << "  Preset  : " << m_currentConfig.presetName << "\n";
    m_logFile << "  Config  : " << m_currentConfig.config << "\n";
    m_logFile << "  MultiCfg: "
              << (m_currentConfig.isMultiConfig ? "true" : "false") << "\n";
    m_logFile << "========================================\n\n";
    m_logFile.flush();
  } catch (const std::exception &e) {
    std::cerr << "[BuildSystem] Failed to open log file: " << e.what()
              << std::endl;
  }
}

void BuildSystem::closeLogFile() {
  std::lock_guard<std::mutex> lock(m_logMutex);
  if (m_logFile.is_open()) {
    m_logFile << "\n========================================\n";
    m_logFile << "  End of log\n";
    m_logFile << "========================================\n";
    m_logFile.close();
  }
}

// level: "INFO" / "WARN" / "ERROR" / "DEBUG" / "CMD"
void BuildSystem::writeLog(const std::string &level,
                           const std::string &message) {
  std::string line =
      std::format("[{}] [{}] {}", NowTimestamp("%H:%M:%S"), level, message);

  std::cout << line << std::endl;

  std::lock_guard<std::mutex> lock(m_logMutex);
  if (m_logFile.is_open()) {
    m_logFile << line << "\n";
    m_logFile.flush();
  }
}

// =========================================================
// パス解決ヘルパー
// =========================================================

fs::path BuildSystem::getEditorRootDir() const { return m_cachedEditorRootDir; }

fs::path BuildSystem::getDistOutputDir() const {
  auto projectRoot = GetProjectRoot();
  return projectRoot / "dist" / m_cachedProjectName;
}

// =========================================================
// setProgressCallback / updateProgress
// =========================================================
void BuildSystem::setProgressCallback(ProgressCallback callback) {
  m_progressCallback = std::move(callback);
}

void BuildSystem::updateProgress(BuildStatus status, const std::string &message,
                                 float progress) {
  m_currentResult.status = status;
  m_currentResult.message = message;
  m_currentResult.progress = progress;
  m_currentResult.outputPath = getDistOutputDir().string();

  std::string level = "INFO";
  if (status == BuildStatus::Failed)
    level = "ERROR";
  if (status == BuildStatus::Warning)
    level = "WARN";

  writeLog(level, std::format("[{:.0f}%] {}", progress * 100.0f, message));

  if (m_progressCallback)
    m_progressCallback(m_currentResult);
}

// =========================================================
// RunCommandWithOutput
// =========================================================
bool BuildSystem::RunCommandWithOutput(
    const std::string &command,
    std::function<void(const std::string &)> onOutput) {
  writeLog("CMD", "> " + command);

  FILE *pipe = _popen(command.c_str(), "r");
  if (!pipe) {
    writeLog("ERROR", "Failed to open pipe for command: " + command);
    return false;
  }

  char buffer[4096];
  while (fgets(buffer, sizeof(buffer), pipe)) {
    if (m_cancelled) {
      _pclose(pipe);
      return false;
    }

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
bool BuildSystem::build(bool forceRebuild) {
  m_cancelled = false;

  m_cachedEditorRootDir = FindEditorRootDir();
  if (m_cachedEditorRootDir.empty() || !fs::exists(m_cachedEditorRootDir)) {
    std::string errorMsg =
        std::format("Could not locate editor/ directory from: {}",
                    GetWorkerExeDir().string());
    std::cerr << "[BuildSystem] ERROR: " << errorMsg << std::endl;
    updateProgress(BuildStatus::Failed, errorMsg, 0.0f);
    return false;
  }

  m_cachedBuildDir = m_cachedEditorRootDir.parent_path();
  m_currentConfig = DetectBuildConfig(m_cachedEditorRootDir);
  m_cachedSourceRootDir = FindSourceRootDir();
  m_cachedProjectName = GetProjectName();

  openLogFile();

  auto getSafePath = [](const fs::path &p) {
    std::string s = p.string();
    if (!s.empty() && (s.back() == '\\' || s.back() == '/'))
      s.pop_back();
    return s;
  };

  writeLog("INFO", "EditorRootDir : " + getSafePath(m_cachedEditorRootDir));
  writeLog("INFO", "DistOutputDir : " + getDistOutputDir().string());

  // ---- OrionBuild.exe を探す ----
  auto root = m_cachedSourceRootDir;
#ifdef _DEBUG
  auto exePath =
      root / "tools" / "OrionBuild" / "target" / "debug" / "OrionBuild.exe";
#else
  auto exePath =
      root / "tools" / "OrionBuild" / "target" / "release" / "OrionBuild.exe";
#endif

  if (!fs::exists(exePath)) {
    updateProgress(BuildStatus::Failed,
                   "OrionBuild.exe not found: " + exePath.string(), 0.0f);
    return false;
  }

  std::string projectRootStr = getSafePath(GetProjectRoot());
  std::string editorRootStr = getSafePath(m_cachedEditorRootDir);
  std::string exeStr = getSafePath(exePath);

  std::string innerCmd =
      std::format("\"{}\" build --project-root \"{}\" --editor-root \"{}\" "
                  "--project-name \"{}\" --config \"{}\"{}",
                  exeStr, projectRootStr, editorRootStr, m_cachedProjectName,
                  m_currentConfig.config, forceRebuild ? " --force" : "");

  // 全体を引用符でラップ（cmd.exe の引用符剥ぎ取り対策）
  std::string finalCmd = "\"" + innerCmd + "\"";

  writeLog("CMD", "Launching: " + finalCmd);

  bool ok = RunCommandWithOutput(
      finalCmd, [&](const std::string &line) { writeLog("RUST", line); });

  if (m_cancelled) {
    updateProgress(BuildStatus::Failed, "Build cancelled.", 0.0f);
  } else if (ok) {
    updateProgress(
        BuildStatus::Success,
        std::format("Build completed! Output: {}", getDistOutputDir().string()),
        1.0f);
  } else {
    updateProgress(BuildStatus::Failed, "Build failed. See log for details.",
                   1.0f);
  }
  closeLogFile();
  return ok;
}

bool BuildSystem::cancel() {
  if (m_cancelled)
    return false;
  m_cancelled = true;
  writeLog("WARN", "Build cancelled by user.");
  return true;
}

} // namespace Editor::Build