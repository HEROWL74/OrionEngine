// engine/Scripting/LuaAPI.hpp
#pragma once
#include "../Core/EntityID.hpp"
#include "../Core/GameObject.hpp"

namespace Engine::Scripting
{
    Engine::Core::GameObject* resolveGameObject(Engine::Core::EntityID id);
}