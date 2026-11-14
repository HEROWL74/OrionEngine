// src/Graphics/Material.cpp
#include "Material.hpp"
#include <format>

namespace Renderer {
//=========================================================================
// Material 実装
//=========================================================================

Material::Material(const std::string &name) : m_name(name) {}

Utils::VoidResult Material::initialize(Device *device) {
  if (m_initialized) {
    return {};
  }

  CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown,
                  "Device is null");
  CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown,
                  "Device is not valid");

  m_device = device;

  auto cbResult = createConstantBuffer();
  if (!cbResult) {
    Utils::log_warning(std::format("createConstantBuffer failed for '{}': {}",
                                   m_name, cbResult.error().message));
    return cbResult;
  }

  m_initialized = true;
  Utils::log_info(std::format("Material '{}' initialized", m_name));
  return {};
}

void Material::setTexture(TextureType type, std::shared_ptr<Texture> texture) {
  if (texture) {
    m_textures[type] = texture;
  } else {
    removeTexture(type);
  }
  m_isDirty = true;
}

std::shared_ptr<Texture> Material::getTexture(TextureType type) const {
  auto it = m_textures.find(type);
  return (it != m_textures.end()) ? it->second : nullptr;
}

bool Material::hasTexture(TextureType type) const {
  return m_textures.find(type) != m_textures.end();
}

void Material::removeTexture(TextureType type) {
  auto it = m_textures.find(type);
  if (it != m_textures.end()) {
    m_textures.erase(it);
    if (type == TextureType::Albedo) {
      m_properties.useAlbedoTex = 0;
    }
    m_isDirty = true;
    updateConstantBuffer();
  }
}

Utils::VoidResult Material::updateConstantBuffer() {
  if (!m_initialized) {
    return std::unexpected(Utils::make_error(
        Utils::ErrorType::Unknown,
        std::format("Material '{}' not initialized", m_name)));
  }
  if (!m_constantBufferData) {
    return std::unexpected(Utils::make_error(
        Utils::ErrorType::Unknown,
        std::format("Material '{}' constant buffer not mapped", m_name)));
  }

  MaterialConstantBuffer cbData{};

  cbData.albedo = Math::Vector4(m_properties.albedo.x, m_properties.albedo.y,
                                m_properties.albedo.z, m_properties.metallic);

  cbData.roughnessAoEmissiveStrength =
      Math::Vector4(m_properties.roughness, m_properties.ao,
                    m_properties.emissiveStrength, 0.0f);

  cbData.emissive =
      Math::Vector4(m_properties.emissive.x, m_properties.emissive.y,
                    m_properties.emissive.z, m_properties.normalStrength);

  cbData.alphaParams =
      Math::Vector4(m_properties.alpha, m_properties.useAlphaTest ? 1.0f : 0.0f,
                    m_properties.alphaTestThreshold, m_properties.heightScale);

  cbData.uvTransform =
      Math::Vector4(m_properties.uvScale.x, m_properties.uvScale.y,
                    m_properties.uvOffset.x, m_properties.uvOffset.y);

  cbData.hasAlbedoTexture = (m_properties.useAlbedoTex != 0) ? 1 : 0;

  memcpy(m_constantBufferData, &cbData, sizeof(MaterialConstantBuffer));

  m_isDirty = false;
  return {};
}

void Material::bind(ID3D12GraphicsCommandList *commandList,
                    UINT rootParameterIndex) const {
  if (!m_initialized || !m_constantBuffer) {
    Utils::log_warning(
        std::format("Attempting to bind uninitialized material '{}'", m_name));
    return;
  }

  commandList->SetGraphicsRootConstantBufferView(
      rootParameterIndex, m_constantBuffer->GetGPUVirtualAddress());
}

Utils::VoidResult Material::createConstantBuffer() {
  if (!m_device || !m_device->isValid() || !m_device->getDevice()) {
    return std::unexpected(Utils::make_error(
        Utils::ErrorType::Unknown,
        std::format("Device invalid when creating constant buffer for '{}'",
                    m_name)));
  }

  const UINT cbSize = (sizeof(MaterialConstantBuffer) + 255) & ~255;

  D3D12_HEAP_PROPERTIES heapProps{};
  heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
  heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heapProps.CreationNodeMask = 1;
  heapProps.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Width = cbSize;
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

  HRESULT hr = m_device->getDevice()->CreateCommittedResource(
      &heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
      IID_PPV_ARGS(&m_constantBuffer));

  if (FAILED(hr)) {
    return std::unexpected(Utils::make_error(
        Utils::ErrorType::ResourceCreation,
        std::format("Failed to create constant buffer for '{}': HRESULT=0x{:X}",
                    m_name, static_cast<unsigned>(hr)),
        hr));
  }

  D3D12_RANGE readRange{0, 0};
  hr = m_constantBuffer->Map(0, &readRange, &m_constantBufferData);
  if (FAILED(hr)) {
    m_constantBuffer.Reset();
    return std::unexpected(Utils::make_error(
        Utils::ErrorType::ResourceCreation,
        std::format("Failed to map constant buffer for '{}': HRESULT=0x{:X}",
                    m_name, static_cast<unsigned>(hr)),
        hr));
  }

  Utils::log_info(std::format("Constant buffer created ({} bytes) for '{}'",
                              cbSize, m_name));
  return {};
}

