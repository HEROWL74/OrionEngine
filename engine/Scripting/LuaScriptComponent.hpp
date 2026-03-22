// engine/Scripting/LuaScriptComponent.hpp
#pragma once

#include <string>
#include <functional>
#include "../Core/GameObject.hpp"
#include "ScriptManager.hpp"
#include <sol/sol.hpp>
#include <lua.h>

namespace Editor::UI { struct GameLogicEntry; }

namespace Engine::Scripting
{
    class LuaScriptComponent : public Core::Component
    {
    public:
        explicit LuaScriptComponent(const std::string& scriptPath);
        ~LuaScriptComponent() override;

        LuaScriptComponent(const LuaScriptComponent&) = delete;
        LuaScriptComponent& operator=(const LuaScriptComponent&) = delete;
        LuaScriptComponent(LuaScriptComponent&&) = delete;
        LuaScriptComponent& operator=(LuaScriptComponent&&) = delete;

        void start() override;
        void update(float deltaTime) override;

        const std::string& getScriptPath() const { return m_scriptPath; }
        void invalidateCachedFunctions();

        using ProfileCallback = std::function<void(const Editor::UI::GameLogicEntry&)>;
        void setProfileCallback(ProfileCallback cb) { m_profileCallback = std::move(cb); }

        using PushScopeCallback = std::function<void(
            const std::string& objectName,
            const std::string& scriptName,
            const std::string& functionName,
            const std::string& scriptPath,
            int line)>;
        using PopScopeCallback = std::function<void()>;

        void setPushScopeCallback(PushScopeCallback cb) { m_pushScopeCallback = std::move(cb); }
        void setPopScopeCallback(PopScopeCallback   cb) { m_popScopeCallback = std::move(cb); }

        using LineCallback = std::function<void(
            const std::string& scriptPath,
            int line,
            float ms)>;
        void setLineCallback(LineCallback cb) { m_lineCallback = std::move(cb); }

        void setLineProfilingEnabled(bool enabled) { m_lineProfilingEnabled = enabled; }
        void setLineThresholdMs(float ms) { m_lineThresholdMs = ms; }

    private:
        std::string   m_scriptPath;
        sol::table    m_scriptTable;
        sol::function m_onStart;
        sol::function m_onUpdate;

        bool m_needsRefresh = true;
        bool m_luaStarted = false;

        ProfileCallback   m_profileCallback;
        PushScopeCallback m_pushScopeCallback;
        PopScopeCallback  m_popScopeCallback;
        LineCallback      m_lineCallback;

        bool  m_lineProfilingEnabled = true;
        float m_lineThresholdMs = 0.05f;

        void notifyProfile(
            const std::string& objectName,
            const std::string& functionName,
            float ms,
            bool  succeeded) const;

        // スタックを必ずバランスさせる安全な実装
        static int getFunctionLine(const sol::function& fn);
    };
}