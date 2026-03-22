// engine/Scripting/LuaBindings.cpp
#include "LuaBindings.hpp"
#include "LuaAPI.hpp"
#include <chrono>

namespace Engine::Scripting
{
    using Clock = std::chrono::high_resolution_clock;

    static float elapsedMs(std::chrono::time_point<Clock> t0)
    {
        return std::chrono::duration<float, std::milli>(Clock::now() - t0).count();
    }

    void LuaBindings::bindMath(sol::state& lua)
    {
        lua.new_usertype<Math::Vector3>("Vector3",
            sol::constructors<Math::Vector3(), Math::Vector3(float, float, float)>(),
            "x", &Math::Vector3::x,
            "y", &Math::Vector3::y,
            "z", &Math::Vector3::z,
            "zero", &Math::Vector3::zero,
            "up", &Math::Vector3::up,
            "length", &Math::Vector3::length,
            "normalized", &Math::Vector3::normalized
        );
    }

    void LuaBindings::bindCamera(sol::state& lua)
    {
        using namespace World;
        lua.new_usertype<Camera>("Camera",
            sol::constructors<Camera()>(),
            "setPosition", &Camera::setPosition,
            "setRotation", &Camera::setRotation,
            "lookAt", &Camera::lookAt,
            "moveForward", &Camera::moveForward,
            "moveRight", &Camera::moveRight,
            "moveUp", &Camera::moveUp
        );
    }

    void LuaBindings::bindInput(sol::state& lua)
    {
        using namespace Input;
        lua.new_usertype<InputSystem>("InputSystem",
            sol::no_constructor,
            "get", &InputSystem::get,
            "isKeyW", &InputSystem::isKeyW,
            "isKeyS", &InputSystem::isKeyS,
            "isKeyA", &InputSystem::isKeyA,
            "isKeyD", &InputSystem::isKeyD,
            "isKeySpace", &InputSystem::isKeySpace,
            "isKeyWPressed", &InputSystem::isKeyWPressed,
            "isKeySPressed", &InputSystem::isKeySPressed,
            "isKeyAPressed", &InputSystem::isKeyAPressed,
            "isKeyDPressed", &InputSystem::isKeyDPressed,
            "isKeySpacePressed", &InputSystem::isKeySpacePressed,
            "isKeyWReleased", &InputSystem::isKeyWReleased,
            "isKeySReleased", &InputSystem::isKeySReleased,
            "isKeyAReleased", &InputSystem::isKeyAReleased,
            "isKeyDReleased", &InputSystem::isKeyDReleased,
            "isKeySpaceReleased", &InputSystem::isKeySpaceReleased
        );
    }

    void LuaBindings::bindPhysics(sol::state& lua)
    {
        using namespace Physics;
        lua.new_usertype<BoxCollider>("BoxCollider",
            sol::no_constructor,
            "setSize", [](BoxCollider* bc, float x, float y, float z) {
                bc->setSize(Math::Vector3(x, y, z));
            },
            "getSize", &BoxCollider::getSize,
            "setCenter", [](BoxCollider* bc, float x, float y, float z) {
                bc->setCenter(Math::Vector3(x, y, z));
            },
            "getCenter", &BoxCollider::getCenter,
            "setTrigger", &BoxCollider::setTrigger,
            "isTrigger", &BoxCollider::isTrigger
        );
    }

    void LuaBindings::bindAudio(sol::state& lua)
    {
        using namespace Audio;
        lua.new_usertype<AudioComponent>("AudioComponent",
            sol::no_constructor,
            "setFilePath", &AudioComponent::setFilePath,
            "getFilePath", &AudioComponent::getFilePath,
            "play", &AudioComponent::play,
            "stop", &AudioComponent::stop,
            "pause", &AudioComponent::pause,
            "resume", &AudioComponent::resume,
            "setLoop", &AudioComponent::setLoop,
            "isLoop", &AudioComponent::isLoop,
            "setVolume", &AudioComponent::setVolume,
            "getVolume", &AudioComponent::getVolume,
            "isPlaying", &AudioComponent::isPlaying,
            "isPaused", &AudioComponent::isPaused
        );
    }

    void LuaBindings::bindUIText(sol::state& lua)
    {
        using namespace EngineUI;
        lua.new_usertype<UIText>("UIText",
            sol::no_constructor,
            "getName", &UIText::getName,
            "setName", &UIText::setName,
            "getText", &UIText::getText,
            "setText", &UIText::setText,
            "isVisible", &UIText::isVisible,
            "setVisible", &UIText::setVisible,
            "getPosition", &UIText::getPosition,
            "setPosition", &UIText::setPosition,
            "getRotation", &UIText::getRotation,
            "setRotation", &UIText::setRotation,
            "getScale", &UIText::getScale,
            "setScale", &UIText::setScale,
            "setPositionXYZ", &UIText::setPositionXYZ,
            "setRotationXYZ", &UIText::setRotationXYZ,
            "setScaleXYZ", &UIText::setScaleXYZ,
            "getFontSize", &UIText::getFontSize,
            "setFontSize", &UIText::setFontSize,
            "getColor", &UIText::getColor,
            "setColor", &UIText::setColor,
            "setColorRGB", &UIText::setColorRGB,
            "getAlpha", &UIText::getAlpha,
            "setAlpha", &UIText::setAlpha
        );
    }

