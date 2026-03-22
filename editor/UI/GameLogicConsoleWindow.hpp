// editor/UI/GameLogicConsoleWindow.hpp
#pragma once
#include "../engine/Utils/Common.hpp"
#include "ImGuiManager.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

namespace Editor::UI
{
    // =========================================================================
    // GameLogicEntry — onStart/onUpdate 全体 or Transform 呼び出し単位
    // =========================================================================
    struct GameLogicEntry
    {
        uint64_t    frameIndex = 0;
        std::string objectName;
        std::string scriptName;
        std::string groupName;
        std::string functionName;
        std::string scriptPath;
        int         line = 0;
        int         depth = 0;
        float       ms = 0.0f;
        bool        succeeded = true;
    };

    // =========================================================================
    // LineEntry — Lua 行単位の計測結果（重い行のみ）
    // =========================================================================
    struct LineEntry
    {
        std::string scriptPath;
        int         line = 0;
        float       totalMs = 0.0f;   // フレーム内の累計
        int         hitCount = 0;     // フレーム内の実行回数
    };

    // =========================================================================
    // ツリー構造体
    // =========================================================================
    struct LeafEntry
    {
        std::string functionName;
        std::string scriptPath;
        int         line = 0;
        float       ms = 0.0f;
        bool        succeeded = true;
    };

    struct GroupEntry
    {
        std::string            groupName;
        std::vector<LeafEntry> leaves;
        float                  totalMs = 0.0f;
    };

    struct FunctionEntry
    {
        std::string             functionName;
        std::string             scriptPath;
        int                     line = 0;
        std::vector<GroupEntry> groups;
        float                   selfMs = 0.0f;
        float                   totalMs = 0.0f;
    };

    struct ScriptEntry
    {
        std::string                scriptName;
        std::vector<FunctionEntry> functions;
        float                      totalMs = 0.0f;
    };

    struct ObjectEntry
    {
        std::string              objectName;
        std::vector<ScriptEntry> scripts;
        float                    totalMs = 0.0f;
    };

    // =========================================================================
    class GameLogicConsoleWindow : public ImGuiWindow
    {
    public:
        GameLogicConsoleWindow();
        ~GameLogicConsoleWindow() = default;

        GameLogicConsoleWindow(const GameLogicConsoleWindow&) = delete;
        GameLogicConsoleWindow& operator=(const GameLogicConsoleWindow&) = delete;

        void draw() override;

        // onStart/onUpdate + Transform エントリ
        void setEntries(std::vector<GameLogicEntry> entries);

        // 行プロファイル結果（重い行のみ）
        void addLineEntry(const std::string& scriptPath, int line, float ms);
        void clearLineEntries();

        const std::vector<GameLogicEntry>& getEntries() const { return m_entries; }

    private:
        std::vector<GameLogicEntry> m_entries;
        uint64_t                    m_frameIndex = 0;
        float                       m_warnThresholdMs = 1.0f;
        float                       m_errorThresholdMs = 5.0f;

        // 行プロファイル: scriptPath:line → LineEntry
        std::vector<LineEntry> m_lineEntries;

        std::vector<ObjectEntry> buildTree() const;

        void drawToolbar();
        void drawTree(const std::vector<ObjectEntry>& tree);
        void drawObjectNode(const ObjectEntry& obj);
        void drawScriptNode(const ScriptEntry& script);
        void drawFunctionNode(const FunctionEntry& fn);
        void drawGroupNode(const GroupEntry& group);
        void drawLeafNode(const LeafEntry& leaf);
        void drawHotLines();   // 重い行一覧

        const char* msIcon(float ms, bool succeeded) const;
        ImVec4      msColor(float ms, bool succeeded) const;
        bool        isCritical(float ms, bool succeeded) const;
        void        drawCriticalRowBg() const;
    };

}