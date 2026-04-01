// editor/UI/GameLogicConsoleWindow.cpp
#include "GameLogicConsoleWindow.hpp"
#include "../engine/Scripting/LuaScriptUtility.hpp"
#include <imgui.h>
#include <format>
#include <algorithm>

namespace Editor::UI
{
    const char* GameLogicConsoleWindow::msIcon(float ms, bool succeeded) const
    {
        if (!succeeded)               return "[FAILED]";
        if (ms >= m_errorThresholdMs) return "[CRITICAL]";
        if (ms >= m_warnThresholdMs)  return "[WARN]";
        return "[ OK]";
    }

    ImVec4 GameLogicConsoleWindow::msColor(float ms, bool succeeded) const
    {
        if (!succeeded)               return ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
        if (ms >= m_errorThresholdMs) return ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
        if (ms >= m_warnThresholdMs)  return ImVec4(1.0f, 0.85f, 0.0f, 1.0f);
        return ImVec4(0.4f, 1.0f, 0.5f, 1.0f);
    }

    bool GameLogicConsoleWindow::isCritical(float ms, bool succeeded) const
    {
        return !succeeded || ms >= m_errorThresholdMs;
    }

    // CRITICAL ノードの背景を描画する。TreeNodeEx の直前に呼ぶ。
    // SpanAvailWidth なノードは行全体を覆う幅が取れるため、
    // CursorScreenPos から AvailWidth 分の矩形をオーバーレイする。
    void GameLogicConsoleWindow::drawCriticalRowBg() const
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float  w = ImGui::GetContentRegionAvail().x;
        float  h = ImGui::GetTextLineHeightWithSpacing();
        dl->AddRectFilled(
            pos,
            ImVec2(pos.x + w, pos.y + h),
            IM_COL32(160, 20, 20, 90));   // 半透明の赤
    }

    // -------------------------------------------------------------------------
    void GameLogicConsoleWindow::addLineEntry(
        const std::string& scriptPath, int line, float ms)
    {
        // 同じ scriptPath:line があれば累計、なければ追加
        for (auto& e : m_lineEntries)
        {
            if (e.scriptPath == scriptPath && e.line == line)
            {
                e.totalMs += ms;
                e.hitCount++;
                return;
            }
        }
        LineEntry e;
        e.scriptPath = scriptPath;
        e.line = line;
        e.totalMs = ms;
        e.hitCount = 1;
        m_lineEntries.push_back(e);
    }

    void GameLogicConsoleWindow::clearLineEntries()
    {
        m_lineEntries.clear();
    }

    // -------------------------------------------------------------------------
    std::vector<ObjectEntry> GameLogicConsoleWindow::buildTree() const
    {
        std::vector<ObjectEntry> result;
        std::unordered_map<std::string, size_t> objIdx;

        auto findOrAddObj = [&](const std::string& name) -> ObjectEntry&
            {
                auto it = objIdx.find(name);
                if (it == objIdx.end())
                {
                    objIdx[name] = result.size();
                    result.emplace_back();
                    result.back().objectName = name;
                }
                return result[objIdx[name]];
            };
        auto findOrAddScript = [](ObjectEntry& obj, const std::string& name) -> ScriptEntry&
            {
                for (auto& s : obj.scripts)
                    if (s.scriptName == name) return s;
                obj.scripts.emplace_back();
                obj.scripts.back().scriptName = name;
                return obj.scripts.back();
            };
        auto findOrAddFunction = [](ScriptEntry& script, const std::string& name) -> FunctionEntry&
            {
                for (auto& f : script.functions)
                    if (f.functionName == name) return f;
                script.functions.emplace_back();
                script.functions.back().functionName = name;
                return script.functions.back();
            };
        auto findOrAddGroup = [](FunctionEntry& fn, const std::string& name) -> GroupEntry&
            {
                for (auto& g : fn.groups)
                    if (g.groupName == name) return g;
                fn.groups.emplace_back();
                fn.groups.back().groupName = name;
                return fn.groups.back();
            };

        // Pass 1: depth 0
        for (const auto& e : m_entries)
        {
            if (e.depth != 0) continue;
            ObjectEntry& obj = findOrAddObj(e.objectName);
            ScriptEntry& script = findOrAddScript(obj, e.scriptName);
            FunctionEntry& fn = findOrAddFunction(script, e.functionName);
            fn.selfMs += e.ms;
            fn.totalMs += e.ms;
            if (fn.scriptPath.empty() && !e.scriptPath.empty())
            {
                fn.scriptPath = e.scriptPath; fn.line = e.line;
            }
            script.totalMs += e.ms;
            obj.totalMs += e.ms;
        }

        // Pass 2: depth 2 → 後ろの depth 0 に紐付け
        for (size_t i = 0; i < m_entries.size(); ++i)
        {
            const auto& e = m_entries[i];
            if (e.depth != 2) continue;

            std::string parentFuncName;
            for (size_t j = i + 1; j < m_entries.size(); ++j)
            {
                const auto& c = m_entries[j];
                if (c.depth == 0 && c.objectName == e.objectName &&
                    c.scriptName == e.scriptName)
                {
                    parentFuncName = c.functionName; break;
                }
            }
            if (parentFuncName.empty()) continue;

            auto objIt = objIdx.find(e.objectName);
            if (objIt == objIdx.end()) continue;
            ObjectEntry& obj = result[objIt->second];

            ScriptEntry* script = nullptr;
            FunctionEntry* fn = nullptr;
            for (auto& s : obj.scripts)
                if (s.scriptName == e.scriptName) { script = &s; break; }
            if (!script) continue;
            for (auto& f : script->functions)
                if (f.functionName == parentFuncName) { fn = &f; break; }
            if (!fn) continue;

            const std::string gName = e.groupName.empty() ? "(other)" : e.groupName;
            GroupEntry& grp = findOrAddGroup(*fn, gName);

            LeafEntry leaf;
            leaf.functionName = e.functionName;
            leaf.scriptPath = e.scriptPath;
            leaf.line = e.line;
            leaf.ms = e.ms;
            leaf.succeeded = e.succeeded;
            grp.leaves.push_back(leaf);

            grp.totalMs += e.ms;
            fn->totalMs += e.ms;
            script->totalMs += e.ms;
            obj.totalMs += e.ms;
        }

        return result;
    }

    // -------------------------------------------------------------------------
    GameLogicConsoleWindow::GameLogicConsoleWindow()
        : ImGuiWindow("Game Logic Structure", true)
    {
    }

    void GameLogicConsoleWindow::setEntries(std::vector<GameLogicEntry> entries)
    {
        if (!entries.empty()) m_frameIndex = entries.back().frameIndex;
        m_entries = std::move(entries);
    }

    // -------------------------------------------------------------------------
    void GameLogicConsoleWindow::draw()
    {
        if (!m_visible) return;
        if (!ImGui::Begin(m_title.c_str(), &m_visible)) { ImGui::End(); return; }

        drawToolbar();
        ImGui::Separator();

        // タブで「関数ツリー」と「Hot Lines」を切り替え
        if (ImGui::BeginTabBar("GameLogicTabs"))
        {
            if (ImGui::BeginTabItem("Call Tree"))
            {
                if (m_entries.empty())
                    ImGui::TextDisabled("No data. Play the scene to see Lua execution.");
                else
                {
                    auto tree = buildTree();
                    drawTree(tree);
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Hot Lines"))
            {
                drawHotLines();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    void GameLogicConsoleWindow::drawToolbar()
    {
        ImGui::Text("Frame: %llu", static_cast<unsigned long long>(m_frameIndex));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80.0f);
        ImGui::SliderFloat("Warn(ms)", &m_warnThresholdMs, 0.1f, 10.0f);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80.0f);
        ImGui::SliderFloat("Err(ms)", &m_errorThresholdMs, 0.5f, 50.0f);
        ImGui::SameLine();
        ImGui::TextDisabled("(double-click to jump)");
    }

    // -------------------------------------------------------------------------
    // Hot Lines タブ
    // 重い行を totalMs 降順で表示。ダブルクリックで VSCode へジャンプ。
    // -------------------------------------------------------------------------
    void GameLogicConsoleWindow::drawHotLines()
    {
        if (m_lineEntries.empty())
        {
            ImGui::TextDisabled("No hot lines detected.");
            ImGui::TextDisabled("Lines exceeding the threshold will appear here.");
            return;
        }

        // totalMs 降順にソート（表示用コピー）
        std::vector<LineEntry> sorted = m_lineEntries;
        std::sort(sorted.begin(), sorted.end(),
            [](const LineEntry& a, const LineEntry& b) {
                return a.totalMs > b.totalMs;
            });

        if (ImGui::BeginTable("HotLines", 4,
            ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupColumn("Line", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("ms", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableSetupColumn("Hits", ImGuiTableColumnFlags_WidthFixed, 40.0f);
            ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (const auto& e : sorted)
            {
                ImGui::TableNextRow();

                // CRITICAL 行は背景を赤く塗る（RowBg より優先される）
                if (isCritical(e.totalMs, true))
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,
                        IM_COL32(160, 20, 20, 120));

                // 色分け
                ImVec4 col = msColor(e.totalMs, true);
                ImGui::PushStyleColor(ImGuiCol_Text, col);

                // Line 列
                ImGui::TableSetColumnIndex(0);
                std::string lineLabel = std::format("{}##line_{}_{}",
                    e.line, e.line, e.scriptPath);
                ImGui::Selectable(lineLabel.c_str(), false,
                    ImGuiSelectableFlags_SpanAllColumns);

                // ダブルクリックで VSCode へジャンプ
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                {
                    if (!e.scriptPath.empty())
                        Engine::Scripting::LuaScriptUtility::openInVSCode(
                            e.scriptPath, e.line);
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s:%d", e.scriptPath.c_str(), e.line);

                // ms 列
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.3f", e.totalMs);

                // Hits 列
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%d", e.hitCount);

                // File 列（ファイル名のみ）
                ImGui::TableSetColumnIndex(3);
                std::filesystem::path p(e.scriptPath);
                ImGui::Text("%s", p.filename().string().c_str());

                ImGui::PopStyleColor();
            }

            ImGui::EndTable();
        }
    }

    // -------------------------------------------------------------------------
    void GameLogicConsoleWindow::drawTree(const std::vector<ObjectEntry>& tree)
    {
        if (ImGui::BeginChild("LogicTreeArea", ImVec2(0, 0), false))
            for (const auto& obj : tree)
                drawObjectNode(obj);
        ImGui::EndChild();
    }

    void GameLogicConsoleWindow::drawObjectNode(const ObjectEntry& obj)
    {
        if (isCritical(obj.totalMs, true)) drawCriticalRowBg();
        ImVec4 col = msColor(obj.totalMs, true);
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        std::string label = std::format("{} {}  ({:.3f} ms)##obj_{}",
            msIcon(obj.totalMs, true), obj.objectName, obj.totalMs, obj.objectName);
        bool open = ImGui::TreeNodeEx(label.c_str(),
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);
        ImGui::PopStyleColor();
        if (open)
        {
            for (const auto& s : obj.scripts) drawScriptNode(s);
            ImGui::TreePop();
        }
    }

    void GameLogicConsoleWindow::drawScriptNode(const ScriptEntry& script)
    {
        if (isCritical(script.totalMs, true)) drawCriticalRowBg();
        ImVec4 col = msColor(script.totalMs, true);
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        std::string label = std::format("{} {}  ({:.3f} ms)##script_{}",
            msIcon(script.totalMs, true), script.scriptName,
            script.totalMs, script.scriptName);
        bool open = ImGui::TreeNodeEx(label.c_str(),
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);
        ImGui::PopStyleColor();
        if (open)
        {
            for (const auto& fn : script.functions) drawFunctionNode(fn);
            ImGui::TreePop();
        }
    }

    void GameLogicConsoleWindow::drawFunctionNode(const FunctionEntry& fn)
    {
        bool hasChildren = !fn.groups.empty();
        if (isCritical(fn.totalMs, true)) drawCriticalRowBg();
        ImVec4 col = msColor(fn.totalMs, true);
        ImGui::PushStyleColor(ImGuiCol_Text, col);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
        if (!hasChildren)
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        else
            flags |= ImGuiTreeNodeFlags_DefaultOpen;

        std::string label = std::format("{} {}  (self:{:.3f}ms  total:{:.3f}ms)##fn_{}",
            msIcon(fn.totalMs, true), fn.functionName,
            fn.selfMs, fn.totalMs, fn.functionName);

        bool open = ImGui::TreeNodeEx(label.c_str(), flags);
        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) && !fn.scriptPath.empty())
            Engine::Scripting::LuaScriptUtility::openInVSCode(fn.scriptPath, fn.line);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Double-click: %s:%d", fn.scriptPath.c_str(), fn.line);

        if (open && hasChildren)
        {
            for (const auto& grp : fn.groups) drawGroupNode(grp);
            ImGui::TreePop();
        }
    }

    void GameLogicConsoleWindow::drawGroupNode(const GroupEntry& group)
    {
        bool anyError = false;
        for (const auto& l : group.leaves) if (!l.succeeded) anyError = true;

        if (isCritical(group.totalMs, !anyError)) drawCriticalRowBg();
        ImVec4 col = msColor(group.totalMs, !anyError);
        ImGui::PushStyleColor(ImGuiCol_Text, col);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
        if (group.leaves.empty())
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

        std::string label = std::format("{} {}  ({:.3f} ms)##grp_{}",
            msIcon(group.totalMs, !anyError), group.groupName,
            group.totalMs, group.groupName);

        bool open = ImGui::TreeNodeEx(label.c_str(), flags);
        ImGui::PopStyleColor();

        if (open && !group.leaves.empty())
        {
            for (const auto& leaf : group.leaves) drawLeafNode(leaf);
            ImGui::TreePop();
        }
    }

    void GameLogicConsoleWindow::drawLeafNode(const LeafEntry& leaf)
    {
        if (isCritical(leaf.ms, leaf.succeeded)) drawCriticalRowBg();
        ImVec4 col = msColor(leaf.ms, leaf.succeeded);
        ImGui::PushStyleColor(ImGuiCol_Text, col);

        ImGui::TreeNodeEx(
            std::format("{}##{}", leaf.functionName, leaf.functionName).c_str(),
            ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
            ImGuiTreeNodeFlags_SpanAvailWidth);
        ImGui::SameLine();
        ImGui::Text("%s  %.3f ms", msIcon(leaf.ms, leaf.succeeded), leaf.ms);

        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) && !leaf.scriptPath.empty())
            Engine::Scripting::LuaScriptUtility::openInVSCode(leaf.scriptPath, leaf.line);
        if (ImGui::IsItemHovered() && leaf.line > 0)
            ImGui::SetTooltip("Double-click: %s:%d", leaf.scriptPath.c_str(), leaf.line);
    }

}