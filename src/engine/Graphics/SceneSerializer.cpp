// src/engine/Graphics/SceneSerializer.cpp
#include "SceneSerializer.hpp"
#include <fstream>

namespace Engine::Graphics
{
    Utils::VoidResult SceneSerializer::saveScene(
        const Scene& scene,
        const std::string& filepath)
    {
        try
        {
            nlohmann::json sceneJson;
            sceneJson["version"] = "1.0";
            sceneJson["gameObjects"] = nlohmann::json::array();

            // ★ 修正: isDestroyed()をチェックして、破棄済みオブジェクトを除外
            for (const auto& gameObject : scene.getGameObjects())
            {
                if (gameObject && !gameObject->isDestroyed())
                {
                    sceneJson["gameObjects"].push_back(
                        serializeGameObject(gameObject.get())
                    );
                }
            }

            // ファイルに書き込み
            std::ofstream file(filepath);
            if (!file.is_open())
            {
                return std::unexpected(Utils::make_error(
                    Utils::ErrorType::FileI0,
                    "Failed to open file for writing: " + filepath
                ));
            }

            file << sceneJson.dump(4);
            file.close();

            Utils::log_info(std::format("Scene saved to: {} ({} objects)",
                filepath, sceneJson["gameObjects"].size()));
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
        const std::string& filepath)
    {
        try
        {
            std::ifstream file(filepath);
            if (!file.is_open())
            {
                return std::unexpected(Utils::make_error(
                    Utils::ErrorType::FileI0,
                    "Failed to open file for reading: " + filepath
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
                filepath, sceneJson["gameObjects"].size()));
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

    nlohmann::json SceneSerializer::serializeGameObject(
        const Core::GameObject* gameObject)
    {
        nlohmann::json json;

        json["name"] = gameObject->getName();
        json["active"] = gameObject->isActive();

        // Transform
        json["transform"] = serializeTransform(gameObject->getTransform());

        // RenderComponent
        auto* renderComponent = gameObject->getComponent<RenderComponent>();
        if (renderComponent)
        {
            json["renderComponent"] = serializeRenderComponent(renderComponent);
        }

        // Luaスクリプト情報
        auto* lua = gameObject->getComponent<Engine::Scripting::LuaScriptComponent>();
        if (lua)
        {
            json["lua"] = serializeLuaComponent(lua);
        }

        // BoxCollider情報
        if (auto* box = gameObject->getComponent<Physics::BoxCollider>())
        {
            json["boxCollider"] = serializeBoxCollider(box);
        }

        return json;
    }

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

        auto* gameObject = scene.createGameObject(name);

        if (!gameObject)
        {
            return std::unexpected(Utils::make_error(
                Utils::ErrorType::Unknown,
                "Failed to create GameObject"
            ));
        }

        // Transformを復元
        if (json.contains("transform"))
        {
            deserializeTransform(gameObject->getTransform(), json["transform"]);
            Utils::log_info(std::format("  Transform loaded for {}", name));
        }

        // RenderComponentを復元
        if (json.contains("renderComponent"))
        {
            Utils::log_info(std::format("  Loading RenderComponent for {}", name));
            auto result = deserializeRenderComponent(
                gameObject, device, shaderManager,
                materialManager, textureManager,
                json["renderComponent"]
            );

            if (!result)
            {
                return result;
            }
            Utils::log_info(std::format("  RenderComponent loaded for {}", name));
        }

        if (json.contains("lua"))
        {
            const auto& luaJson = json["lua"];
            std::string scriptPath = luaJson.value("scriptPath", "");

            if (!scriptPath.empty())
            {
                auto* luaComponent = gameObject->addComponent<Scripting::LuaScriptComponent>(
                    scriptPath
                );

                if (luaComponent)
                {
                    Utils::log_info(std::format("  LuaScriptComponent loaded: {}", scriptPath));
                }
                else
                {
                    Utils::log_warning("Failed to add LuaScriptComponent");
                }
            }
        }

        if (json.contains("boxCollider"))
        {
            auto* box = gameObject->addComponent<Physics::BoxCollider>();
            deserializeBoxCollider(box, json["boxCollider"]);
        }

        bool isActive = json.value("active", true);
        gameObject->setActive(isActive);
        Utils::log_info(std::format("GameObject {} created successfully (active: {})", name, isActive));

        return {};
    }

    nlohmann::json SceneSerializer::serializeTransform(
        const Core::Transform* transform)
    {
        nlohmann::json json;

        auto pos = transform->getPosition();
        json["position"] = { pos.x, pos.y, pos.z };

        auto rot = transform->getRotation();
        json["rotation"] = { rot.x, rot.y, rot.z };

        auto scale = transform->getScale();
        json["scale"] = { scale.x, scale.y, scale.z };

        return json;
    }

    void SceneSerializer::deserializeTransform(
        Core::Transform* transform,
        const nlohmann::json& json)
    {
        if (json.contains("position"))
        {
            auto pos = json["position"];
            transform->setPosition(Math::Vector3(
                pos[0].get<float>(),
                pos[1].get<float>(),
                pos[2].get<float>()
            ));
        }

        if (json.contains("rotation"))
        {
            auto rot = json["rotation"];
            transform->setRotation(Math::Vector3(
                rot[0].get<float>(),
                rot[1].get<float>(),
                rot[2].get<float>()
            ));
        }

        if (json.contains("scale"))
        {
            auto scale = json["scale"];
            transform->setScale(Math::Vector3(
                scale[0].get<float>(),
                scale[1].get<float>(),
                scale[2].get<float>()
            ));
        }
    }

    nlohmann::json SceneSerializer::serializeRenderComponent(
        const RenderComponent* component)
    {
        nlohmann::json json;

        json["renderableType"] = static_cast<int>(component->getRenderableType());
        json["visible"] = component->isVisible();

        auto material = component->getMaterial();
        if (material)
        {
            json["material"] = serializeMaterial(material.get());
        }

        return json;
    }

    Utils::VoidResult SceneSerializer::deserializeRenderComponent(
        Core::GameObject* gameObject,
        Device* device,
        ShaderManager* shaderManager,
        MaterialManager* materialManager,
        TextureManager* textureManager,
        const nlohmann::json& json)
    {
        auto renderType = static_cast<RenderableType>(
            json.value("renderableType", 0)
            );

        Utils::log_info(std::format("    RenderableType: {}", static_cast<int>(renderType)));

        auto* renderComponent = gameObject->addComponent<RenderComponent>(renderType);
        if (!renderComponent)
        {
            return std::unexpected(Utils::make_error(
                Utils::ErrorType::Unknown,
                "Failed to add RenderComponent"
            ));
        }

        renderComponent->setMaterialManager(materialManager);

        Utils::log_info("Initializing RenderComponent...");
        auto initResult = renderComponent->initialize(device, shaderManager);
        if (!initResult)
        {
            return initResult;
        }
        Utils::log_info("RenderComponent initialized");

        if (json.contains("material"))
        {
            const auto& matJson = json["material"];

            std::string baseName = matJson.value("name", "Material");
            std::string matName = gameObject->getName() + "_" + baseName;

            Utils::log_info(std::format("Creating material: {}", matName));

            auto material = materialManager->createMaterial(matName);
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
                            albedo[2].get<float>()
                        );
                        Utils::log_info(std::format("      Albedo: ({}, {}, {})",
                            props.albedo.x, props.albedo.y, props.albedo.z));
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
                        Utils::log_info(std::format("      Loading texture: {}", texPath));
                        auto texture = textureManager->loadTexture(texPath, true, true);
                        if (texture)
                        {
                            material->setTexture(TextureType::Albedo, texture);
                            Utils::log_info("      Texture loaded");
                        }
                    }
                }

                renderComponent->setMaterial(material);
                Utils::log_info("Material set successfully");
            }
            else
            {
                Utils::log_warning("Failed to create material");
            }
        }

