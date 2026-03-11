// src/Scripting/LuaScriptComponent.cpp
#include "LuaScriptComponent.hpp"
#include "../Utils/Common.hpp"
#include "GameObjectHandle.hpp"
#include "LuaAPI.hpp"

namespace Engine::Scripting {

    LuaScriptComponent::LuaScriptComponent(const std::string& scriptPath)
        : m_scriptPath(scriptPath)
    {
        ScriptManager::get().registerComponent(this);
    }

    LuaScriptComponent::~LuaScriptComponent()
    {
        invalidateCachedFunctions();
        ScriptManager::get().unregisterComponent(this);
    }

    void LuaScriptComponent::invalidateCachedFunctions()
    {
        Utils::log_info(std::format("Invalidating LuaScriptComponent: {}", m_scriptPath));

        m_scriptTable = sol::lua_nil;
        m_onStart = sol::lua_nil;
        m_onUpdate = sol::lua_nil;

        m_needsRefresh = true;
        m_luaStarted = false;

        Utils::log_info(std::format("LuaScriptComponent invalidated successfully: {}", m_scriptPath));
    }

    void LuaScriptComponent::start()
    {
        if (!isEnabled())
        {
            Utils::log_warning("LuaScriptComponent::start() called but component is disabled");
            return;
        }

        if (m_luaStarted) return;

        auto* obj = getGameObject();
        if (!obj || obj->isDestroyed())
        {
            Utils::log_warning("LuaScriptComponent::start aborted (GameObject invalid)");
            return;
        }

        auto& mgr = ScriptManager::get();

        Utils::log_info(std::format("Loading script: {}", m_scriptPath));

        auto result = mgr.loadScript(m_scriptPath, ScriptType::Component);
        if (!result)
        {
            Utils::log_error(Utils::make_error(
                Utils::ErrorType::FileI0,
                std::format("Failed to load script: {}", m_scriptPath)
            ));
            return;
        }

        m_scriptTable = mgr.getScriptTable(m_scriptPath);
        if (!m_scriptTable.valid())
        {
            Utils::log_warning(std::format(
                "Lua script '{}' did not return a valid table", m_scriptPath));
            return;
        }

        m_onStart = m_scriptTable["onStart"];
        m_onUpdate = m_scriptTable["onUpdate"];

        if (m_onStart.valid())
        {
            try
            {
                auto* currentObj = getGameObject();
                if (!currentObj || currentObj->isDestroyed())
                {
                    Utils::log_warning("GameObject invalid before onStart");
                    return;
                }

                Utils::log_info(std::format("Calling onStart for: {}", currentObj->getName()));

                // C++側で解決済みのGameObject*を直接渡す。LuaはIDを知らなくていい。
                m_onStart(currentObj);
            }
            catch (const sol::error& e)
            {
                Utils::log_error(Utils::make_error(Utils::ErrorType::FileI0,
                    std::format("Lua error in onStart ({}): {}", m_scriptPath, e.what())));
                m_onStart = sol::lua_nil;
            }
        }

        m_luaStarted = true;
        Utils::log_info(std::format("Script started successfully: {}", m_scriptPath));
    }

    void LuaScriptComponent::update(float deltaTime)
    {
        auto* obj = getGameObject();
        if (!obj || obj->isDestroyed()) return;

        if (m_needsRefresh)
        {
            m_luaStarted = false;
            m_needsRefresh = false;
        }

        if (!m_luaStarted) start();
        if (!m_onUpdate.valid()) return;

        try
        {
            auto* currentObj = getGameObject();
            if (!currentObj || currentObj->isDestroyed()) return;

            // C++側で解決済みのGameObject*を直接渡す。
            m_onUpdate(currentObj, deltaTime);
        }
        catch (const sol::error& e)
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::FileI0,
                std::format("Lua error in onUpdate ({}): {}", m_scriptPath, e.what())));
            m_onUpdate = sol::lua_nil;
        }
        catch (const std::exception& e)
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("C++ exception in onUpdate ({}): {}", m_scriptPath, e.what())));
            m_onUpdate = sol::lua_nil;
        }
    }

}