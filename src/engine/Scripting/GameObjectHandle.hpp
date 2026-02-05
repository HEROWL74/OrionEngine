#pragma once
#include <cstdint>

namespace Engine::Scripting {

    struct GameObjectHandle {
        uint64_t id = 0;

        bool isValid() const { return id != 0; }
    };
}