    void LuaBindings::bindGameObjectHandle(sol::state& lua)
    {
        lua.new_usertype<GameObjectHandle>(
            "GameObject",
            sol::constructors<GameObjectHandle()>(),
            "id", &GameObjectHandle::id
        );
    }

    void LuaBindings::bindTransform(sol::state& lua)
    {
        LeafCallback cb = m_leafCallback;

        // ---- デバッグ: cb が設定されているか確認 ----
        Utils::log_info(std::format("[LuaBindings] bindTransform called. cb is {}",
            cb ? "SET" : "NULL"));

        using T = Core::Transform;
        using V3 = Math::Vector3;

        lua.new_usertype<T>("Transform",
            "getPosition", &T::getPosition,
            "getRotation", &T::getRotation,
            "getScale", &T::getScale,

            "setPosition", sol::overload(
                [cb](T* t, const V3& v) {
                    auto t0 = Clock::now();
                    t->setPosition(v);
                    if (cb) cb("Transform", "setPosition", elapsedMs(t0));
                },
                [cb](T* t, float x, float y, float z) {
                    auto t0 = Clock::now();
                    t->setPosition(V3(x, y, z));
                    if (cb) cb("Transform", "setPosition", elapsedMs(t0));
                }
            ),

            "setRotation", sol::overload(
                [cb](T* t, const V3& v) {
                    auto t0 = Clock::now();
                    t->setRotation(v);
                    if (cb) cb("Transform", "setRotation", elapsedMs(t0));
                },
                [cb](T* t, float x, float y, float z) {
                    auto t0 = Clock::now();
                    t->setRotation(V3(x, y, z));
                    if (cb) cb("Transform", "setRotation", elapsedMs(t0));
                }
            ),

            "setScale", sol::overload(
                [cb](T* t, const V3& v) {
                    auto t0 = Clock::now();
                    t->setScale(v);
                    if (cb) cb("Transform", "setScale", elapsedMs(t0));
                },
                [cb](T* t, float x, float y, float z) {
                    auto t0 = Clock::now();
                    t->setScale(V3(x, y, z));
                    if (cb) cb("Transform", "setScale", elapsedMs(t0));
                }
            ),

            "translate", sol::overload(
                [cb](T* t, const V3& v) {
                    auto t0 = Clock::now();
                    t->translate(v);
                    if (cb) cb("Transform", "translate", elapsedMs(t0));
                },
                [cb](T* t, float x, float y, float z) {
                    auto t0 = Clock::now();
                    t->translate(V3(x, y, z));
                    if (cb) cb("Transform", "translate", elapsedMs(t0));
                }
            ),

            "rotate", sol::overload(
                [cb](T* t, const V3& v) {
                    auto t0 = Clock::now();
                    t->rotate(v);
                    if (cb) cb("Transform", "rotate", elapsedMs(t0));
                },
                [cb](T* t, float x, float y, float z) {
                    auto t0 = Clock::now();
                    t->rotate(V3(x, y, z));
                    if (cb) cb("Transform", "rotate", elapsedMs(t0));
                }
            ),

            "move", [cb](T* t, float x, float y, float z) {
                auto t0 = Clock::now();
                t->translate(V3(x, y, z));
                if (cb) cb("Transform", "move", elapsedMs(t0));
            },

            // moveX / moveY / moveZ: 毎フレーム呼ばれる可能性があるため
            // デバッグログは static カウンタで最初の数回だけ出す
            "moveX", [cb](T* t, float x) {
                auto t0 = Clock::now();
                auto p = t->getPosition();
                t->setPosition(V3(p.x + x, p.y, p.z));
                float ms = elapsedMs(t0);

                static int dbgCount = 0;
                if (dbgCount < 5) {
                    Utils::log_info(std::format(
                        "[LuaBindings] moveX called. cb={} ms={:.4f}",
                        cb ? "SET" : "NULL", ms));
                    ++dbgCount;
                }

                if (cb) cb("Transform", "moveX", ms);
            },

            "moveY", [cb](T* t, float y) {
                auto t0 = Clock::now();
                auto p = t->getPosition();
                t->setPosition(V3(p.x, p.y + y, p.z));
                float ms = elapsedMs(t0);
                if (cb) cb("Transform", "moveY", ms);
            },

            "moveZ", [cb](T* t, float z) {
                auto t0 = Clock::now();
                auto p = t->getPosition();
                t->setPosition(V3(p.x, p.y, p.z + z));
                float ms = elapsedMs(t0);
                if (cb) cb("Transform", "moveZ", ms);
            }
        );
    }

    void LuaBindings::registerBindings(sol::state& lua)
    {
        bindMath(lua);
        bindCamera(lua);
        bindInput(lua);
        bindPhysics(lua);
        bindAudio(lua);
        bindUIText(lua);
        bindTransform(lua);

        lua.new_usertype<Core::GameObject>("GameObject",
            "getName", &Core::GameObject::getName,
            "getTransform", &Core::GameObject::getTransform,
            "isActive", &Core::GameObject::isActive,
            "setActive", &Core::GameObject::setActive,
            "getUIText", [](Core::GameObject* obj) -> EngineUI::UIText* {
                return obj->getComponent<EngineUI::UIText>();
            },
            "getAudio", [](Core::GameObject* obj) -> Audio::AudioComponent* {
                return obj->getComponent<Audio::AudioComponent>();
            }
        );

        bindGameObjectHandle(lua);
    }

}