        renderComponent->setVisible(json.value("visible", true));

        return {};
    }

    nlohmann::json SceneSerializer::serializeMaterial(const Material* material)
    {
        nlohmann::json json;

        json["name"] = "Material";

        auto props = material->getProperties();
        json["properties"]["albedo"] = { props.albedo.x, props.albedo.y, props.albedo.z };
        json["properties"]["metallic"] = props.metallic;
        json["properties"]["roughness"] = props.roughness;

        json["textures"] = nlohmann::json::object();

        return json;
    }

    json SceneSerializer::serializeLuaComponent(const Scripting::LuaScriptComponent* component)
    {
        json json;
        json["name"] = "Lua";
        json["scriptPath"] = component->getScriptPath();

        return json;
    }

    json SceneSerializer::serializeBoxCollider(
        const Physics::BoxCollider* collider)
    {
        json colliderJson;

        const auto& size = collider->getSize();
        const auto& center = collider->getCenter();

        colliderJson["size"] = { size.x, size.y, size.z };
        colliderJson["center"] = { center.x, center.y, center.z };
        colliderJson["isTrigger"] = collider->isTrigger();

        return colliderJson;
    }

    void SceneSerializer::deserializeBoxCollider(
        Physics::BoxCollider* collider,
        const json& json)
    {
        if (json.contains("size"))
        {
            auto& s = json["size"];
            collider->setSize({
                s[0].get<float>(),
                s[1].get<float>(),
                s[2].get<float>()
                });
        }

        if (json.contains("center"))
        {
            auto& c = json["center"];
            collider->setCenter({
                c[0].get<float>(),
                c[1].get<float>(),
                c[2].get<float>()
                });
        }

        collider->setTrigger(json.value("isTrigger", true));
    }
}