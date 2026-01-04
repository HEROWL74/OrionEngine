//src/Graphics/MaterialSerialization.hpp
#pragma once

#include <string>
#include <memory>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "Material.hpp"
#include "Texture.hpp"
#include "../Utils/Common.hpp"

using json = nlohmann::json;

namespace Engine::Graphics
{
    //=========================================================================
    // マテリアルシリアライゼーションクラス
    //=========================================================================
    class MaterialSerializer
    {
    public:
        MaterialSerializer() = default;
        ~MaterialSerializer() = default;

        // マテリアルの保存
        [[nodiscard]] static Utils::VoidResult saveMaterial(
            std::shared_ptr<Material> material,
            const std::string& filePath
        );

        // マテリアルの読み込み
        [[nodiscard]] static Utils::Result<std::shared_ptr<Material>> loadMaterial(
            const std::string& filePath,
            MaterialManager* materialManager,
            TextureManager* textureManager
        );

        // マテリアルをJSONに変換
        [[nodiscard]] static Utils::Result<json> materialToJson(std::shared_ptr<Material> material);

        // JSONからマテリアルを作成
        [[nodiscard]] static Utils::Result<std::shared_ptr<Material>> jsonToMaterial(
            const json& j,
            MaterialManager* materialManager,
            TextureManager* textureManager
        );

        // バッチ保存/読み込み
        [[nodiscard]] static Utils::VoidResult saveMaterialLibrary(
            const std::unordered_map<std::string, std::shared_ptr<Material>>& materials,
            const std::string& filePath
        );

        [[nodiscard]] static Utils::Result<std::unordered_map<std::string, std::shared_ptr<Material>>>
            loadMaterialLibrary(
                const std::string& filePath,
                MaterialManager* materialManager,
                TextureManager* textureManager
            );

    private:
        /*
        // JSON変換ヘルパー
        static json vector3ToJson(const Math::Vector3& vec);
        static Math::Vector3 jsonToVector3(const json& j);
        static json vector2ToJson(const Math::Vector2& vec);
        static Math::Vector2 jsonToVector2(const json& j);

        // テクスチャパス処理
        static std::string getRelativeTexturePath(const std::string& materialPath, const std::string& texturePath);
        static std::string resolveTexturePath(const std::string& materialPath, const std::string& relativePath);
        */
        // バリデーション
        static bool validateMaterialJson(const json& j);
        static bool validateVersion(const json& j);
    };

    //=========================================================================
    // マテリアルプリセットマネージャー
    //=========================================================================
    class MaterialPresetManager
    {
    public:
        MaterialPresetManager() = default;
        ~MaterialPresetManager() = default;

        // 初期化
        [[nodiscard]] Utils::VoidResult initialize(
            MaterialManager* materialManager,
            TextureManager* textureManager,
            const std::string& presetDirectory = "assets/materials/presets/"
        );

        // プリセット管理
        [[nodiscard]] Utils::VoidResult savePreset(
            std::shared_ptr<Material> material,
            const std::string& presetName,
            const std::string& description = ""
        );

        [[nodiscard]] Utils::Result<std::shared_ptr<Material>> loadPreset(const std::string& presetName);

        // プリセット一覧
        std::vector<std::string> getPresetNames() const;
        std::string getPresetDescription(const std::string& presetName) const;

        // プリセット削除
        [[nodiscard]] Utils::VoidResult deletePreset(const std::string& presetName);

        // 内蔵プリセット作成
        [[nodiscard]] Utils::VoidResult createBuiltInPresets();

    private:
        MaterialManager* m_materialManager = nullptr;
        TextureManager* m_textureManager = nullptr;
        std::string m_presetDirectory;
        std::unordered_map<std::string, std::string> m_presetDescriptions;

        // ヘルパー関数
        std::string getPresetFilePath(const std::string& presetName) const;
        [[nodiscard]] Utils::VoidResult scanPresetDirectory();
    };

    //=========================================================================
    // マテリアルインポーター
    //=========================================================================
    class MaterialImporter
    {
    public:
        MaterialImporter() = default;
        ~MaterialImporter() = default;

        // サポートされている形式
        enum class ImportFormat
        {
            JSON,       // 独自JSON形式
            MTL,        // Wavefront MTL
            FBX,        // FBX
            GLTF        // glTF
        };

