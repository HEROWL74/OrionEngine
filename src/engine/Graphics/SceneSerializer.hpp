// src/engine/Graphics/SceneSerializer.hpp
#pragma once

#include <string>
#include <fstream>
#include <nlohmann/json.hpp>
#include "Scene.hpp"
#include "../Scripting/LuaScriptComponent.hpp"
#include "../Physics/BoxCollider.hpp"
#include "../UI/UIComponent.hpp"

namespace Engine::Graphics
{
	using json = nlohmann::json;

	// ===============================================
	// Sceneのシリアライズ、デシリアライズを行うクラス
	// ===============================================
	class SceneSerializer
	{
	public:
		SceneSerializer() = default;
		~SceneSerializer() = default;

		// SceneをJSONに保存
		[[nodiscard]] Utils::VoidResult saveScene(
			const Scene& scene,
			const std::string& filePath);

		// JSONからSceneを読み込み
		[[nodiscard]] Utils::VoidResult loadScene(
			Scene& scene,
			Device* device,
			ShaderManager* shaderManager,
			MaterialManager* materialManager,
			TextureManager* textureManager,
			const std::string& filePath);

	private:
		// GameObjectをJSON化
		json serializeGameObject(const Core::GameObject* gameObject);

		// JSONからGameObjectを復元
		[[nodiscard]] Utils::VoidResult deserializeGameObject(
			Scene& scene,
			Device* device,
			ShaderManager* shaderManager,
			MaterialManager* materialManager,
			TextureManager* textureManager,
			const json& json
		);

		// UITextコンポーネントのシリアライズ
		json serializeUITextComponent(const EngineUI::UIText* text);
		void deserializeUITextComponent(EngineUI::UIText* text, const json& json);

		// Transform情報のシリアライズ
		json serializeTransform(const Core::Transform* transform);
		void deserializeTransform(Core::Transform* transform, const json& json);

		// RenderComponent情報のシリアライズ
		json serializeRenderComponent(const RenderComponent* renderComponent);

		[[nodiscard]] Utils::VoidResult deserializeRenderComponent(
			Core::GameObject* gameObject,
			Device* device,
			ShaderManager* shaderManager,
			MaterialManager* materialManager,
			TextureManager* textureManager,
			const json& json);

		// Material情報のシリアライズ
		json serializeMaterial(const Material* material);

		// Luaスクリプト情報のシリアライズ
		json serializeLuaComponent(const Scripting::LuaScriptComponent* component);

		// BoxCollider情報のシリアライズ
		json serializeBoxCollider(const Physics::BoxCollider* collider);
		void deserializeBoxCollider(Physics::BoxCollider* collider, const json& json);
	};
}