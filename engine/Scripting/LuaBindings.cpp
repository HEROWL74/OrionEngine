#include "LuaBindings.hpp"
#include "LuaAPI.hpp"

namespace Engine::Scripting
{
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
		using namespace Graphics;

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

			// 押されている間ずっとtrue (連続入力用)
			"isKeyW", &InputSystem::isKeyW,
			"isKeyS", &InputSystem::isKeyS,
			"isKeyA", &InputSystem::isKeyA,
			"isKeyD", &InputSystem::isKeyD,
			"isKeySpace", &InputSystem::isKeySpace,

			// 押された瞬間だけtrue (トグル用)
			"isKeyWPressed", &InputSystem::isKeyWPressed,
			"isKeySPressed", &InputSystem::isKeySPressed,
			"isKeyAPressed", &InputSystem::isKeyAPressed,
			"isKeyDPressed", &InputSystem::isKeyDPressed,
			"isKeySpacePressed", &InputSystem::isKeySpacePressed,

			// 離された瞬間だけtrue
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

		lua.new_usertype<Audio::AudioComponent>("AudioComponent",
			sol::no_constructor,
			// ファイルパス設定（Inspector経由で設定）
			"setFilePath", &AudioComponent::setFilePath,
			"getFilePath", &AudioComponent::getFilePath,

			// 再生制御
			"play", &AudioComponent::play,
			"stop", &AudioComponent::stop,
			"pause", &AudioComponent::pause,
			"resume", &AudioComponent::resume,

			// 設定
			"setLoop", &AudioComponent::setLoop,
			"isLoop", &AudioComponent::isLoop,
			"setVolume", &AudioComponent::setVolume,
			"getVolume", &AudioComponent::getVolume,

			// 状態取得
			"isPlaying", &AudioComponent::isPlaying,
			"isPaused", &AudioComponent::isPaused
		);
	}

	void LuaBindings::bindUIText(sol::state& lua)
	{
		using namespace EngineUI;

		lua.new_usertype<UIText>("UIText",
			sol::no_constructor,
			// 基本プロパティ
			"getName", &UIText::getName,
			"setName", &UIText::setName,
			"getText", &UIText::getText,
			"setText", &UIText::setText,

			// 表示制御
			"isVisible", &UIText::isVisible,
			"setVisible", &UIText::setVisible,

			// Transform (Vector3版)
			"getPosition", &UIText::getPosition,
			"setPosition", &UIText::setPosition,
			"getRotation", &UIText::getRotation,
			"setRotation", &UIText::setRotation,
			"getScale", &UIText::getScale,
			"setScale", &UIText::setScale,

			// Transform (float x, y, z版 - Luaから使いやすい)
			"setPositionXYZ", &UIText::setPositionXYZ,
			"setRotationXYZ", &UIText::setRotationXYZ,
			"setScaleXYZ", &UIText::setScaleXYZ,

			// スタイル
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

	void LuaBindings::registerBindings(sol::state& lua)
	{
		bindMath(lua);
		bindCamera(lua);
		bindInput(lua);
		bindPhysics(lua);
		bindUIText(lua);
		bindAudio(lua);

		// GameObject バインディング
		lua.new_usertype<Core::GameObject>("GameObject",
			"getName", &Core::GameObject::getName,
			"getTransform", &Core::GameObject::getTransform,
			"isActive", &Core::GameObject::isActive,
			"setActive", &Core::GameObject::setActive,
			// UITextコンポーネントを取得
			"getUIText", [](Core::GameObject* obj) -> EngineUI::UIText* {
				return obj->getComponent<EngineUI::UIText>();
			},
			// AudioComponentを取得
			"getAudio", [](Core::GameObject* obj) -> Audio::AudioComponent* {
				return obj->getComponent<Audio::AudioComponent>();
			}
		);

		// Transform バインディング
		lua.new_usertype<Core::Transform>("Transform",
			"getPosition", &Core::Transform::getPosition,
			"setPosition", sol::overload(
				&Core::Transform::setPosition,
				[](Core::Transform* t, float x, float y, float z) {
					t->setPosition(Math::Vector3(x, y, z));
				}
			),
			"getRotation", &Core::Transform::getRotation,
			"setRotation", sol::overload(
				&Core::Transform::setRotation,
				[](Core::Transform* t, float x, float y, float z) {
					t->setRotation(Math::Vector3(x, y, z));
				}
			),
			"getScale", &Core::Transform::getScale,
			"setScale", sol::overload(
				&Core::Transform::setScale,
				[](Core::Transform* t, float x, float y, float z) {
					t->setScale(Math::Vector3(x, y, z));
				}
			),
			"translate", sol::overload(
				&Core::Transform::translate,
				[](Core::Transform* t, float x, float y, float z) {
					t->translate(Math::Vector3(x, y, z));
				}
			),
			"rotate", sol::overload(
				&Core::Transform::rotate,
				[](Core::Transform* t, float x, float y, float z) {
					t->rotate(Math::Vector3(x, y, z));
				}
			),
			"move", [](Core::Transform* t, float x, float y, float z) {
				t->translate(Math::Vector3(x, y, z));
			},
			"moveX", [](Core::Transform* t, float x) {
				auto pos = t->getPosition();
				t->setPosition(Math::Vector3(pos.x + x, pos.y, pos.z));
			},
			"moveY", [](Core::Transform* t, float y) {
				auto pos = t->getPosition();
				t->setPosition(Math::Vector3(pos.x, pos.y + y, pos.z));
			},
			"moveZ", [](Core::Transform* t, float z) {
				auto pos = t->getPosition();
				t->setPosition(Math::Vector3(pos.x, pos.y, pos.z + z));
			}
		);
	}
}

