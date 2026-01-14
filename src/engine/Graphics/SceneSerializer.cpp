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

            // 全GameObjectをシリアライズ
            for (const auto& gameObject : scene.getGameObjects())
            {
                if (gameObject)
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

            file << sceneJson.dump(4); // 4スペースインデント
            file.close();

            Utils::log_info(std::format("Scene saved to: {}", filepath));
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
            // ファイルを開く
            std::ifstream file(filepath);
            if (!file.is_open())
            {
                return std::unexpected(Utils::make_error(
                    Utils::ErrorType::FileI0,
                    "Failed to open file for reading: " + filepath
                ));
            }

            // JSONをパース
            nlohmann::json sceneJson;
            file >> sceneJson;
            file.close();

            // バージョンチェック
            if (!sceneJson.contains("version"))
            {
                return std::unexpected(Utils::make_error(
                    Utils::ErrorType::Unknown,
                    "Invalid scene file: missing version"
                ));
            }

            // GameObjectsを復元
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

            Utils::log_info(std::format("Scene loaded from: {}", filepath));
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
        // TODO: LuaScriptComponent の情報もシリアライズ
        auto* lua = gameObject->getComponent<Engine::Scripting::LuaScriptComponent>();
        if (lua)
        {
            json["lua"] = serializeLuaComponent(lua);
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
        // GameObjectを作成
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

        // Active状態を設定
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

        // RenderableType
        json["renderableType"] = static_cast<int>(component->getRenderableType());
        json["visible"] = component->isVisible();

        // Material情報
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
        // RenderableTypeを取得
        auto renderType = static_cast<RenderableType>(
            json.value("renderableType", 0)
            );

        Utils::log_info(std::format("    RenderableType: {}", static_cast<int>(renderType)));

        // RenderComponentを追加
        auto* renderComponent = gameObject->addComponent<RenderComponent>(renderType);
        if (!renderComponent)
        {
            return std::unexpected(Utils::make_error(
                Utils::ErrorType::Unknown,
                "Failed to add RenderComponent"
            ));
        }

        // MaterialManagerを設定
        renderComponent->setMaterialManager(materialManager);

        // 初期化
        Utils::log_info("Initializing RenderComponent...");
        auto initResult = renderComponent->initialize(device, shaderManager);
        if (!initResult)
        {
            return initResult;
        }
        Utils::log_info("RenderComponent initialized");

        // Material情報を復元
        if (json.contains("material"))
        {
            const auto& matJson = json["material"];

            // ユニークなMaterial名を生成（オブジェクト名を含める）
            std::string baseName = matJson.value("name", "Material");
            std::string matName = gameObject->getName() + "_" + baseName;

            Utils::log_info(std::format("Creating material: {}", matName));

            // Materialを作成
            auto material = materialManager->createMaterial(matName);
            if (material)
            {
                // プロパティを復元
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

                // テクスチャを復元
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

        // Visible状態を設定
        renderComponent->setVisible(json.value("visible", true));
 
        return {};
    }

    nlohmann::json SceneSerializer::serializeMaterial(const Material* material)
    {
        nlohmann::json json;

        json["name"] = "Material"; 

        // プロパティ
        auto props = material->getProperties();
        json["properties"]["albedo"] = { props.albedo.x, props.albedo.y, props.albedo.z };
        json["properties"]["metallic"] = props.metallic;
        json["properties"]["roughness"] = props.roughness;

        // テクスチャパス（今後実装）
        json["textures"] = nlohmann::json::object();

        return json;
    }

    json SceneSerializer::serializeLuaComponent(const Scripting::LuaScriptComponent* component)
    {
        json json;
        json["name"] = "Lua";
        // プロパティ
        json["scriptPath"] = component->getScriptPath();

        return json;
    }
}