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
	// Scene縺ｮ繧ｷ繝ｪ繧｢繝ｩ繧､繧ｺ縲√ョ繧ｷ繝ｪ繧｢繝ｩ繧､繧ｺ繧定｡後≧繧ｯ繝ｩ繧ｹ
	// ===============================================
	class SceneSerializer
	{
	public:
		SceneSerializer() = default;
		~SceneSerializer() = default;

		// Scene繧谷SON縺ｫ菫晏ｭ・
		[[nodiscard]] Utils::VoidResult saveScene(
			const Scene& scene,
			const std::string& filePath);

		// JSON縺九ｉScene繧定ｪｭ縺ｿ霎ｼ縺ｿ
		[[nodiscard]] Utils::VoidResult loadScene(
			Scene& scene,
			Device* device,
			ShaderManager* shaderManager,
			MaterialManager* materialManager,
			TextureManager* textureManager,
			const std::string& filePath);

	private:
		// GameObject繧谷SON蛹・
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

		// UIText繧ｳ繝ｳ繝昴・繝阪Φ繝医・繧ｷ繝ｪ繧｢繝ｩ繧､繧ｺ
		json serializeUITextComponent(const EngineUI::UIText* text);
		void deserializeUITextComponent(EngineUI::UIText* text, const json& json);

		// Transform諠・ｱ縺ｮ繧ｷ繝ｪ繧｢繝ｩ繧､繧ｺ
		json serializeTransform(const Core::Transform* transform);
		void deserializeTransform(Core::Transform* transform, const json& json);

		// RenderComponent諠・ｱ縺ｮ繧ｷ繝ｪ繧｢繝ｩ繧､繧ｺ
		json serializeRenderComponent(const RenderComponent* renderComponent);

		[[nodiscard]] Utils::VoidResult deserializeRenderComponent(
			Core::GameObject* gameObject,
			Device* device,
			ShaderManager* shaderManager,
			MaterialManager* materialManager,
			TextureManager* textureManager,
			const json& json);

		// Material諠・ｱ縺ｮ繧ｷ繝ｪ繧｢繝ｩ繧､繧ｺ
		json serializeMaterial(const Material* material);

		// Lua繧ｹ繧ｯ繝ｪ繝励ヨ諠・ｱ縺ｮ繧ｷ繝ｪ繧｢繝ｩ繧､繧ｺ
		json serializeLuaComponent(const Scripting::LuaScriptComponent* component);

		// BoxCollider諠・ｱ縺ｮ繧ｷ繝ｪ繧｢繝ｩ繧､繧ｺ & 繝・す繝ｪ繧｢繝ｩ繧､繧ｺ
		json serializeBoxCollider(const Physics::BoxCollider* collider);
		void deserializeBoxCollider(Physics::BoxCollider* collider, const json& json);

		// AudioComponent諠・ｱ縺ｮ繧ｷ繝ｪ繧｢繝ｩ繧､繧ｺ & 繝・す繝ｪ繧｢繝ｩ繧､繧ｺ
		json serializeAudioComponent(const Audio::AudioComponent* audioComponent);
		void deserializeAudioComponent(Audio::AudioComponent* audioComponent, const json& json);
	};
}

