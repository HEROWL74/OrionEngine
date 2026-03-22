// engine/World/ActiveScene.cpp
#include "ActiveScene.hpp"

namespace Engine::World
{
    static Scene* g_activeScene = nullptr;

    Scene* getActiveScene()
    {
        return g_activeScene;
    }

    void setActiveScene(Scene* scene)
    {
        g_activeScene = scene;
    }
}

