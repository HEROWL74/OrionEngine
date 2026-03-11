#pragma once
#include "Scene.hpp"

namespace Engine::World
{
    // 現在アクティブなScene（Editor / Runtime 共通）
    Scene* getActiveScene();
    void setActiveScene(Scene* scene);
}

