// src/engine/Graphics/SceneSerializer.hpp
#pragma once

#include <string>
#include <fstream>
#include <nlohmann/json.hpp>
#include "Scene.hpp"
#include "../Scripting/LuaScriptComponent.hpp"
#include "../Physics/BoxCollider.hpp"
#include "../Audio/AudioComponent.hpp"
#include "../UI/UIComponent.hpp"

namespace Engine::Graphics
{
	using json = nlohmann::json;

	// ===============================================
	// SceneSerializer
	// ===============================================
	class SceneSerializer
	{
	public:
		SceneSerializer() = default;
		~SceneSerializer() = default;

		// Scene Save
		[[nodiscard]] Utils::VoidResult saveScene(
			const Scene& scene,
			const std::filesystem::path& filePath);

		// JSON load
		[[nodiscard]] Utils::VoidResult loadScene(
			Scene& scene,
			Device* device,
			ShaderManager* shaderManager,
			MaterialManager* materialManager,
			TextureManager* textureManager,
			const std::filesystem::path& filePath);

	private:
		// GameObject Serialize
		json serializeGameObject(const Core::GameObject* gameObject);

		// JSON縺九ｉGameObject繧貞ｾｩ蜈・
		[[nodiscard]] Utils::VoidResult deserializeGameObject(
			Scene& scene,
			Device* device,
			ShaderManager* shaderManager,
			MaterialManager* materialManager,
			TextureManager* textureManager,
			const json& json
		);

		// UIText Serialize & Deserialzie
		json serializeUITextComponent(const EngineUI::UIText* text);
		void deserializeUITextComponent(EngineUI::UIText* text, const json& json);

		// Transform Serialize & Deserialzie
		json serializeTransform(const Core::Transform* transform);
		void deserializeTransform(Core::Transform* transform, const json& json);

		// RenderComponent Serialize
		json serializeRenderComponent(const RenderComponent* renderComponent);

		[[nodiscard]] Utils::VoidResult deserializeRenderComponent(
			Core::GameObject* gameObject,
			Device* device,
			ShaderManager* shaderManager,
			MaterialManager* materialManager,
			TextureManager* textureManager,
			const json& json);

		// Material Serialize
		json serializeMaterial(const Material* material);

		// Lua Serialize
		json serializeLuaComponent(const Scripting::LuaScriptComponent* component);

		// BoxCollider Serialize & Deserialzie
		json serializeBoxCollider(const Physics::BoxCollider* collider);
		void deserializeBoxCollider(Physics::BoxCollider* collider, const json& json);

		// AudioComponent Serialize & Deserialzie
		json serializeAudioComponent(const Audio::AudioComponent* audioComponent);
		void deserializeAudioComponent(Audio::AudioComponent* audioComponent, const json& json);
	};
}

