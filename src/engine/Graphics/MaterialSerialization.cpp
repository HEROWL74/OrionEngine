//src/Graphics/MaterialSerialization.cpp
#include "MaterialSerialization.hpp"
#include <format>

namespace Engine::Graphics
{
    //=========================================================================
    // MaterialSerializer実装
    //=========================================================================

    Utils::VoidResult MaterialSerializer::saveMaterial(
        std::shared_ptr<Material> material,
        const std::string& filePath)
    {
        CHECK_CONDITION(material != nullptr, Utils::ErrorType::Unknown, "Material is null");

        auto jsonResult = materialToJson(material);
        if (!jsonResult)
        {
            return std::unexpected(jsonResult.error());
        }

        try
        {
            std::filesystem::path path(filePath);
            std::filesystem::create_directories(path.parent_path());

            std::ofstream file(filePath);
            if (!file.is_open())
            {
                return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                    std::format("Cannot open file for writing: {}", filePath)));
            }

            file << jsonResult->dump(4);
            file.close();

            Utils::log_info(std::format("Material saved: {}", filePath));
            return {};
        }
        catch (const std::exception& e)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                std::format("Failed to save material: {}", e.what())));
        }
    }

    Utils::Result<std::shared_ptr<Material>> MaterialSerializer::loadMaterial(
        const std::string& filePath,
        MaterialManager* materialManager,
        TextureManager* textureManager)
    {
        CHECK_CONDITION(materialManager != nullptr, Utils::ErrorType::Unknown, "MaterialManager is null");
        CHECK_CONDITION(textureManager != nullptr, Utils::ErrorType::Unknown, "TextureManager is null");

        if (!std::filesystem::exists(filePath))
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                std::format("Material file not found: {}", filePath)));
        }

        try
        {
            std::ifstream file(filePath);
            if (!file.is_open())
            {
                return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                    std::format("Cannot open material file: {}", filePath)));
            }

            json j;
            file >> j;
            file.close();

            auto materialResult = jsonToMaterial(j, materialManager, textureManager);
            if (!materialResult)
            {
                return std::unexpected(materialResult.error());
            }

            Utils::log_info(std::format("Material loaded: {}", filePath));
            return materialResult;
        }
        catch (const std::exception& e)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                std::format("Failed to load material: {}", e.what())));
        }
    }

    Utils::Result<json> MaterialSerializer::materialToJson(std::shared_ptr<Material> material)
    {
        CHECK_CONDITION(material != nullptr, Utils::ErrorType::Unknown, "Material is null");

        try
        {
            json j;
            j["version"] = "1.0";
            j["type"] = "material";
            j["name"] = material->getName();
            j["properties"] = material->getProperties();

            json textures = json::object();
            for (int i = 0; i < static_cast<int>(TextureType::Count); ++i)
            {
                TextureType type = static_cast<TextureType>(i);
                auto texture = material->getTexture(type);
                if (texture)
                {
                    std::string typeName = textureTypeToString(type);
                    json textureInfo;
                    textureInfo["path"] = texture->getDesc().debugName; 
                    textureInfo["width"] = texture->getWidth();
                    textureInfo["height"] = texture->getHeight();
                    textureInfo["format"] = static_cast<int>(texture->getFormat());
                    textures[typeName] = textureInfo;
                }
            }
            j["textures"] = textures;

            j["metadata"] = {
                {"created", std::time(nullptr)},
                {"engine", "DX12GameEngine"},
                {"engineVersion", "1.0"}
            };

            return j;
        }
        catch (const std::exception& e)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("Failed to serialize material: {}", e.what())));
        }
    }

    Utils::Result<std::shared_ptr<Material>> MaterialSerializer::jsonToMaterial(
        const json& j,
        MaterialManager* materialManager,
        TextureManager* textureManager)
    {
        try
        {
            if (!validateMaterialJson(j))
            {
                return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Invalid material JSON"));
            }

            std::string name = j.at("name").get<std::string>();
            auto material = materialManager->createMaterial(name);
            if (!material)
            {
                return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                    std::format("Failed to create material: {}", name)));
            }

            MaterialProperties props = j.at("properties").get<MaterialProperties>();
            material->setProperties(props);


            if (j.contains("textures"))
            {
                const auto& texturesJson = j.at("textures");
                for (auto& [typeName, textureInfo] : texturesJson.items())
                {
                    TextureType type = stringToTextureType(typeName);
                    std::string texturePath = textureInfo.at("path").get<std::string>();

                    auto texture = textureManager->loadTexture(texturePath);
                    if (texture)
                    {
                        material->setTexture(type, texture);
                    }
                    else
                    {
                        Utils::log_warning(std::format("Failed to load texture: {}", texturePath));
                    }
                }
            }

            return material;
        }
        catch (const std::exception& e)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("Failed to deserialize material: {}", e.what())));
        }
    }

    bool MaterialSerializer::validateMaterialJson(const json& j)
    {
        if (!j.contains("version") || !j.contains("type") || !j.contains("name") || !j.contains("properties"))
        {
            return false;
        }

        if (j.at("type").get<std::string>() != "material")
        {
            return false;
        }

        return validateVersion(j);
    }

    bool MaterialSerializer::validateVersion(const json& j)
    {
        std::string version = j.at("version").get<std::string>();
        return version == "1.0"; 
    }

    //=========================================================================
    // MaterialPresetManager実装
    //=========================================================================

    Utils::VoidResult MaterialPresetManager::initialize(
        MaterialManager* materialManager,
        TextureManager* textureManager,
        const std::string& presetDirectory)
    {
        m_materialManager = materialManager;
        m_textureManager = textureManager;
        m_presetDirectory = presetDirectory;
        

        std::filesystem::create_directories(m_presetDirectory);


        auto scanResult = scanPresetDirectory();
        if (!scanResult)
        {
            return scanResult;
        }


        auto builtInResult = createBuiltInPresets();
        if (!builtInResult)
        {
            return builtInResult;
        }

        Utils::log_info("MaterialPresetManager initialized successfully");
        return {};
    }

    Utils::VoidResult MaterialPresetManager::savePreset(
        std::shared_ptr<Material> material,
        const std::string& presetName,
        const std::string& description)
    {
        CHECK_CONDITION(material != nullptr, Utils::ErrorType::Unknown, "Material is null");

        std::string filePath = getPresetFilePath(presetName);


        auto presetMaterial = m_materialManager->createMaterial(presetName);
        if (!presetMaterial)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("Failed to create preset material: {}", presetName)));
        }

        presetMaterial->setProperties(material->getProperties());


        for (int i = 0; i < static_cast<int>(TextureType::Count); ++i)
        {
            TextureType type = static_cast<TextureType>(i);
            auto texture = material->getTexture(type);
            if (texture)
            {
                presetMaterial->setTexture(type, texture);
            }
        }

        auto saveResult = MaterialSerializer::saveMaterial(presetMaterial, filePath);
        if (!saveResult)
        {
            return saveResult;
        }

        m_presetDescriptions[presetName] = description;
        Utils::log_info(std::format("Material preset saved: {}", presetName));
        return {};
    }

    Utils::Result<std::shared_ptr<Material>> MaterialPresetManager::loadPreset(const std::string& presetName)
    {
        std::string filePath = getPresetFilePath(presetName);
        return MaterialSerializer::loadMaterial(filePath, m_materialManager, m_textureManager);
    }

    Utils::VoidResult MaterialPresetManager::createBuiltInPresets()
    {
        {
            auto metal = m_materialManager->createMaterial("Metal");
            MaterialProperties metalProps;
            metalProps.albedo = Math::Vector3(0.7f, 0.7f, 0.7f);
            metalProps.metallic = 1.0f;
            metalProps.roughness = 0.1f;
            metal->setProperties(metalProps);
            savePreset(metal, "Metal", "Shiny metallic surface");
        }


        {
            auto plastic = m_materialManager->createMaterial("Plastic");
            MaterialProperties plasticProps;
            plasticProps.albedo = Math::Vector3(0.8f, 0.2f, 0.2f);
            plasticProps.metallic = 0.0f;
            plasticProps.roughness = 0.4f;
            plastic->setProperties(plasticProps);
            savePreset(plastic, "Plastic", "Colored plastic material");
        }


        {
            auto glass = m_materialManager->createMaterial("Glass");
            MaterialProperties glassProps;
            glassProps.albedo = Math::Vector3(0.9f, 0.9f, 0.9f);
            glassProps.metallic = 0.0f;
            glassProps.roughness = 0.0f;
            glassProps.alpha = 0.1f;
            glass->setProperties(glassProps);
            savePreset(glass, "Glass", "Transparent glass material");
        }

        return {};
    }

    std::vector<std::string> MaterialPresetManager::getPresetNames() const
    {
        std::vector<std::string> names;
        for (const auto& entry : std::filesystem::directory_iterator(m_presetDirectory))
        {
            if (entry.path().extension() == ".json")
            {
                names.push_back(entry.path().stem().string());
            }
        }
        return names;
    }

    std::string MaterialPresetManager::getPresetDescription(const std::string& presetName) const
    {
        auto it = m_presetDescriptions.find(presetName);
        return (it != m_presetDescriptions.end()) ? it->second : "";
    }

    std::string MaterialPresetManager::getPresetFilePath(const std::string& presetName) const
    {
        return m_presetDirectory + presetName + ".json";
    }

    Utils::VoidResult MaterialPresetManager::scanPresetDirectory()
    {
        try
        {
            if (!std::filesystem::exists(m_presetDirectory))
            {
                return {};
            }

            for (const auto& entry : std::filesystem::directory_iterator(m_presetDirectory))
            {
                if (entry.path().extension() == ".json")
                {
                    std::string presetName = entry.path().stem().string();
                    m_presetDescriptions[presetName] = "";
                }
            }

            return {};
        }
        catch (const std::exception& e)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                std::format("Failed to scan preset directory: {}", e.what())));
        }
    }

    Utils::VoidResult MaterialPresetManager::deletePreset(const std::string& presetName)
    {
        try
        {
            std::string filePath = getPresetFilePath(presetName);
            if (std::filesystem::exists(filePath))
            {
                std::filesystem::remove(filePath);
                m_presetDescriptions.erase(presetName);
                Utils::log_info(std::format("Material preset deleted: {}", presetName));
            }
            return {};
        }
        catch (const std::exception& e)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                std::format("Failed to delete preset: {}", e.what())));
        }
    }

    //=========================================================================
    // MaterialImporter実装
    //=========================================================================

    Utils::Result<std::shared_ptr<Material>> MaterialImporter::importMaterial(
        const std::string& filePath,
        ImportFormat format,
        MaterialManager* materialManager,
        TextureManager* textureManager)
    {
        switch (format)
        {
        case ImportFormat::JSON:
            return MaterialSerializer::loadMaterial(filePath, materialManager, textureManager);

        case ImportFormat::MTL:
            return importFromMTL(filePath, materialManager, textureManager);

        case ImportFormat::GLTF:
            return importFromGLTF(filePath, materialManager, textureManager);

        default:
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Unsupported import format"));
        }
    }

    MaterialImporter::ImportFormat MaterialImporter::detectFormat(const std::string& filePath)
    {
        std::string extension = std::filesystem::path(filePath).extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        if (extension == ".json") return ImportFormat::JSON;
        if (extension == ".mtl") return ImportFormat::MTL;
        if (extension == ".gltf" || extension == ".glb") return ImportFormat::GLTF;

        return ImportFormat::JSON;
    }

    bool MaterialImporter::isFormatSupported(ImportFormat format)
    {
        switch (format)
        {
        case ImportFormat::JSON:
        case ImportFormat::MTL:
            return true;
        case ImportFormat::GLTF:
        case ImportFormat::FBX:
            return false; 
        default:
            return false;
        }
    }

    Utils::Result<std::shared_ptr<Material>> MaterialImporter::importFromMTL(
        const std::string& filePath,
        MaterialManager* materialManager,
        TextureManager* textureManager)
    {
        try
        {
            std::ifstream file(filePath);
            if (!file.is_open())
            {
                return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                    std::format("Cannot open MTL file: {}", filePath)));
            }

            std::string materialName = std::filesystem::path(filePath).stem().string();
            auto material = materialManager->createMaterial(materialName);
            if (!material)
            {
                return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                    std::format("Failed to create material: {}", materialName)));
            }

            MaterialProperties props;
            std::string line;
            std::string basePath = std::filesystem::path(filePath).parent_path().string() + "/";

            while (std::getline(file, line))
            {
                std::istringstream iss(line);
                std::string command;
                iss >> command;

                if (command == "Ka") 
                {
                    float r, g, b;
                    iss >> r >> g >> b;
                }
                else if (command == "Kd")
                {
                    float r, g, b;
                    iss >> r >> g >> b;
                    props.albedo = Math::Vector3(r, g, b);
                }
                else if (command == "Ks")
                {
                    float r, g, b;
                    iss >> r >> g >> b;
                    float luminance = 0.299f * r + 0.587f * g + 0.114f * b;
                    props.metallic = luminance;
                }
                else if (command == "Ns") 
                {
                    float ns;
                    iss >> ns;
                    props.roughness = std::sqrt(2.0f / (ns + 2.0f));
                }
                else if (command == "d" || command == "Tr")
                {
                    float alpha;
                    iss >> alpha;
                    props.alpha = (command == "Tr") ? (1.0f - alpha) : alpha;
                }
                else if (command == "map_Kd") 
                {
                    std::string texturePath;
                    iss >> texturePath;
                    auto texture = textureManager->loadTexture(basePath + texturePath);
                    if (texture)
                    {
                        material->setTexture(TextureType::Albedo, texture);
                    }
                }
                else if (command == "map_bump" || command == "bump")
                {
                    std::string texturePath;
                    iss >> texturePath;
                    auto texture = textureManager->loadTexture(basePath + texturePath);
                    if (texture)
                    {
                        material->setTexture(TextureType::Normal, texture);
                    }
                }
            }

            material->setProperties(props);
            Utils::log_info(std::format("MTL material imported: {}", materialName));
            return material;
        }
        catch (const std::exception& e)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                std::format("Failed to import MTL: {}", e.what())));
        }
    }

    Utils::Result<std::shared_ptr<Material>> MaterialImporter::importFromGLTF(
        const std::string& filePath,
        MaterialManager* materialManager,
        TextureManager* textureManager)
    {
        return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "GLTF import not implemented yet"));
    }

    //=========================================================================
    // MaterialExporter実装
    //=========================================================================

    Utils::VoidResult MaterialExporter::exportMaterial(
        std::shared_ptr<Material> material,
        const std::string& filePath,
        ExportFormat format)
    {
        switch (format)
        {
        case ExportFormat::JSON:
            return MaterialSerializer::saveMaterial(material, filePath);

        case ExportFormat::MTL:
            return exportToMTL(material, filePath);

        default:
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Unsupported export format"));
        }
    }

    bool MaterialExporter::isFormatSupported(ExportFormat format)
    {
        switch (format)
        {
        case ExportFormat::JSON:
        case ExportFormat::MTL:
            return true;
        case ExportFormat::GLTF:
            return false;
        default:
            return false;
        }
    }

    Utils::VoidResult MaterialExporter::exportToMTL(
        std::shared_ptr<Material> material,
        const std::string& filePath)
    {
        try
        {
            std::ofstream file(filePath);
            if (!file.is_open())
            {
                return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                    std::format("Cannot open file for writing: {}", filePath)));
            }

            const auto& props = material->getProperties();

            file << "# Material exported from DX12GameEngine\n";
            file << "newmtl " << material->getName() << "\n";


            file << "Ka 0.1 0.1 0.1\n";


            file << std::format("Kd {:.3f} {:.3f} {:.3f}\n", props.albedo.x, props.albedo.y, props.albedo.z);


            file << std::format("Ks {:.3f} {:.3f} {:.3f}\n", props.metallic, props.metallic, props.metallic);


            float ns = 2.0f / (props.roughness * props.roughness) - 2.0f;
            file << std::format("Ns {:.1f}\n", ns);

  
            file << std::format("d {:.3f}\n", props.alpha);

 
            auto albedoTexture = material->getTexture(TextureType::Albedo);
            if (albedoTexture)
            {
                file << "map_Kd " << albedoTexture->getDesc().debugName << "\n";
            }

            auto normalTexture = material->getTexture(TextureType::Normal);
            if (normalTexture)
            {
                file << "map_bump " << normalTexture->getDesc().debugName << "\n";
            }

            file.close();
            Utils::log_info(std::format("Material exported to MTL: {}", filePath));
            return {};
        }
        catch (const std::exception& e)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                std::format("Failed to export MTL: {}", e.what())));
        }
    }

    Utils::VoidResult MaterialExporter::exportToGLTF(
        std::shared_ptr<Material> material,
        const std::string& filePath)
    {
        return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "GLTF export not implemented yet"));
    }

    //=========================================================================
    // MaterialSerializer実装
    //=========================================================================

    Utils::VoidResult MaterialSerializer::saveMaterialLibrary(
        const std::unordered_map<std::string, std::shared_ptr<Material>>& materials,
        const std::string& filePath)
    {
        try
        {
            json library;
            library["version"] = "1.0";
            library["type"] = "material_library";
            library["materials"] = json::array();

            for (const auto& [name, material] : materials)
            {
                auto materialJson = materialToJson(material);
                if (materialJson)
                {
                    library["materials"].push_back(*materialJson);
                }
            }

            std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());

            std::ofstream file(filePath);
            if (!file.is_open())
            {
                return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                    std::format("Cannot open file for writing: {}", filePath)));
            }

            file << library.dump(4);
            file.close();

            Utils::log_info(std::format("Material library saved: {} materials to {}", materials.size(), filePath));
            return {};
        }
        catch (const std::exception& e)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                std::format("Failed to save material library: {}", e.what())));
        }
    }

    Utils::Result<std::unordered_map<std::string, std::shared_ptr<Material>>> MaterialSerializer::loadMaterialLibrary(
        const std::string& filePath,
        MaterialManager* materialManager,
        TextureManager* textureManager)
    {
        try
        {
            if (!std::filesystem::exists(filePath))
            {
                return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                    std::format("Material library file not found: {}", filePath)));
            }

            std::ifstream file(filePath);
            if (!file.is_open())
            {
                return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                    std::format("Cannot open material library file: {}", filePath)));
            }

            json library;
            file >> library;
            file.close();

            if (!library.contains("materials") || !library["materials"].is_array())
            {
                return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Invalid material library format"));
            }

            std::unordered_map<std::string, std::shared_ptr<Material>> materials;

            for (const auto& materialJson : library["materials"])
            {
                auto materialResult = jsonToMaterial(materialJson, materialManager, textureManager);
                if (materialResult)
                {
                    materials[(*materialResult)->getName()] = *materialResult;
                }
            }

            Utils::log_info(std::format("Material library loaded: {} materials from {}", materials.size(), filePath));
            return materials;
        }
        catch (const std::exception& e)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                std::format("Failed to load material library: {}", e.what())));
        }
    }
}
