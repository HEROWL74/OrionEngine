// src/engine/Core/ProjectSettings.cpp
#include "ProjectSettings.hpp"
#include "../Utils/Common.hpp"

#include <Windows.h>
#include <fstream>
#include <sstream>

namespace Engine::Core
{
    // Singleton
    ProjectSettings& ProjectSettings::get()
    {
        static ProjectSettings instance;
        return instance;
    }

    // =========================================================
    // 内部ユーティリティ
    // =========================================================
    namespace
    {
        static std::filesystem::path GetExeDir()
        {
            wchar_t path[MAX_PATH]{};

            GetModuleFileNameW(nullptr, path, MAX_PATH);

            return std::filesystem::path(path).parent_path();
        }

        bool parseString(const std::string& src, const std::string& key, std::string& out)
        {
            std::string searchKey = "\"" + key + "\"";
            auto pos = src.find(searchKey);
            if (pos == std::string::npos) return false;

            pos = src.find(':', pos + searchKey.size());
            if (pos == std::string::npos) return false;

            auto q1 = src.find('"', pos + 1);
            if (q1 == std::string::npos) return false;

            auto q2 = src.find('"', q1 + 1);
            if (q2 == std::string::npos) return false;

            out = src.substr(q1 + 1, q2 - q1 - 1);
            return true;
        }

        bool parseInt(const std::string& src, const std::string& key, int& out)
        {
            std::string searchKey = "\"" + key + "\"";
            auto pos = src.find(searchKey);
            if (pos == std::string::npos) return false;

            pos = src.find(':', pos + searchKey.size());
            if (pos == std::string::npos) return false;

            auto numStart = src.find_first_not_of(" \t\r\n", pos + 1);
            if (numStart == std::string::npos) return false;

            try {
                size_t consumed = 0;
                out = std::stoi(src.substr(numStart), &consumed);
                return consumed > 0;
            }
            catch (...) { return false; }
        }

        bool parseBool(const std::string& src, const std::string& key, bool& out)
        {
            std::string searchKey = "\"" + key + "\"";
            auto pos = src.find(searchKey);
            if (pos == std::string::npos) return false;

            pos = src.find(':', pos + searchKey.size());
            if (pos == std::string::npos) return false;

            auto valStart = src.find_first_not_of(" \t\r\n", pos + 1);
            if (valStart == std::string::npos) return false;

            if (src.compare(valStart, 4, "true") == 0) { out = true;  return true; }
            if (src.compare(valStart, 5, "false") == 0) { out = false; return true; }
            return false;
        }

        bool extractBlock(const std::string& src, const std::string& key, std::string& block)
        {
            std::string searchKey = "\"" + key + "\"";
            auto pos = src.find(searchKey);
            if (pos == std::string::npos) return false;

            auto brace = src.find('{', pos + searchKey.size());
            if (brace == std::string::npos) return false;

            int depth = 0;
            auto it = brace;
            while (it < src.size()) {
                if (src[it] == '{') ++depth;
                else if (src[it] == '}') {
                    if (--depth == 0) { block = src.substr(brace, it - brace + 1); return true; }
                }
                ++it;
            }
            return false;
        }
    }

    // =========================================================
    // setPaths
    // =========================================================
    void ProjectSettings::setPaths(const EnginePaths& paths)
    {
        m_paths = paths;
    }