        // インポート
        [[nodiscard]] static Utils::Result<std::shared_ptr<Material>> importMaterial(
            const std::string& filePath,
            ImportFormat format,
            MaterialManager* materialManager,
            TextureManager* textureManager
        );

        // 形式の自動判定
        static ImportFormat detectFormat(const std::string& filePath);

        // サポート形式チェック
        static bool isFormatSupported(ImportFormat format);

    private:
        // 各形式のインポーター
        [[nodiscard]] static Utils::Result<std::shared_ptr<Material>> importFromMTL(
            const std::string& filePath,
            MaterialManager* materialManager,
            TextureManager* textureManager
        );

        [[nodiscard]] static Utils::Result<std::shared_ptr<Material>> importFromGLTF(
            const std::string& filePath,
            MaterialManager* materialManager,
            TextureManager* textureManager
        );
    };

    //=========================================================================
    // マテリアルエクスポーター
    //=========================================================================
    class MaterialExporter
    {
    public:
        MaterialExporter() = default;
        ~MaterialExporter() = default;

        // サポートされている形式
        enum class ExportFormat
        {
            JSON,       // 独自JSON形式
            MTL,        // Wavefront MTL
            GLTF        // glTF
        };

        // エクスポート
        [[nodiscard]] static Utils::VoidResult exportMaterial(
            std::shared_ptr<Material> material,
            const std::string& filePath,
            ExportFormat format
        );

        // サポート形式チェック
        static bool isFormatSupported(ExportFormat format);

    private:
        // 各形式のエクスポーター
        [[nodiscard]] static Utils::VoidResult exportToMTL(
            std::shared_ptr<Material> material,
            const std::string& filePath
        );

        [[nodiscard]] static Utils::VoidResult exportToGLTF(
            std::shared_ptr<Material> material,
            const std::string& filePath
        );
    };
}

// JSON変換特化
namespace nlohmann
{
    template<>
    struct adl_serializer<Engine::Math::Vector3>
    {
        static void to_json(json& j, const Engine::Math::Vector3& vec)
        {
            j = json{ {"x", vec.x}, {"y", vec.y}, {"z", vec.z} };
        }

        static void from_json(const json& j, Engine::Math::Vector3& vec)
        {
            j.at("x").get_to(vec.x);
            j.at("y").get_to(vec.y);
            j.at("z").get_to(vec.z);
        }
    };

    template<>
    struct adl_serializer<Engine::Math::Vector2>
    {
        static void to_json(json& j, const Engine::Math::Vector2& vec)
        {
            j = json{ {"x", vec.x}, {"y", vec.y} };
        }

        static void from_json(const json& j, Engine::Math::Vector2& vec)
        {
            j.at("x").get_to(vec.x);
            j.at("y").get_to(vec.y);
        }
    };

    template<>
    struct adl_serializer<Engine::Graphics::MaterialProperties>
    {
        static void to_json(json& j, const Engine::Graphics::MaterialProperties& props)
        {
            j = json{
                {"albedo", props.albedo},
                {"metallic", props.metallic},
                {"roughness", props.roughness},
                {"ao", props.ao},
                {"emissive", props.emissive},
                {"emissiveStrength", props.emissiveStrength},
                {"normalStrength", props.normalStrength},
                {"heightScale", props.heightScale},
                {"alpha", props.alpha},
                {"useAlphaTest", props.useAlphaTest},
                {"alphaTestThreshold", props.alphaTestThreshold},
                {"uvScale", props.uvScale},
                {"uvOffset", props.uvOffset}
            };
        }

        static void from_json(const json& j, Engine::Graphics::MaterialProperties& props)
        {
            j.at("albedo").get_to(props.albedo);
            j.at("metallic").get_to(props.metallic);
            j.at("roughness").get_to(props.roughness);
            j.at("ao").get_to(props.ao);
            j.at("emissive").get_to(props.emissive);
            j.at("emissiveStrength").get_to(props.emissiveStrength);
            j.at("normalStrength").get_to(props.normalStrength);
            j.at("heightScale").get_to(props.heightScale);
            j.at("alpha").get_to(props.alpha);
            j.at("useAlphaTest").get_to(props.useAlphaTest);
            j.at("alphaTestThreshold").get_to(props.alphaTestThreshold);
            j.at("uvScale").get_to(props.uvScale);
            j.at("uvOffset").get_to(props.uvOffset);
        }
    };
}