void Material::setProperties(const MaterialProperties &properties) {
  m_properties = properties;
  m_isDirty = true;

  if (m_initialized && m_device && m_device->isValid()) {
    auto result = updateConstantBuffer();
    if (!result) {
      Utils::log_warning(
          std::format("Failed to update constant buffer for '{}': {}", m_name,
                      result.error().message));
    }
  }
}

Utils::VoidResult Material::saveToFile(const std::string &) const {
  return std::unexpected(
      Utils::make_error(Utils::ErrorType::Unknown, "Not implemented yet"));
}

Utils::VoidResult Material::loadFromFile(const std::string &) {
  return std::unexpected(
      Utils::make_error(Utils::ErrorType::Unknown, "Not implemented yet"));
}

//=========================================================================
// MaterialManager 実装
//=========================================================================

Utils::VoidResult MaterialManager::initialize(Device *device) {
  if (m_initialized) {
    Utils::log_warning("MaterialManager already initialized");
    return {};
  }

  CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown,
                  "Device is null");
  CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown,
                  "Device is not valid");

  m_device = device;

  auto defaultResult = createDefaultMaterial();
  if (!defaultResult) {
    m_device = nullptr;
    Utils::log_warning(std::format("Failed to create default material: {}",
                                   defaultResult.error().message));
    return defaultResult;
  }

  m_initialized = true;
  Utils::log_info("MaterialManager initialized successfully");
  return {};
}

std::shared_ptr<Material>
MaterialManager::createMaterial(const std::string &name) {
  if (!m_initialized) {
    Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                                       "MaterialManager not initialized"));
    return nullptr;
  }

  // 重複を避けるためにユニークな名前を生成
  std::string uniqueName = name;
  for (int counter = 1; hasMaterial(uniqueName); ++counter) {
    uniqueName = name + "_" + std::to_string(counter);
  }

  if (uniqueName != name) {
    Utils::log_info(
        std::format("Material '{}' already exists, creating '{}' instead", name,
                    uniqueName));
  }

  auto material = std::make_shared<Material>(uniqueName);
  auto initResult = material->initialize(m_device);
  if (!initResult) {
    Utils::log_error(initResult.error());
    return nullptr;
  }

  m_materials[uniqueName] = material;
  Utils::log_info(std::format("Material '{}' created", uniqueName));
  return material;
}

std::shared_ptr<Material>
MaterialManager::getMaterial(const std::string &name) const {
  auto it = m_materials.find(name);
  return (it != m_materials.end()) ? it->second : nullptr;
}

bool MaterialManager::hasMaterial(const std::string &name) const {
  return m_materials.find(name) != m_materials.end();
}

void MaterialManager::removeMaterial(const std::string &name) {
  auto it = m_materials.find(name);
  if (it != m_materials.end()) {
    m_materials.erase(it);
    Utils::log_info(std::format("Material '{}' removed", name));
  }
}

void MaterialManager::updateAllMaterials() {
  for (auto &[name, material] : m_materials) {
    if (material && material->m_isDirty) {
      material->updateConstantBuffer();
    }
  }
}

Utils::VoidResult MaterialManager::createDefaultMaterial() {
  m_defaultMaterial = std::make_shared<Material>("DefaultMaterial");

  auto initResult = m_defaultMaterial->initialize(m_device);
  if (!initResult) {
    Utils::log_warning(std::format("Failed to initialize default material: {}",
                                   initResult.error().message));
    m_defaultMaterial.reset();
    return initResult;
  }

  MaterialProperties defaultProps;
  defaultProps.albedo = Math::Vector3(0.8f, 0.8f, 0.8f);
  defaultProps.metallic = 0.0f;
  defaultProps.roughness = 0.5f;
  defaultProps.ao = 1.0f;
  m_defaultMaterial->setProperties(defaultProps);

  m_materials["DefaultMaterial"] = m_defaultMaterial;

  Utils::log_info("Default material created successfully");
  return {};
}

//=========================================================================
// ユーティリティ関数
//=========================================================================

std::string textureTypeToString(TextureType type) {
  switch (type) {
  case TextureType::Albedo:
    return "Albedo";
  case TextureType::Normal:
    return "Normal";
  case TextureType::Metallic:
    return "Metallic";
  case TextureType::Roughness:
    return "Roughness";
  case TextureType::AO:
    return "AO";
  case TextureType::Emissive:
    return "Emissive";
  case TextureType::Height:
    return "Height";
  default:
    return "Unknown";
  }
}

TextureType stringToTextureType(const std::string &str) {
  if (str == "Albedo")
    return TextureType::Albedo;
  if (str == "Normal")
    return TextureType::Normal;
  if (str == "Metallic")
    return TextureType::Metallic;
  if (str == "Roughness")
    return TextureType::Roughness;
  if (str == "AO")
    return TextureType::AO;
  if (str == "Emissive")
    return TextureType::Emissive;
  if (str == "Height")
    return TextureType::Height;

  Utils::log_warning(std::format("Unknown texture type: {}", str));
  return TextureType::Albedo;
}
} // namespace Renderer