    // =========================================================
    // load
    // =========================================================
    bool ProjectSettings::load(const std::filesystem::path& jsonPath)
    {
        std::ifstream file(jsonPath);
        if (!file.is_open())
        {
            Utils::log_warning("ProjectSettings: cannot open " + jsonPath.string() + " — using defaults");
            return false;
        }

        std::ostringstream ss;
        ss << file.rdbuf();
        std::string src = ss.str();

        parseString(src, "ProjectName", m_projectName);
        parseString(src, "EngineVersion", m_engineVersion);
        parseString(src, "DefaultScene", m_defaultScene);
        parseString(src, "AssetRoot", m_assetRoot);
        parseString(src, "ProjectType", m_projectType);

        std::string windowBlock;
        if (extractBlock(src, "Window", windowBlock))
        {
            parseInt(windowBlock, "Width", m_window.width);
            parseInt(windowBlock, "Height", m_window.height);
            parseBool(windowBlock, "Fullscreen", m_window.fullscreen);
            parseBool(windowBlock, "VSync", m_window.vsync);
        }


        m_jsonPath = std::filesystem::weakly_canonical(jsonPath);
        m_projectRootDir = m_jsonPath.parent_path();

        // projectRoot は JSON の場所から自動決定
        m_paths.projectRoot = m_projectRootDir;

        Utils::log_info("ProjectSettings loaded from: " + m_jsonPath.string());
        Utils::log_info("  ProjectName  : " + m_projectName);
        Utils::log_info("  AssetRoot    : " + m_assetRoot);
        Utils::log_info("  AssetRootPath: " + getAssetRootPath().string());
        Utils::log_info("  DefaultScene : " + getDefaultScenePath().string());
        return true;
    }

    // =========================================================
    // loadForEditor
    //
    // exeの場所から上へ辿り "project/ProjectSettings.json" を探す。
    // カレントディレクトリに依存しない。
    //
    // ディレクトリ構成（開発ビルド）:
    //   build/x64-debug/
    //     editor/
    //       Debug/
    //         OrionEditor.exe   ← GetExeDir()
    //       project/            ← ここを探す
    //         ProjectSettings.json
    //         Assets/
    //       engine-assets/
    //
    // ディレクトリ構成（配布）:
    //   OrionEditor/
    //     OrionEditor.exe       ← GetExeDir()
    //     project/
    //       ProjectSettings.json
    //       Assets/
    //     engine-assets/
    // =========================================================
    void ProjectSettings::loadForEditor()
    {
        m_projectName.clear();

        auto exeDir = GetExeDir();
        auto current = exeDir;

        // exeDir から上へ辿って project/ProjectSettings.json を探す
        while (current.has_parent_path() && current != current.parent_path())
        {
            auto candidate = current / "project" / "ProjectSettings.json";
            if (std::filesystem::exists(candidate))
            {
                if (load(candidate)) return;
            }
            current = current.parent_path();
        }

        // 見つからない場合は exeDir 基準の絶対パスでデフォルト設定
        Utils::log_warning("ProjectSettings(Editor): project/ProjectSettings.json not found — using defaults");
        m_projectRootDir = std::filesystem::weakly_canonical(exeDir / "project");
    }

    // =========================================================
    // loadForRuntime
    //
    // 配布パッケージでは Assets/ と同階層 or Assets/ 内に
    // ProjectSettings.json を置く運用。
    //
    // ディレクトリ構成（配布）:
    //   MyGame/
    //     MyGame.exe            ← GetExeDir()
    //     Assets/
    //       ProjectSettings.json
    //       scenes/
    //     engine-assets/
    // =========================================================
    void ProjectSettings::loadForRuntime()
    {
        auto exeDir = GetExeDir();

        const std::vector<std::filesystem::path> candidates = {
            exeDir / "Assets" / "ProjectSettings.json",
            exeDir / "ProjectSettings.json",
        };

        for (const auto& p : candidates)
        {
            if (std::filesystem::exists(p))
            {
                if (load(p)) return;
            }
        }

        Utils::log_warning("ProjectSettings(Runtime): JSON not found — using defaults");
        m_projectRootDir = exeDir;
    }

    std::wstring ProjectSettings::getProjectNameW() const
    {
        std::wstring result;
        result.reserve(m_projectName.size());
        for (unsigned char c : m_projectName)
        {
            result += static_cast<wchar_t>(c);
        }
        return result;
    }

} // namespace Engine::Core

