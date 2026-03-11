// src/Graphics/SceneSerializer.cpp
#include "SceneSerializer.hpp"
#include <fstream>

namespace Engine::Graphics
{
    Utils::VoidResult SceneSerializer::saveScene(
        const Scene& scene,
        const std::filesystem::path& filepath)
    {
        try
        {
            nlohmann::json sceneJson;
            sceneJson["version"] = "1.0";
            sceneJson["gameObjects"] = nlohmann::json::array();

            for (const auto& gameObject : scene.getGameObjects())
            {
                if (gameObject && !gameObject->isDestroyed())
                {
                    sceneJson["gameObjects"].push_back(
                        serializeGameObject(scene, gameObject.get())
                    );
                }
            }

            std::ofstream file(filepath);
            if (!file.is_open())
            {
                return std::unexpected(Utils::make_error(
                    Utils::ErrorType::FileI0,
                    "Failed to open file for writing: " + filepath.string()
                ));
            }

            file << sceneJson.dump(4);
            file.close();

            Utils::log_info(std::format("Scene saved to: {} ({} objects)",
                filepath.string(), sceneJson["gameObjects"].size()));
            return {};
        }
        catch (const std::exception& e)
        {
            return std::unexpected(Utils::make_error(
                Utils::ErrorType::Unknown,
                std::format("Failed to save scene: {}", e.what())
            ));
        }
    }

    Utils::VoidResult SceneSerializer::loadScene(
        Scene& scene,
        Device* device,
        ShaderManager* shaderManager,
        MaterialManager* materialManager,
        TextureManager* textureManager,
        const std::filesystem::path& filePath)
    {
        try
        {
            std::ifstream file(filePath);
            if (!file.is_open())
            {
                return std::unexpected(Utils::make_error(
                    Utils::ErrorType::FileI0,
                    "Failed to open file for reading: " + filePath.string()
                ));
            }

            scene.clear();

            nlohmann::json sceneJson;
            file >> sceneJson;
            file.close();

            if (!sceneJson.contains("version"))
            {
                return std::unexpected(Utils::make_error(
                    Utils::ErrorType::Unknown,
                    "Invalid scene file: missing version"
                ));
            }

            if (sceneJson.contains("gameObjects"))
            {
                for (const auto& objJson : sceneJson["gameObjects"])
                {
                    auto result = deserializeGameObject(
                        scene, device, shaderManager,
                        materialManager, textureManager, objJson
                    );

                    if (!result)
                    {
                        Utils::log_warning(std::format(
                            "Failed to load GameObject: {}",
                            result.error().message
                        ));
                    }
                }
            }

            Utils::log_info(std::format("Scene loaded from: {} ({} objects)",
                filePath.string(),
                sceneJson.value("gameObjects", nlohmann::json::array()).size()));
            return {};
        }
        catch (const std::exception& e)
        {
            return std::unexpected(Utils::make_error(
                Utils::ErrorType::Unknown,
                std::format("Failed to load scene: {}", e.what())
            ));
        }
    }

    // -------------------------------------------------------
    // serialize
    // -------------------------------------------------------

    nlohmann::json SceneSerializer::serializeGameObject(
        const Scene& scene,
        const Core::GameObject* gameObject)
    {
        nlohmann::json j;

        j["name"] = gameObject->getName();
        j["active"] = gameObject->isActive();
        j["transform"] = serializeTransform(gameObject->getTransform());

        // RenderBatchSystemからエントリを取得してシリアライズ
        const auto* entry = scene.getRenderBatch().findEntry(gameObject->getId());
        if (entry)
            j["renderComponent"] = serializeRenderEntry(*entry, scene);

        // ComponentBatchSystemからコンポーネントを取得
        const auto& batch = scene.getComponentBatch();

        if (auto* uiText = batch.get<EngineUI::UIText>(gameObject->getId()))
            j["uiText"] = serializeUITextComponent(uiText);

        if (auto* lua = batch.get<Scripting::LuaScriptComponent>(gameObject->getId()))
            j["lua"] = serializeLuaComponent(lua);

        if (auto* box = batch.get<Physics::BoxCollider>(gameObject->getId()))
            j["boxCollider"] = serializeBoxCollider(box);

        if (auto* audio = batch.get<Audio::AudioComponent>(gameObject->getId()))
            j["audioComponent"] = serializeAudioComponent(audio);

        return j;
    }

    nlohmann::json SceneSerializer::serializeRenderEntry(
        const RenderEntry& entry, const Scene& /*scene*/)
    {
        nlohmann::json j;
        j["renderableType"] = static_cast<int>(entry.type);
        j["visible"] = entry.visible;

        if (entry.material)
            j["material"] = serializeMaterial(entry.material.get());

        return j;
    }

    // -------------------------------------------------------
    // deserialize
    // -------------------------------------------------------

