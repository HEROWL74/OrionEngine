//src/Scripting/LuaScriptComponent.hpp
#pragma once

#include <string>
#include "../Core/GameObject.hpp"
#include "ScriptManager.hpp"
#include <sol/sol.hpp>

namespace Engine::Scripting
{
    // ================================================
    // LuaスクリプトをGameObjectに付与するコンポーネント
    // ================================================
    class LuaScriptComponent : public Core::Component
    {
    public:
        explicit LuaScriptComponent(const std::string& scriptPath);

        // コンポーネント破棄時にScriptManagerから登録解除するため明示的に宣言
        ~LuaScriptComponent() override;

        //コピー・ムーブ禁止
        LuaScriptComponent(const LuaScriptComponent&) = delete;
        LuaScriptComponent& operator=(const LuaScriptComponent&) = delete;
        LuaScriptComponent(LuaScriptComponent&&) = delete;
        LuaScriptComponent& operator=(LuaScriptComponent&&) = delete;

        // ライフサイクル
        void start() override;
        void update(float deltaTime) override;

        // スクリプトパスの取得（シリアライズ時に使用）
        const std::string& getScriptPath() const { return m_scriptPath; }

        // ScriptManagerからVM再構築前に呼ばれる。
        // キャッシュされたsol::functionを全て無効化する。
        void invalidateCachedFunctions();

    private:
        std::string   m_scriptPath;
        sol::table m_scriptTable;
        sol::function m_onStart;
        sol::function m_onUpdate;

        // trueの場合、次のupdate()で関数ハンドルを再取得する
        bool m_needsRefresh = true;
        bool m_luaStarted = false;
    };
}