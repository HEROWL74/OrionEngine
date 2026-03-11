// src/Graphics/SceneSerializer.hpp
#pragma once
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>
#include "Scene.hpp"
#include "../Scripting/LuaScriptComponent.hpp"
#include "../Physics/BoxCollider.hpp"
#include "../Audio/AudioComponent.hpp"
#include "../UI/UIComponent.hpp"

namespace Engine::World
{
    using json = nlohmann::json;
    using namespace Renderer;

    class SceneSerializer
    {
    public:
        SceneSerializer() = default;
        ~SceneSerializer() = default;

        [[nodiscard]] Utils::VoidResult saveScene(
            const Scene& scene,
            const std::filesystem::path& filePath);

        [[nodiscard]] Utils::VoidResult loadScene(
            Scene& scene,
            Device* device,
            ShaderManager* shaderManager,
            MaterialManager* materialManager,
            TextureManager* textureManager,
            const std::filesystem::path& filePath);

    private:
        json serializeGameObject(const Scene& scene, const Core::GameObject* gameObject);

        [[nodiscard]] Utils::VoidResult deserializeGameObject(
            Scene& scene,
            Device* device,
            ShaderManager* shaderManager,
            MaterialManager* materialManager,
            TextureManager* textureManager,
            const json& json);

        json serializeUITextComponent(const EngineUI::UIText* text);
        void deserializeUITextComponent(EngineUI::UIText* text, const json& json);

        json serializeTransform(const Core::Transform* transform);
        void deserializeTransform(Core::Transform* transform, const json& json);

        // RenderComponentは廃止。RenderEntryをシリアライズする。
        json serializeRenderEntry(const RenderEntry& entry, const Scene& scene);
        [[nodiscard]] Utils::VoidResult deserializeRenderComponent(
            Core::GameObject* gameObject,
            Scene& scene,
            Device* device,
            ShaderManager* shaderManager,
            MaterialManager* materialManager,
            TextureManager* textureManager,
            const json& json);

        json serializeMaterial(const Material* material);
        json serializeLuaComponent(const Scripting::LuaScriptComponent* component);
        json serializeBoxCollider(const Physics::BoxCollider* collider);
        void deserializeBoxCollider(Physics::BoxCollider* collider, const json& json);
        json serializeAudioComponent(const Audio::AudioComponent* audioComponent);
        void deserializeAudioComponent(Audio::AudioComponent* audioComponent, const json& json);
    };
}