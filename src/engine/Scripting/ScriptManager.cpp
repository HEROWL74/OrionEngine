//src/Scripting/ScriptManager.cpp

#include "ScriptManager.hpp"
#include "LuaScriptComponent.hpp"
#include "LuaBindings.hpp"
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

        if (m_bindingCallback)
        {
            Utils::log_info("Executing initial Lua bindings...");
            try
            {
                m_bindingCallback(m_lua);
                Utils::log_info("Initial Lua bindings completed");

                // バインディング確認
                verifyBindings();
            }
            catch (const std::exception& e)
            {
                Utils::log_error(Utils::make_error(
                    Utils::ErrorType::Unknown,
                    std::format("Failed to execute initial bindings: {}", e.what())
                ));
            }
        }
        else
        {
            Utils::log_warning("⚠️ Binding callback not set. Call setBindingCallback() before initialize()");
        }

        // スクリプトディレクトリをスキャン
        std::vector<std::string> scanDirs = { "scripts", "assets/scripts", "assets" };

        for (const auto& dir : scanDirs)
        {
            if (fs::exists(dir))
            {
                scanAndLoadSharedObjects(dir);
            }
        }
    }

    void ScriptManager::setBindingCallback(std::function<void(sol::state&)> callback)
    {
        m_bindingCallback = callback;
        Utils::log_info("Lua binding callback registered");
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
                    std::format("Failed to re-register bindings: {}", e.what())
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
                Utils::log_info(std::format("✓ {} is bound", typeName));
            }
            else
            {
                Utils::log_warning(std::format("✗ {} is NOT bound!", typeName));
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
            if (!fs::exists(path))
            {
                Utils::log_warning(std::format("Script file not found: {}", path));
                return false;
            }

            auto lastWrite = fs::last_write_time(path);

            {
                auto it = m_scripts.find(path);
                if (it != m_scripts.end() && it->second.lastWriteTime == lastWrite)
                {
                    return true;
                }
            }

            if (type == ScriptType::SharedObject)
            {
                m_lua.script_file(path);
            }

            ScriptData data;
            data.lastWriteTime = lastWrite;
            data.type = type;

            if (type == ScriptType::Component)
            {
                sol::load_result loaded = m_lua.load_file(path);
                if (!loaded.valid())
                {
                    sol::error err = loaded;
                    Utils::log_warning(std::format("Failed to load script '{}': {}", path, err.what()));
                    return false;
                }

                sol::function scriptFunc = loaded.get<sol::function>();
                sol::protected_function_result pfr = scriptFunc();

                if (!pfr.valid())
                {
                    sol::error err = pfr;
                    Utils::log_warning(std::format("Script execution error '{}': {}", path, err.what()));
                    return false;
                }

                sol::object result = pfr;

                if (!result.valid() || !result.is<sol::table>())
                {
                    Utils::log_warning(std::format(
                        "Script '{}' did not return a table. "
                        "Use: local Script = {} ... return Script",
                        path));
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
                        Utils::log_info(std::format("  Loaded module function: {}.{}", path.c_str(), name.c_str()));
                    }
                }
            }

            m_scripts[path] = std::move(data);

            if (type == ScriptType::SharedObject)
            {
                if (std::find(m_sharedObjects.begin(), m_sharedObjects.end(), path) == m_sharedObjects.end())
                {
                    m_sharedObjects.push_back(path);
                }
            }

            std::string typeStr = (type == ScriptType::SharedObject) ? "[SharedObject]" : "[Component]";
            Utils::log_info(std::format("Script loaded {}: {}", typeStr, path));
            return true;
        }
        catch (const sol::error& e)
        {
            Utils::log_warning(std::format("Lua error in '{}': {}", path, e.what()));
            return false;
        }
        catch (const std::exception& e)
        {
            Utils::log_warning(std::format("C++ exception in '{}': {}", path, e.what()));
            return false;
        }
    }

    void ScriptManager::scanAndLoadSharedObjects(const std::string& rootDirectory)
    {
        Utils::log_info(std::format("Scanning for ScriptableObjects in: {}", rootDirectory));

        if (!fs::exists(rootDirectory))
        {
            Utils::log_warning(std::format("Script directory not found: {}", rootDirectory));
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
        Utils::log_info(std::format("Invalidating {} registered LuaScriptComponents...", (unsigned long long)m_registeredComponents.size()));

        // コピーを作成してイテレーション
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

        Utils::log_info(std::format(
            "Invalidating {} registered LuaScriptComponents before VM reset...",
            m_registeredComponents.size()
        ));
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

                // バインディング確認
                verifyBindings();
            }
            catch (const std::exception& e)
            {
                Utils::log_error(Utils::make_error(
                    Utils::ErrorType::Unknown,
                    std::format("Failed to re-register bindings: {}", e.what())
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
            if (fs::exists(path))
            {
                Utils::log_info(std::format("  Loading: {}", path));
                loadScript(path, ScriptType::SharedObject);
            }
        }

        Utils::log_info("Reloading Component scripts...");
        for (const auto& [path, type] : allScriptPaths)
        {
            if (type == ScriptType::Component && fs::exists(path))
            {
                Utils::log_info(std::format("  Loading: {}", path));
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
                if (!fs::exists(path))
                {
                    continue;
                }

                auto it = m_scripts.find(path);
                if (it == m_scripts.end()) continue;

                auto lastWrite = fs::last_write_time(path);

                if (lastWrite != it->second.lastWriteTime)
                {
                    Utils::log_info(std::format("Reloading modified script: {}", path));

                    invalidateAllComponents();

                    loadScript(path, type);
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