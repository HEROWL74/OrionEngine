// engine/Scripting/LuaBindings.hpp
#pragma once

#include <sol/sol.hpp>
#include <functional>
#include <string>
#include "../World/Camera.hpp"
#include "../Math/Math.hpp"
#include "../Core/GameObject.hpp"
#include "../Input/InputSystem.hpp"
#include "../Physics/PhysicsSystem.hpp"
#include "../UI/UIComponent.hpp"
#include "../Audio/AudioComponent.hpp"
#include "GameObjectHandle.hpp"

namespace Engine::Scripting
{
    // =========================================================================
    // LuaBindings
    //
    // editor 側から LeafCallback を注入することで、
    // engine ライブラリが editor ライブラリに依存せずに計測できる。
    //
    //   LeafCallback(groupName, functionName, ms)
    //
    // GameLogicProfiler::installLeafCallback() で登録する。
    // nullptr の場合は計測なし（通常動作）。
    // =========================================================================
    class LuaBindings
    {
    public:
        // editor 側から注入する計測コールバック
        // 引数: groupName, functionName, elapsedMs
        using LeafCallback = std::function<void(
            const std::string& group,
            const std::string& func,
            float ms)>;

        void setLeafCallback(LeafCallback cb) { m_leafCallback = std::move(cb); }

        void registerBindings(sol::state& lua);

    private:
        LeafCallback m_leafCallback;

        void bindMath(sol::state& lua);
        void bindCamera(sol::state& lua);
        void bindInput(sol::state& lua);
        void bindPhysics(sol::state& lua);
        void bindAudio(sol::state& lua);
        void bindUIText(sol::state& lua);
        void bindTransform(sol::state& lua);   // LeafCallback 付き
        void bindGameObjectHandle(sol::state& lua);
    };
}