// editor/Utils/GameLogicProfiler.cpp
#include "GameLogicProfiler.hpp"

namespace Editor::UI
{
    using Clock = std::chrono::high_resolution_clock;

    // -------------------------------------------------------------------------
    void GameLogicProfiler::installLeafCallback(Engine::Scripting::LuaBindings* bindings)
    {
        if (!bindings) return;
        bindings->setLeafCallback(
            [this](const std::string& group, const std::string& func, float ms)
            { onLeaf(group, func, ms); }
        );
    }

    // -------------------------------------------------------------------------
    void GameLogicProfiler::registerComponent(Engine::Scripting::LuaScriptComponent* comp)
    {
        if (!comp || m_components.count(comp)) return;

        // onStart/onUpdate 全体の計測
        comp->setProfileCallback([this](UI::GameLogicEntry entry)
            { onEntry(std::move(entry)); });

        // depth 0 スコープ
        comp->setPushScopeCallback(
            [this](const std::string& objectName,
                const std::string& scriptName,
                const std::string& functionName,
                const std::string& scriptPath,
                int line)
            {
                pushScope(0, objectName, scriptName, "", functionName, scriptPath, line);
            }
        );
        comp->setPopScopeCallback([this]() { popScope(); });

        // 行プロファイル
        comp->setLineCallback(
            [this](const std::string& scriptPath, int line, float ms)
            { onLine(scriptPath, line, ms); }
        );

        m_components.insert(comp);
    }

    // -------------------------------------------------------------------------
    void GameLogicProfiler::unregisterComponent(Engine::Scripting::LuaScriptComponent* comp)
    {
        if (!comp) return;
        comp->setProfileCallback(nullptr);
        comp->setPushScopeCallback(nullptr);
        comp->setPopScopeCallback(nullptr);
        comp->setLineCallback(nullptr);
        m_components.erase(comp);
    }

    // -------------------------------------------------------------------------
    void GameLogicProfiler::pushScope(int depth,
        const std::string& objectName,
        const std::string& scriptName,
        const std::string& groupName,
        const std::string& functionName,
        const std::string& scriptPath,
        int line)
    {
        ScopeEntry entry;
        entry.depth = depth;
        entry.objectName = objectName;
        entry.scriptName = scriptName;
        entry.groupName = groupName;
        entry.functionName = functionName;
        entry.scriptPath = scriptPath;
        entry.line = line;
        entry.startTime = Clock::now();
        m_scopeStack.push_back(entry);
    }

    // -------------------------------------------------------------------------
    void GameLogicProfiler::popScope()
    {
        if (m_scopeStack.empty()) return;

        const auto& top = m_scopeStack.back();
        float ms = std::chrono::duration<float, std::milli>(
            Clock::now() - top.startTime).count();

        GameLogicEntry entry;
        entry.frameIndex = m_frameIndex;
        entry.objectName = top.objectName;
        entry.scriptName = top.scriptName;
        entry.groupName = top.groupName;
        entry.functionName = top.functionName;
        entry.scriptPath = top.scriptPath;
        entry.line = top.line;
        entry.depth = top.depth;
        entry.ms = ms;
        entry.succeeded = true;
        m_buffer.push_back(entry);

        m_scopeStack.pop_back();
    }

    // -------------------------------------------------------------------------
    void GameLogicProfiler::onLeaf(const std::string& group,
        const std::string& func,
        float ms)
    {
        if (!isInScope()) return;
        const auto* root = currentRootScope();

        GameLogicEntry entry;
        entry.frameIndex = m_frameIndex;
        entry.objectName = root ? root->objectName : "";
        entry.scriptName = root ? root->scriptName : "";
        entry.scriptPath = root ? root->scriptPath : "";
        entry.line = 0;
        entry.groupName = group;
        entry.functionName = func;
        entry.depth = 2;
        entry.ms = ms;
        entry.succeeded = true;
        m_buffer.push_back(entry);
    }

    // -------------------------------------------------------------------------
    // onLine: LuaScriptComponent の LineCallback から呼ばれる
    // -------------------------------------------------------------------------
    void GameLogicProfiler::onLine(const std::string& scriptPath, int line, float ms)
    {
        if (!m_window) return;
        m_window->addLineEntry(scriptPath, line, ms);
    }

    // -------------------------------------------------------------------------
    void GameLogicProfiler::onEntry(UI::GameLogicEntry entry)
    {
        entry.frameIndex = m_frameIndex;
        m_buffer.push_back(std::move(entry));
    }

    // -------------------------------------------------------------------------
    void GameLogicProfiler::flushToWindow()
    {
        if (m_window)
            m_window->setEntries(std::move(m_buffer));
        else
            m_buffer.clear();

        m_frameIndex++;
    }

    // -------------------------------------------------------------------------
    void GameLogicProfiler::reset()
    {
        m_frameIndex = 0;
        m_buffer.clear();
        m_scopeStack.clear();
        if (m_window)
        {
            m_window->setEntries({});
            m_window->clearLineEntries();
        }
    }

}