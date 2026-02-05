#pragma once
#include <cstdint>

namespace Engine {
    namespace Core { class GameObject; }
    namespace Scripting {

        Core::GameObject* resolveGameObject(uint64_t id);

    }
}
