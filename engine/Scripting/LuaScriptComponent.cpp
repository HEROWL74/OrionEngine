// engine/Scripting/LuaScriptComponent.cpp
#include "LuaScriptComponent.hpp"
#include "../Utils/Common.hpp"
#include "GameObjectHandle.hpp"
#include "LuaAPI.hpp"
#include "../../editor/UI/GameLogicConsoleWindow.hpp"
#include <chrono>
#include <filesystem>

namespace Engine::Scripting {

    // HookData の thread_local 定義（クラス内 static thread_local の ODR 定義）
    namespace {
        thread_local std::function<void(const std::string&, int, float)> g_hookCb;
        thread_local std::string  g_hookPath;
        thread_local float        g_hookThreshold = 0.05f;
        thread_local std::chrono::time_point<std::chrono::high_resolution_clock> g_hookLineStart;
        thread_local int          g_hookPrevLine = 0;

        void luaLineHookImpl(lua_State* L, lua_Debug* ar)
        {
            auto now = std::chrono::high_resolution_clock::now();
            if (g_hookPrevLine > 0 && g_hookCb)
            {
                float ms = std::chrono::duration<float, std::milli>(
                    now - g_hookLineStart).count();
                if (ms >= g_hookThreshold)
                    g_hookCb(g_hookPath, g_hookPrevLine, ms);
            }
            lua_getinfo(L, "l", ar);
            g_hookPrevLine = ar->currentline;
            g_hookLineStart = now;
        }
    }

    static std::string extractScriptName(const std::string& path)
    {
        return std::filesystem::path(path).filename().string();
    }

    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    static float elapsedMs(TimePoint start)
    {
        return std::chrono::duration<float, std::milli>(Clock::now() - start).count();
    }

    // -------------------------------------------------------------------------
    // getFunctionLine
    //
    // lua_getinfo(L, ">S", &ar) の ">" オプションはスタックトップを
    // 消費して関数情報を取得する。
    // 成功時: スタックは fn.push() 前と同じ（getinfo が pop する）
    // 失敗時: fn.push() で積んだ値がスタックに残る → 手動 pop が必要
    //
    // Release CI では最適化により失敗ケースのスタック不一致が
    // 後続の Lua 操作でクラッシュを引き起こす。
    // → 呼び出し前後のスタック深さを記録して必ず一致させる。
    // -------------------------------------------------------------------------
    int LuaScriptComponent::getFunctionLine(const sol::function& fn)
    {
        if (!fn.valid()) return 0;

        lua_State* L = fn.lua_state();
        if (!L) return 0;

        const int stackBefore = lua_gettop(L);

        fn.push();  // スタック +1

        lua_Debug ar{};
        // lua_getinfo の ">" はスタックトップを pop して情報を取得する
        const int result = lua_getinfo(L, ">S", &ar);

        const int stackAfter = lua_gettop(L);

        if (stackAfter != stackBefore)
        {
            // 失敗時などでスタックが残っている場合は正規化する
            lua_settop(L, stackBefore);
        }

        if (result != 0)
            return ar.linedefined;

        return 0;
    }

