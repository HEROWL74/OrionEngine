#include "LuaAPI.hpp"
#include "../Graphics/ActiveScene.hpp"

namespace Engine::Scripting
{
    Engine::Core::GameObject* resolveGameObject(uint64_t id)
    {
        auto* scene = Engine::Graphics::getActiveScene();
        if (!scene)
            return nullptr;

        return scene->findGameObjectById(
            static_cast<Engine::Core::GameObject::ObjectID>(id)
        );
    }
}