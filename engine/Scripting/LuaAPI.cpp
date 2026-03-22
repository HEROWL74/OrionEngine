// engine/Scripting/LuaAPI.cpp
#include "LuaAPI.hpp"
#include "../World/ActiveScene.hpp"

namespace Engine::Scripting
{
    // EntityIDを受け取るオーバーロード
    Engine::Core::GameObject* resolveGameObject(Engine::Core::EntityID id)
    {
        auto* scene = Engine::World::getActiveScene();
        if (!scene) return nullptr;
        return scene->findGameObjectById(id);
    }
}