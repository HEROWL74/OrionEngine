// editor/Utils/GameLogicProfiler.hpp
#pragma once

#include "../UI/GameLogicConsoleWindow.hpp"
#include "../engine/Scripting/LuaScriptComponent.hpp"
#include "../engine/Scripting/LuaBindings.hpp"
#include <vector>
#include <unordered_set>
#include <cstdint>
#include <string>
#include <chrono>

namespace Editor::UI
{
    struct ScopeEntry
    {
        std::string objectName;
        std::string scriptName;
        std::string groupName;
        std::string functionName;
        std::string scriptPath;
        int         line = 0;
        int         depth = 0;
        std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
    };

    class GameLogicProfiler
    {
    public:
        GameLogicProfiler() = default;
        ~GameLogicProfiler() = default;
        GameLogicProfiler(const GameLogicProfiler&) = delete;
        GameLogicProfiler& operator=(const GameLogicProfiler&) = delete;

        void setWindow(GameLogicConsoleWindow* window) { m_window = window; }

        void installLeafCallback(Engine::Scripting::LuaBindings* bindings);
        void registerComponent(Engine::Scripting::LuaScriptComponent* comp);
        void unregisterComponent(Engine::Scripting::LuaScriptComponent* comp);

        void pushScope(int depth,
            const std::string& objectName,
            const std::string& scriptName,
            const std::string& groupName,
            const std::string& functionName,
            const std::string& scriptPath,
            int line);
        void popScope();

        void flushToWindow();
        void reset();

        uint64_t getFrameIndex() const { return m_frameIndex; }
        bool     isInScope()     const { return !m_scopeStack.empty(); }

        const ScopeEntry* currentRootScope() const
        {
            for (const auto& s : m_scopeStack)
                if (s.depth == 0) return &s;
            return nullptr;
        }

    private:
        GameLogicConsoleWindow* m_window = nullptr;
        std::unordered_set<Engine::Scripting::LuaScriptComponent*> m_components;
        std::vector<GameLogicEntry> m_buffer;
        std::vector<ScopeEntry>     m_scopeStack;
        uint64_t                    m_frameIndex = 0;

        void onEntry(GameLogicEntry entry);
        void onLeaf(const std::string& group, const std::string& func, float ms);

        // 行プロファイル結果をウィンドウへ転送
        void onLine(const std::string& scriptPath, int line, float ms);
    };

}