    Utils::VoidResult SceneSerializer::deserializeGameObject(
        Scene& scene,
        Device* device,
        ShaderManager* shaderManager,
        MaterialManager* materialManager,
        TextureManager* textureManager,
        const nlohmann::json& json)
    {
        std::string name = json.value("name", "GameObject");
        Utils::log_info(std::format("Loading GameObject: {}", name));

        Core::GameObject* gameObject = nullptr;

        if (json.contains("uiText"))
        {
            auto* uiText = scene.createUIText(name);
            if (!uiText)
                return std::unexpected(Utils::make_error(
                    Utils::ErrorType::Unknown, "Failed to create UIText"));

            gameObject = uiText->getGameObject();
            deserializeUITextComponent(uiText, json["uiText"]);
            Utils::log_info(std::format("  UIText component loaded for {}", name));
        }
        else
        {
            gameObject = scene.createGameObject(name);
            if (!gameObject)
                return std::unexpected(Utils::make_error(
                    Utils::ErrorType::Unknown, "Failed to create GameObject"));
        }

        if (json.contains("transform"))
        {
            deserializeTransform(gameObject->getTransform(), json["transform"]);
            Utils::log_info(std::format("  Transform loaded for {}", name));
        }

        if (json.contains("renderComponent"))
        {
            Utils::log_info(std::format("  Loading RenderComponent for {}", name));
            auto result = deserializeRenderComponent(
                gameObject, scene, device, shaderManager,
                materialManager, textureManager,
                json["renderComponent"]);
            if (!result) return result;
            Utils::log_info(std::format("  RenderComponent loaded for {}", name));
        }

        if (json.contains("lua"))
        {
            const auto& luaJson = json["lua"];
            std::string scriptPath = luaJson.value("scriptPath", "");

            if (!scriptPath.empty())
            {
                std::replace(scriptPath.begin(), scriptPath.end(), '\\', '/');
                std::string lower = scriptPath;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (lower.rfind("assets/", 0) == 0)
                    scriptPath = "Assets/" + scriptPath.substr(7);

                scene.addComponent<Scripting::LuaScriptComponent>(gameObject, scriptPath);
                Utils::log_info(std::format("  LuaScriptComponent loaded: {}", scriptPath));
            }
        }

        if (json.contains("boxCollider"))
        {
            auto* box = scene.addComponent<Physics::BoxCollider>(gameObject);
            deserializeBoxCollider(box, json["boxCollider"]);
        }

        if (json.contains("audioComponent"))
        {
            auto* audio = scene.addComponent<Audio::AudioComponent>(gameObject);
            if (audio)
            {
                auto initResult = audio->initialize();
                if (initResult)
                {
                    deserializeAudioComponent(audio, json["audioComponent"]);
                    Utils::log_info("  AudioComponent loaded");
                }
                else
                {
                    Utils::log_warning("Failed to initialize AudioComponent");
                }
            }
        }

        gameObject->setActive(json.value("active", true));
        Utils::log_info(std::format("GameObject {} created successfully", name));
        return {};
    }

    Utils::VoidResult SceneSerializer::deserializeRenderComponent(
        Core::GameObject* gameObject,
        Scene& scene,
        Device* device,
        ShaderManager* shaderManager,
        MaterialManager* materialManager,
        TextureManager* textureManager,
        const nlohmann::json& json)
    {
        auto renderType = static_cast<RenderableType>(json.value("renderableType", 0));
        bool visible = json.value("visible", true);

        std::shared_ptr<Material> material;

        if (json.contains("material"))
        {
            const auto& matJson = json["material"];
            std::string matName = gameObject->getName() + "_" + matJson.value("name", "Material");

            material = materialManager->createMaterial(matName);
            if (material)
            {
                if (matJson.contains("properties"))
                {
                    MaterialProperties props;
                    const auto& propsJson = matJson["properties"];

                    if (propsJson.contains("albedo"))
                    {
                        auto albedo = propsJson["albedo"];
                        props.albedo = Math::Vector3(
                            albedo[0].get<float>(),
                            albedo[1].get<float>(),
                            albedo[2].get<float>());
                    }
                    props.metallic = propsJson.value("metallic", 0.0f);
                    props.roughness = propsJson.value("roughness", 0.5f);
                    material->setProperties(props);
                }

                if (matJson.contains("textures"))
                {
                    const auto& textures = matJson["textures"];
                    if (textures.contains("albedo"))
                    {
                        std::string texPath = textures["albedo"];
                        auto texture = textureManager->loadTexture(texPath, true, true);
                        if (texture)
                            material->setTexture(TextureType::Albedo, texture);
                    }
                }
            }
        }

        // RenderComponentの代わりにRenderBatchSystemへ登録する
        scene.registerRenderable(gameObject, renderType, material);
        scene.setRenderableVisible(gameObject, visible);

        return {};
    }

    // -------------------------------------------------------
    // 以下は変更なし
    // -------------------------------------------------------

