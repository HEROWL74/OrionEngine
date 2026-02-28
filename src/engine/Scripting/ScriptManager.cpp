//src/Scripting/ScriptManager.cpp

#include "ScriptManager.hpp"
#include "LuaScriptComponent.hpp"
#include <Windows.h>
#include "LuaBindings.hpp"
#include "../Utils/Common.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

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

        if (m_bindingCallback)
        {
            Utils::log_info("Executing initial Lua bindings...");
            try
            {
                m_bindingCallback(m_lua);
                Utils::log_info("Initial Lua bindings completed");
                verifyBindings();
            }
            catch (const std::exception& e)
            {
                Utils::log_error(Utils::make_error(
                    Utils::ErrorType::Unknown,
                    std::string("Failed to execute initial bindings: ") + e.what()
                ));
            }
        }
        else
        {
            Utils::log_warning(" Binding callback not set. Call setBindingCallback() before initialize()");
        }

        // Assets ディレクトリ全体をスキャン（統一）
        fs::path assetsDir = resolveAssetPath("");

        if (fs::exists(assetsDir))
        {
            Utils::log_info("Scanning for SharedObjects in: " + assetsDir.string());
            scanAndLoadSharedObjects(assetsDir.string());
        }
        else
        {
            Utils::log_warning("Assets directory not found: " + assetsDir.string());
        }
    }

    void ScriptManager::setBindingCallback(std::function<void(sol::state&)> callback)
    {
        m_bindingCallback = callback;
        Utils::log_info("Lua binding callback registered");
    }

    fs::path ScriptManager::resolveAssetPath(const std::string& relativePath) const
    {
        // 空の場合は Assets ルートを返す
        if (relativePath.empty())
        {
            // ProjectSettings が絶対パスを保持していれば最優先で使用
            auto& settings = Engine::Core::ProjectSettings::get();
            fs::path projectRoot = settings.getProjectRootDir();

            if (!projectRoot.empty())
            {
                fs::path assetsPath = projectRoot / "Assets";
                if (fs::exists(assetsPath))
                {
                    return fs::weakly_canonical(assetsPath);
                }
            }

            // フォールバック: exeDir から上へ辿って project/Assets を探す
            wchar_t exePath[MAX_PATH]{};
            GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            auto current = fs::path(exePath).parent_path();

            while (current.has_parent_path() && current != current.parent_path())
            {
                auto candidate = current / "project" / "Assets";
                if (fs::exists(candidate))
                    return fs::weakly_canonical(candidate);
                current = current.parent_path();
            }

            Utils::log_warning("Assets directory not found by exe-based search");
            return "Assets";
        }

        // パスを正規化（バックスラッシュをスラッシュに）
        std::string normalizedPath = relativePath;
        std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');

        // 大文字小文字を問わず "assets/" または "Assets/" プレフィックスを除去して
        // 純粋な相対パス（scripts/move.lua 等）に統一する
        {
            std::string lower = normalizedPath;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (lower.rfind("assets/", 0) == 0)
            {
                normalizedPath = normalizedPath.substr(7); // "assets/" の7文字を除去
            }
        }

        // ProjectSettings からパス解決
        auto& settings = Engine::Core::ProjectSettings::get();
        fs::path projectRoot = settings.getProjectRootDir();

        if (!projectRoot.empty())
        {
            // Assets に統一
            fs::path fullPath = projectRoot / "Assets" / normalizedPath;
            if (fs::exists(fullPath))
            {
                Utils::log_info("Resolved asset path: " + fullPath.string());
                return fs::weakly_canonical(fullPath);
            }
        }

        // フォールバック: exeDir から上へ辿って project/Assets/{path} を探す
        {
            wchar_t exePath[MAX_PATH]{};
            GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            auto current = fs::path(exePath).parent_path();

            while (current.has_parent_path() && current != current.parent_path())
            {
                auto candidate = current / "project" / "Assets" / normalizedPath;
                if (fs::exists(candidate))
                {
                    Utils::log_info("Resolved asset path (exe-search): " + candidate.string());
                    return fs::weakly_canonical(candidate);
                }
                current = current.parent_path();
            }
        }

        // 見つからない場合
        Utils::log_warning("Could not resolve asset path: " + normalizedPath);
        return normalizedPath;
    }

    void ScriptManager::rebindAll()
    {
        if (m_bindingCallback)
        {
            Utils::log_info("Re-registering all Lua bindings...");
            try
            {
                m_bindingCallback(m_lua);
                Utils::log_info("Lua bindings re-registered successfully");
            }
            catch (const std::exception& e)
            {
                Utils::log_error(Utils::make_error(
                    Utils::ErrorType::Unknown,
                    std::string("Failed to re-register bindings: ") + e.what()
                ));
            }
        }
        else
        {
            Utils::log_warning("⚠️ Binding callback not set! Call setBindingCallback() first.");
        }
    }

    void ScriptManager::verifyBindings()
    {
        Utils::log_info("=== Verifying Lua Bindings ===");

        const std::vector<std::string> requiredTypes = {
            "Vector3", "GameObject", "Transform", "UIText", "AudioComponent"
        };

        for (const auto& typeName : requiredTypes)
        {
            sol::optional<sol::table> type = m_lua[typeName];
            if (type)
            {
                Utils::log_info("✓ " + typeName + " is bound");
            }
            else
            {
                Utils::log_warning("✗ " + typeName + " is NOT bound!");
            }
        }

        Utils::log_info("============================");
    }

    bool ScriptManager::isSharedObject(const std::string& filepath) const
    {
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            return false;
        }

        std::string line;
        int lineCount = 0;
        const int maxLinesToCheck = 20;

        while (std::getline(file, line) && lineCount < maxLinesToCheck)
        {
            line.erase(0, line.find_first_not_of(" \t\r\n"));

            if (line.find("--@SharedObject") == 0 ||
                line.find("-- @SharedObject") == 0)
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
            // 入力パスを正規化（バックスラッシュ→スラッシュ、大文字小文字を問わず assets/ を Assets/ に統一）
            std::string inputPath = path;
            std::replace(inputPath.begin(), inputPath.end(), '\\', '/');
            {
                std::string lower = inputPath;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (lower.rfind("assets/", 0) == 0)
                    inputPath = "Assets/" + inputPath.substr(7);
            }

            // キャッシュキー: 常に "Assets/scripts/foo.lua" 形式の相対パスで統一
            // これにより LuaScriptComponent が渡す相対パスとキャッシュが一致する
            std::string cacheKey = inputPath;

            // 実ファイルパスを解決（絶対パスが必要な場合のみ resolveAssetPath を使う）
            fs::path resolvedPath = inputPath;
            if (!resolvedPath.is_absolute() && !fs::exists(resolvedPath))
            {
                resolvedPath = resolveAssetPath(inputPath);
            }

            if (!fs::exists(resolvedPath))
            {
                Utils::log_warning("Script file not found: " + path + " (resolved: " + resolvedPath.string() + ")");
                return false;
            }

            // normalizedPath は互換用（ログ等で使用）
            std::string normalizedPath = cacheKey;

            auto lastWrite = fs::last_write_time(resolvedPath);

            {
                auto it = m_scripts.find(cacheKey);
                if (it != m_scripts.end() && it->second.lastWriteTime == lastWrite)
                {
                    return true;
                }
            }

            if (type == ScriptType::SharedObject)
            {
                m_lua.script_file(resolvedPath.string());
            }

            ScriptData data;
            data.lastWriteTime = lastWrite;
            data.type = type;

            if (type == ScriptType::Component)
            {
                sol::load_result loaded = m_lua.load_file(resolvedPath.string());
                if (!loaded.valid())
                {
                    sol::error err = loaded;
                    Utils::log_warning("Failed to load script '" + resolvedPath.string() + "': " + err.what());
                    return false;
                }

                sol::function scriptFunc = loaded.get<sol::function>();
                sol::protected_function_result pfr = scriptFunc();

                if (!pfr.valid())
                {
                    sol::error err = pfr;
                    Utils::log_warning("Script execution error '" + resolvedPath.string() + "': " + err.what());
                    return false;
                }

                sol::object result = pfr;

                if (!result.valid() || !result.is<sol::table>())
                {
                    Utils::log_warning("Script '" + resolvedPath.string() + "' did not return a table. Use: local Script = {} ... return Script");
                    return false;
                }

                data.moduleTable = result.as<sol::table>();

                const std::vector<std::string> knownFunctions = {
                    "onStart", "onUpdate", "onCollisionEnter", "onCollisionExit"
                };

                for (const auto& name : knownFunctions)
                {
                    sol::object fn = data.moduleTable[name];
                    if (fn.valid() && fn.is<sol::function>())
                    {
                        data.functions[name] = fn.as<sol::function>();
                        Utils::log_info("  Loaded module function: " + normalizedPath + "." + name);
                    }
                }
            }

            m_scripts[cacheKey] = std::move(data);

            if (type == ScriptType::SharedObject)
            {
                if (std::find(m_sharedObjects.begin(), m_sharedObjects.end(), cacheKey) == m_sharedObjects.end())
                {
                    m_sharedObjects.push_back(cacheKey);
                }
            }

            std::string typeStr = (type == ScriptType::SharedObject) ? "[SharedObject]" : "[Component]";
            Utils::log_info("Script loaded " + typeStr + ": " + cacheKey + " (file: " + resolvedPath.string() + ")");
            return true;
        }
        catch (const sol::error& e)
        {
            Utils::log_warning("Lua error in '" + path + "': " + e.what());
            return false;
        }
        catch (const std::exception& e)
        {
            Utils::log_warning("C++ exception in '" + path + "': " + e.what());
            return false;
        }
    }

    void ScriptManager::scanAndLoadSharedObjects(const std::string& rootDirectory)
    {
        Utils::log_info("Scanning for ScriptableObjects in: " + rootDirectory);

        if (!fs::exists(rootDirectory))
        {
            Utils::log_warning("Script directory not found: " + rootDirectory);
            return;
        }

        int scannedCount = 0;
        int loadedCount = 0;

        try
        {
            for (const auto& entry : fs::recursive_directory_iterator(rootDirectory))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".lua")
                {
                    scannedCount++;
                    std::string path = entry.path().string();
                    std::replace(path.begin(), path.end(), '\\', '/');

                    if (isSharedObject(path))
                    {
                        if (loadScript(path, ScriptType::SharedObject))
                        {
                            loadedCount++;
                        }
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            Utils::log_warning(std::string("Error scanning directory: ") + e.what());
        }

        Utils::log_info("Scanned " + std::to_string(scannedCount) + " Lua files, loaded " + std::to_string(loadedCount) + " ScriptableObjects");
    }

    sol::function ScriptManager::getFunction(const std::string& path, const std::string& functionName) const
    {
        // 入力パスを cacheKey 形式（Assets/scripts/foo.lua）に正規化してから検索
        std::string key = path;
        std::replace(key.begin(), key.end(), '\\', '/');
        {
            std::string lower = key;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (lower.rfind("assets/", 0) == 0)
                key = "Assets/" + key.substr(7);
        }

        auto it = m_scripts.find(key);
        if (it != m_scripts.end())
        {
            auto fit = it->second.functions.find(functionName);
            if (fit != it->second.functions.end())
            {
                return fit->second;
            }
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

    void ScriptManager::registerComponent(LuaScriptComponent* comp)
    {
        if (comp)
        {
            m_registeredComponents.insert(comp);
        }
    }

    void ScriptManager::unregisterComponent(LuaScriptComponent* comp)
    {
        m_registeredComponents.erase(comp);
    }

    void ScriptManager::invalidateAllComponents()
    {
        Utils::log_info("Invalidating " + std::to_string(m_registeredComponents.size()) + " registered LuaScriptComponents...");

        std::vector<LuaScriptComponent*> componentsCopy;
        componentsCopy.reserve(m_registeredComponents.size());

        for (auto* comp : m_registeredComponents)
        {
            if (comp)
            {
                componentsCopy.push_back(comp);
            }
        }

        for (auto* comp : componentsCopy)
        {
            comp->invalidateCachedFunctions();
        }

        Utils::log_info("All components invalidated successfully");
    }

    void ScriptManager::reloadAll()
    {
        Utils::log_info("=== COMPLETE LUA VM RELOAD ===");

        std::vector<std::string> scriptableObjPaths = m_sharedObjects;
        std::unordered_map<std::string, ScriptType> allScriptPaths;

        for (const auto& [path, script] : m_scripts)
        {
            allScriptPaths[path] = script.type;
        }

        Utils::log_info("Invalidating " + std::to_string(m_registeredComponents.size()) + " registered LuaScriptComponents before VM reset...");
        invalidateAllComponents();

        Utils::log_info("Clearing script data...");
        m_scripts.clear();
        m_sharedObjects.clear();

        Utils::log_info("Destroying old Lua VM and creating new one...");
        m_lua = sol::state();
        m_lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::os,
            sol::lib::string, sol::lib::table);

        if (m_bindingCallback)
        {
            Utils::log_info("Re-registering Lua bindings...");
            try
            {
                m_bindingCallback(m_lua);
                Utils::log_info("Lua bindings re-registered successfully");
                verifyBindings();
            }
            catch (const std::exception& e)
            {
                Utils::log_error(Utils::make_error(
                    Utils::ErrorType::Unknown,
                    std::string("Failed to re-register bindings: ") + e.what()
                ));
            }
        }
        else
        {
            Utils::log_warning("Binding callback not set! Lua types may not work correctly.");
        }

        Utils::log_info("Reloading SharedObjects...");
        for (const auto& path : scriptableObjPaths)
        {
            // cacheKey（相対パス）から実ファイルパスを解決して存在確認
            fs::path resolved = resolveAssetPath(path);
            if (fs::exists(resolved))
            {
                Utils::log_info("  Loading: " + path);
                loadScript(path, ScriptType::SharedObject);
            }
        }

        Utils::log_info("Reloading Component scripts...");
        for (const auto& [path, type] : allScriptPaths)
        {
            fs::path resolved = resolveAssetPath(path);
            if (type == ScriptType::Component && fs::exists(resolved))
            {
                Utils::log_info("  Loading: " + path);
                loadScript(path, ScriptType::Component);
            }
        }

        Utils::log_info("=== LUA VM RELOAD COMPLETE ===");
    }

    void ScriptManager::checkForUpdates()
    {
        std::vector<std::pair<std::string, ScriptType>> scriptsToCheck;
        for (const auto& [path, script] : m_scripts)
        {
            scriptsToCheck.emplace_back(path, script.type);
        }

        for (const auto& [path, type] : scriptsToCheck)
        {
            try
            {
                // cacheKey（相対パス）から実ファイルパスを解決
                fs::path resolved = resolveAssetPath(path);
                if (!fs::exists(resolved))
                {
                    continue;
                }

                auto it = m_scripts.find(path);
                if (it == m_scripts.end()) continue;

                auto lastWrite = fs::last_write_time(resolved);

                if (lastWrite != it->second.lastWriteTime)
                {
                    Utils::log_info("Reloading modified script: " + path);
                    invalidateAllComponents();
                    loadScript(path, type);  // path は cacheKey（相対パス）、loadScript 内で解決する
                }
            }
            catch (const std::exception& e)
            {
                Utils::log_warning("File check error for '" + path + "': " + e.what());
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

                Utils::log_info("  " + keyStr + " : " + typeStr);
            }
            });

        Utils::log_info("============================");
    }
}