    // -------------------------------------------------------------------------
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
        m_scriptTable = sol::lua_nil;
        m_onStart = sol::lua_nil;
        m_onUpdate = sol::lua_nil;
        m_needsRefresh = true;
        m_luaStarted = false;
    }

    void LuaScriptComponent::notifyProfile(
        const std::string& objectName,
        const std::string& functionName,
        float ms,
        bool  succeeded) const
    {
        if (!m_profileCallback) return;
        Editor::UI::GameLogicEntry entry;
        entry.objectName = objectName;
        entry.scriptName = extractScriptName(m_scriptPath);
        entry.functionName = functionName;
        entry.scriptPath = m_scriptPath;
        entry.depth = 0;
        entry.ms = ms;
        entry.succeeded = succeeded;
        m_profileCallback(entry);
    }

    // -------------------------------------------------------------------------
    void LuaScriptComponent::start()
    {
        if (!isEnabled() || m_luaStarted) return;

        auto* obj = getGameObject();
        if (!obj || obj->isDestroyed()) return;

        auto& mgr = ScriptManager::get();
        if (!mgr.loadScript(m_scriptPath, ScriptType::Component)) return;

        m_scriptTable = mgr.getScriptTable(m_scriptPath);
        if (!m_scriptTable.valid()) return;

        m_onStart = m_scriptTable["onStart"];
        m_onUpdate = m_scriptTable["onUpdate"];

        if (m_onStart.valid())
        {
            auto* currentObj = getGameObject();
            if (currentObj && !currentObj->isDestroyed())
            {
                const std::string objName = currentObj->getName();
                const std::string scriptName = extractScriptName(m_scriptPath);
                const int startLine = getFunctionLine(m_onStart);
                bool succeeded = true;

                if (m_pushScopeCallback)
                    m_pushScopeCallback(objName, scriptName, "onStart",
                        m_scriptPath, startLine);

                auto t0 = Clock::now();
                try { m_onStart(currentObj); }
                catch (const sol::error& e)
                {
                    Utils::log_error(Utils::make_error(Utils::ErrorType::FileI0,
                        std::format("Lua error in onStart: {}", e.what())));
                    m_onStart = sol::lua_nil;
                    succeeded = false;
                }

                if (m_popScopeCallback) m_popScopeCallback();
                notifyProfile(objName, "onStart", elapsedMs(t0), succeeded);
            }
        }

        m_luaStarted = true;
    }

    // -------------------------------------------------------------------------
    void LuaScriptComponent::update(float deltaTime)
    {
        auto* obj = getGameObject();
        if (!obj || obj->isDestroyed()) return;

        if (m_needsRefresh) { m_luaStarted = false; m_needsRefresh = false; }
        if (!m_luaStarted) start();
        if (!m_onUpdate.valid()) return;

        auto* currentObj = getGameObject();
        if (!currentObj || currentObj->isDestroyed()) return;

        const std::string objName = currentObj->getName();
        const std::string scriptName = extractScriptName(m_scriptPath);
        const int updateLine = getFunctionLine(m_onUpdate);
        bool succeeded = true;

        if (m_pushScopeCallback)
            m_pushScopeCallback(objName, scriptName, "onUpdate",
                m_scriptPath, updateLine);

        // ----------------------------------------------------------------
        // 行プロファイリング
        // lua_sethook を使わず、Lua の debug ライブラリ経由で安全に実装。
        // onUpdate 実行前後でスタックを保護する。
        // ----------------------------------------------------------------
        lua_State* L = m_onUpdate.lua_state();
        bool hookActive = false;

        if (m_lineProfilingEnabled && m_lineCallback && L)
        {
            // thread_local グローバルにコールバックを値コピー
            // → フック関数は this に依存しない（dangling しない）
            g_hookCb = m_lineCallback;  // 値コピー
            g_hookPath = m_scriptPath;
            g_hookThreshold = m_lineThresholdMs;
            g_hookPrevLine = 0;
            g_hookLineStart = Clock::now();

            lua_sethook(L, luaLineHookImpl, LUA_MASKLINE, 0);
            hookActive = true;
        }

        auto t0 = Clock::now();
        try { m_onUpdate(currentObj, deltaTime); }
        catch (const sol::error& e)
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::FileI0,
                std::format("Lua error in onUpdate: {}", e.what())));
            m_onUpdate = sol::lua_nil;
            succeeded = false;
        }
        catch (const std::exception& e)
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("C++ exception in onUpdate: {}", e.what())));
            m_onUpdate = sol::lua_nil;
            succeeded = false;
        }

        // 例外が飛んでも必ずフック解除
        if (hookActive && L)
            lua_sethook(L, nullptr, 0, 0);

        if (m_popScopeCallback) m_popScopeCallback();
        notifyProfile(objName, "onUpdate", elapsedMs(t0), succeeded);
    }

}