    nlohmann::json SceneSerializer::serializeUITextComponent(const EngineUI::UIText* text)
    {
        nlohmann::json j;
        j["text"] = text->getText();
        j["visible"] = text->isVisible();
        j["fontSize"] = text->getFontSize();
        auto color = text->getColor();
        j["color"] = { color.x, color.y, color.z };
        j["alpha"] = text->getAlpha();
        return j;
    }

    void SceneSerializer::deserializeUITextComponent(
        EngineUI::UIText* text, const nlohmann::json& json)
    {
        text->setText(json.value("text", "Text"));
        text->setVisible(json.value("visible", true));
        text->setFontSize(json.value("fontSize", 32.0f));

        if (json.contains("color"))
        {
            auto color = json["color"];
            text->setColor(Math::Vector3(
                color[0].get<float>(), color[1].get<float>(), color[2].get<float>()));
        }
        text->setAlpha(json.value("alpha", 1.0f));
    }

    nlohmann::json SceneSerializer::serializeTransform(const Core::Transform* transform)
    {
        nlohmann::json j;
        auto pos = transform->getPosition();
        auto rot = transform->getRotation();
        auto scale = transform->getScale();
        j["position"] = { pos.x,   pos.y,   pos.z };
        j["rotation"] = { rot.x,   rot.y,   rot.z };
        j["scale"] = { scale.x, scale.y, scale.z };
        return j;
    }

    void SceneSerializer::deserializeTransform(
        Core::Transform* transform, const nlohmann::json& json)
    {
        if (json.contains("position"))
        {
            auto p = json["position"];
            transform->setPosition({ p[0].get<float>(), p[1].get<float>(), p[2].get<float>() });
        }
        if (json.contains("rotation"))
        {
            auto r = json["rotation"];
            transform->setRotation({ r[0].get<float>(), r[1].get<float>(), r[2].get<float>() });
        }
        if (json.contains("scale"))
        {
            auto s = json["scale"];
            transform->setScale({ s[0].get<float>(), s[1].get<float>(), s[2].get<float>() });
        }
    }

    nlohmann::json SceneSerializer::serializeMaterial(const Material* material)
    {
        nlohmann::json j;
        j["name"] = "Material";
        auto props = material->getProperties();
        j["properties"]["albedo"] = { props.albedo.x, props.albedo.y, props.albedo.z };
        j["properties"]["metallic"] = props.metallic;
        j["properties"]["roughness"] = props.roughness;
        j["textures"] = nlohmann::json::object();
        return j;
    }

    nlohmann::json SceneSerializer::serializeLuaComponent(
        const Scripting::LuaScriptComponent* component)
    {
        nlohmann::json j;
        j["name"] = "Lua";
        std::string path = component->getScriptPath();
        std::replace(path.begin(), path.end(), '\\', '/');
        std::string lower = path;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.rfind("assets/", 0) == 0)
            path = "Assets/" + path.substr(7);
        j["scriptPath"] = path;
        return j;
    }

    nlohmann::json SceneSerializer::serializeBoxCollider(
        const Physics::BoxCollider* collider)
    {
        nlohmann::json j;
        const auto& size = collider->getSize();
        const auto& center = collider->getCenter();
        j["size"] = { size.x,   size.y,   size.z };
        j["center"] = { center.x, center.y, center.z };
        j["isTrigger"] = collider->isTrigger();
        return j;
    }

    void SceneSerializer::deserializeBoxCollider(
        Physics::BoxCollider* collider, const nlohmann::json& json)
    {
        if (json.contains("size"))
        {
            auto& s = json["size"];
            collider->setSize({ s[0].get<float>(), s[1].get<float>(), s[2].get<float>() });
        }
        if (json.contains("center"))
        {
            auto& c = json["center"];
            collider->setCenter({ c[0].get<float>(), c[1].get<float>(), c[2].get<float>() });
        }
        collider->setTrigger(json.value("isTrigger", true));
    }

    json SceneSerializer::serializeAudioComponent(
        const Audio::AudioComponent* audioComponent)
    {
        json j;
        std::string audioPath = audioComponent->getFilePath();
        std::replace(audioPath.begin(), audioPath.end(), '\\', '/');
        std::string lower = audioPath;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.rfind("assets/", 0) == 0)
            audioPath = "Assets/" + audioPath.substr(7);
        j["filePath"] = audioPath;
        j["loop"] = audioComponent->isLoop();
        j["volume"] = audioComponent->getVolume();
        return j;
    }

    void SceneSerializer::deserializeAudioComponent(
        Audio::AudioComponent* audioComponent, const nlohmann::json& json)
    {
        std::string filePath = json.value("filePath", "");
        if (!filePath.empty())
        {
            auto loadResult = audioComponent->loadAudio(filePath);
            if (!loadResult)
                Utils::log_warning(std::format("Failed to load audio: {}", filePath));
        }
        audioComponent->setLoop(json.value("loop", false));
        audioComponent->setVolume(json.value("volume", 1.0f));
    }
}