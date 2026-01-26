//src/Scripting/ScriptManager.cpp

#include "ScriptManager.hpp"
#include "../Utils/Common.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace Engine::Scripting
{
    namespace fs = std::filesystem;

    ScriptManager& ScriptManager::get()
    {
        static ScriptManager instance;
        return instance;
    }

    void ScriptManager::initialize()
    {
        m_lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::os, sol::lib::string, sol::lib::table);
        Utils::log_info("Lua VM initialized.");

        // 複数のディレクトリをスキャン
        std::vector<std::string> scanDirs = { "scripts", "assets/scripts", "assets" };

        for (const auto& dir : scanDirs)
        {
            if (fs::exists(dir))
            {
                scanAndLoadScriptableObjects(dir);
            }
        }
    }

    bool ScriptManager::isScriptableObject(const std::string& filepath) const
    {
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            return false;
        }

        // ファイルの最初の数行をチェック（通常は最初の10行以内にマーカーがあるはず）
        std::string line;
        int lineCount = 0;
        const int maxLinesToCheck = 20;

        while (std::getline(file, line) && lineCount < maxLinesToCheck)
        {
            // 空白をトリム
            line.erase(0, line.find_first_not_of(" \t\r\n"));

            // マーカーをチェック
            // --@ScriptableObject または -- @ScriptableObject
            if (line.find("--@ScriptableObject") == 0 ||
                line.find("-- @ScriptableObject") == 0)
            {
                return true;
            }

            lineCount++;
        }

        return false;
    }

    bool ScriptManager::loadScript(const std::string& path, ScriptType type)
    {
        try {
            // ファイルの存在確認
            if (!fs::exists(path))
            {
                Utils::log_warning(std::format("Script file not found: {}", path));
                return false;
            }

            // ファイルの更新時刻を記録
            auto lastWrite = fs::last_write_time(path);

            // スクリプトを実行（グローバルスコープに展開）
            m_lua.script_file(path);

            ScriptData data;
            data.lastWriteTime = lastWrite;
            data.type = type;

            // Componentスクリプトの場合のみ関数をキャッシュ
            if (type == ScriptType::Component)
            {
                const std::vector<std::string> knownFunctions = {
                    "onUpdate", "onStart", "onCollisionEnter", "onCollisionExit"
                };

                for (const auto& name : knownFunctions)
                {
                    sol::object obj = m_lua[name];
                    if (obj.is<sol::function>())
                    {
                        data.functions[name] = obj.as<sol::function>();
                        Utils::log_info(std::format("  Loaded Lua function: {}", name));
                    }
                }
            }

            m_scripts[path] = std::move(data);

            // ScriptableObjectの場合はリストに追加
            if (type == ScriptType::ScriptableObject)
            {
                if (std::find(m_scriptableObjects.begin(), m_scriptableObjects.end(), path) == m_scriptableObjects.end())
                {
                    m_scriptableObjects.push_back(path);
                }
            }

            std::string typeStr = (type == ScriptType::ScriptableObject) ? "[ScriptableObject]" : "[Component]";
            Utils::log_info(std::format("Script loaded {}: {}", typeStr, path));
            return true;
        }
        catch (const std::exception& e)
        {
            Utils::log_warning(std::format("Lua error in '{}': {}", path, e.what()));
            return false;
        }
    }

    void ScriptManager::scanAndLoadScriptableObjects(const std::string& rootDirectory)
    {
        Utils::log_info(std::format("Scanning for ScriptableObjects in: {}", rootDirectory));

        if (!fs::exists(rootDirectory))
        {
            Utils::log_warning(std::format("Script directory not found: {}", rootDirectory));
            return;
        }

        int scannedCount = 0;
        int loadedCount = 0;

        // ルートディレクトリ配下の全ての.luaファイルを再帰的にスキャン
        try
        {
            for (const auto& entry : fs::recursive_directory_iterator(rootDirectory))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".lua")
                {
                    scannedCount++;
                    std::string path = entry.path().string();

                    // パス区切り文字を統一（Windowsの \ を / に変換）
                    std::replace(path.begin(), path.end(), '\\', '/');

                    // ファイル内容をチェックしてScriptableObjectマーカーがあれば読み込み
                    if (isScriptableObject(path))
                    {
                        if (loadScript(path, ScriptType::ScriptableObject))
                        {
                            loadedCount++;
                        }
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            Utils::log_warning(std::format("Error scanning directory: {}", e.what()));
        }

        Utils::log_info(std::format("Scanned {} Lua files, loaded {} ScriptableObjects", scannedCount, loadedCount));
    }

    sol::function ScriptManager::getFunction(const std::string& path, const std::string& functionName) const
    {
        auto it = m_scripts.find(path);
        if (it != m_scripts.end())
        {
            auto fit = it->second.functions.find(functionName);
            if (fit != it->second.functions.end())
            {
                return fit->second;
            }
        }

        // スクリプト固有のキャッシュになくても、グローバルに存在する可能性がある
        sol::object obj = m_lua[functionName];
        if (obj.is<sol::function>())
        {
            return obj.as<sol::function>();
        }

        return sol::nil;
    }

    sol::object ScriptManager::getGlobal(const std::string& name) const
    {
        return m_lua[name];
    }

    void ScriptManager::setGlobal(const std::string& name, sol::object value)
    {
        m_lua[name] = value;
    }

    void ScriptManager::reloadAll()
    {
        Utils::log_info("Reloading all scripts...");

        // グローバル変数を保存
        std::unordered_map<std::string, sol::object> savedGlobals;

        // 保存したいグローバル変数のリスト
        // ScriptableObjectで定義される可能性のある変数名
        std::vector<std::string> globalVarsToSave = {
            "GameState", "Config", "Constants", "Data", "Settings"
        };

        for (const auto& varName : globalVarsToSave)
        {
            sol::object obj = m_lua[varName];
            if (obj.valid() && !obj.is<sol::nil_t>())
            {
                savedGlobals[varName] = obj;
                Utils::log_info(std::format("  Saved global: {}", varName));
            }
        }

        // ScriptableObjectスクリプトを先に再読み込み
        for (const auto& path : m_scriptableObjects)
        {
            loadScript(path, ScriptType::ScriptableObject);
        }

        // 残りのスクリプトを再読み込み
        for (auto& [path, script] : m_scripts)
        {
            if (script.type == ScriptType::Component)
            {
                loadScript(path, ScriptType::Component);
            }
        }

        // グローバル変数を復元（上書きされていない場合のみ）
        for (const auto& [name, value] : savedGlobals)
        {
            sol::object current = m_lua[name];
            if (!current.valid() || current.is<sol::nil_t>())
            {
                m_lua[name] = value;
                Utils::log_info(std::format("  Restored global: {}", name));
            }
        }

        Utils::log_info("All Lua scripts reloaded.");
    }

    void ScriptManager::checkForUpdates()
    {
        for (auto& [path, script] : m_scripts)
        {
            try
            {
                if (!fs::exists(path))
                {
                    continue;
                }

                auto lastWrite = fs::last_write_time(path);

                // ファイルが更新されていたらリロード
                if (lastWrite != script.lastWriteTime)
                {
                    Utils::log_info(std::format("Reloading modified script: {}", path));
                    loadScript(path, script.type);
                }
            }
            catch (const std::exception& e)
            {
                Utils::log_warning(std::format("File check error for '{}': {}", path, e.what()));
            }
        }
    }

    void ScriptManager::dumpGlobals() const
    {
        Utils::log_info("=== Lua Global Variables ===");

        m_lua.globals().for_each([](const sol::object& key, const sol::object& value) {
            if (key.is<std::string>())
            {
                std::string keyStr = key.as<std::string>();

                // システム関数やライブラリは除外
                if (keyStr.find("_") == 0 || keyStr == "package" || keyStr == "coroutine" ||
                    keyStr == "string" || keyStr == "table" || keyStr == "math" ||
                    keyStr == "io" || keyStr == "os" || keyStr == "debug")
                {
                    return;
                }

                std::string typeStr = "unknown";
                if (value.is<bool>()) typeStr = "boolean";
                else if (value.is<double>()) typeStr = "number";
                else if (value.is<std::string>()) typeStr = "string";
                else if (value.is<sol::table>()) typeStr = "table";
                else if (value.is<sol::function>()) typeStr = "function";

                Utils::log_info(std::format("  {} : {}", keyStr, typeStr));
            }
            });

        Utils::log_info("============================");
    }
}