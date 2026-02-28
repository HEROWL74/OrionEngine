//src/Scripting/ScriptManager.hpp
#pragma once

#include <sol/sol.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <filesystem>
#include <functional>
#include "../Core/ProjectSettings.hpp"

namespace Engine::Scripting
{
    class LuaScriptComponent;

    enum class ScriptType
    {
        Component,
        SharedObject
    };

    struct ScriptData
    {
        std::filesystem::file_time_type lastWriteTime;
        ScriptType type;
        sol::table moduleTable;
        std::unordered_map<std::string, sol::function> functions;
    };

    class ScriptManager
    {
    public:
        static ScriptManager& get();

        void initialize();

        void setBindingCallback(std::function<void(sol::state&)> callback);

        void rebindAll();

        void verifyBindings();

        bool loadScript(const std::string& path, ScriptType type);
        void scanAndLoadSharedObjects(const std::string& rootDirectory);

        sol::function getFunction(const std::string& path, const std::string& functionName) const;

        sol::table getScriptTable(const std::string& path) const
        {
            // cacheKey Œ`Ž®iAssets/scripts/foo.luaj‚É³‹K‰»‚µ‚Ä‚©‚çŒŸõ
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
                return it->second.moduleTable;
            }
            return sol::nil;
        }

        sol::object getGlobal(const std::string& name) const;
        void setGlobal(const std::string& name, sol::object value);

        sol::state& getLuaState() { return m_lua; }

        void registerComponent(LuaScriptComponent* comp);
        void unregisterComponent(LuaScriptComponent* comp);

        void invalidateAllComponents();
        void reloadAll();
        void checkForUpdates();

        void dumpGlobals() const;

    private:
        ScriptManager() = default;
        ~ScriptManager() = default;
        ScriptManager(const ScriptManager&) = delete;
        ScriptManager& operator=(const ScriptManager&) = delete;

        bool isSharedObject(const std::string& filepath) const;
        std::filesystem::path resolveAssetPath(const std::string& relativePath) const;

        sol::state m_lua;
        std::unordered_map<std::string, ScriptData> m_scripts;
        std::vector<std::string> m_sharedObjects;
        std::unordered_set<LuaScriptComponent*> m_registeredComponents;

        std::function<void(sol::state&)> m_bindingCallback;
    };
}