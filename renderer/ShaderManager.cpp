// src/Graphics/ShaderManager.cpp
#include "ShaderManager.hpp"
#include "../engine/Core/ProjectSettings.hpp"
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>

namespace Renderer {
//=========================================================================
// Shader Manaeger
//=========================================================================

Utils::Result<std::shared_ptr<Shader>>
Shader::compileFromFile(const ShaderCompileDesc &desc) {
  // codeResult init
  auto codeResult = readShaderFile(desc.filePath);
  if (!codeResult) {
    return std::unexpected(codeResult.error());
  }

  // シェーダー関連処理
  std::string baseDir =
      std::filesystem::path(desc.filePath).parent_path().string();
  std::string processedCode = processIncludes(*codeResult, baseDir);

  auto shader = std::make_shared<Shader>();
  auto initResult =
      shader->initialize(processedCode, desc.entryPoint, desc.type, desc.macros,
                         desc.enableDebug, desc.filePath);
  if (!initResult) {
    return std::unexpected(initResult.error());
  }

  return shader;
}

Utils::Result<std::shared_ptr<Shader>>
Shader::loadFromCSO(const std::string &csoFilePath, ShaderType type) {
  std::ifstream file(csoFilePath, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    return std::unexpected(Utils::make_error(
        Utils::ErrorType::FileI0,
        std::format("Cannot open CSO file: {}", csoFilePath)));
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  // ID3DBlob にデータを格納
  ComPtr<ID3DBlob> blob;
  HRESULT hr = D3DCreateBlob(static_cast<SIZE_T>(size), &blob);
  if (FAILED(hr)) {
    return std::unexpected(Utils::make_error(
        Utils::ErrorType::ResourceCreation,
        std::format("Failed to create blob for CSO: {}", csoFilePath), hr));
  }

  if (!file.read(static_cast<char *>(blob->GetBufferPointer()), size)) {
    return std::unexpected(Utils::make_error(
        Utils::ErrorType::FileI0,
        std::format("Failed to read CSO file: {}", csoFilePath)));
  }

  auto shader = std::make_shared<Shader>();
  shader->m_type = type;
  shader->m_entryPoint = "main";
  shader->m_filePath = csoFilePath;
  shader->m_bytecode = blob;

  return shader;
}

Utils::Result<std::shared_ptr<Shader>> Shader::compileFromString(
    const std::string &shaderCode, const std::string &entryPoint,
    ShaderType type, const std::vector<ShaderMacro> &macros, bool enableDebug) {
  auto shader = std::make_shared<Shader>();
  auto initResult =
      shader->initialize(shaderCode, entryPoint, type, macros, enableDebug);
  if (!initResult) {
    return std::unexpected(initResult.error());
  }

  return shader;
}

Utils::VoidResult
Shader::initialize(const std::string &shaderCode, const std::string &entryPoint,
                   ShaderType type, const std::vector<ShaderMacro> &macros,
                   bool enableDebug, const std::string &filePath) {
  m_type = type;
  m_entryPoint = entryPoint;
  m_filePath = filePath;

  // 繧ｳ繝ｳ繝代う繝ｫ繝輔Λ繧ｰ
  UINT compileFlags = 0;
  if (enableDebug) {
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
  } else {
    compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
  }

  // 繝槭け繝ｭ螟画鋤
  auto d3dMacros = convertMacros(macros);

  // 繧ｷ繧ｧ繝ｼ繝繝ｼ繧ｿ繝ｼ繧ｲ繝・ヨ
  std::string target = shaderTypeToTarget(type);

  ComPtr<ID3DBlob> errorBlob;
  HRESULT hr =
      D3DCompile(shaderCode.c_str(), shaderCode.size(),
                 filePath.empty() ? nullptr : filePath.c_str(),
                 d3dMacros.empty() ? nullptr : d3dMacros.data(),
                 D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(),
                 target.c_str(), compileFlags, 0, &m_bytecode, &errorBlob);

  if (FAILED(hr)) {
    std::string errorMsg = "Shader compilation failed";
    if (errorBlob) {
      errorMsg += std::format(
          ": {}", static_cast<const char *>(errorBlob->GetBufferPointer()));
    }

    return std::unexpected(
        Utils::make_error(Utils::ErrorType::ShaderCompilation, errorMsg, hr));
  }

  return {};
}

std::string Shader::shaderTypeToTarget(ShaderType type) {
  switch (type) {
  case ShaderType::Vertex:
    return "vs_5_1";
  case ShaderType::Pixel:
    return "ps_5_1";
  case ShaderType::Geometry:
    return "gs_5_1";
  case ShaderType::Hull:
    return "hs_5_1";
  case ShaderType::Domain:
    return "ds_5_1";
  case ShaderType::Compute:
    return "cs_5_1";
  default:
    return "vs_5_1";
  }
}

std::vector<D3D_SHADER_MACRO>
Shader::convertMacros(const std::vector<ShaderMacro> &macros) {
  std::vector<D3D_SHADER_MACRO> d3dMacros;
  d3dMacros.reserve(macros.size() + 1);

  for (const auto &macro : macros) {
    D3D_SHADER_MACRO d3dMacro;
    d3dMacro.Name = macro.name.c_str();
    d3dMacro.Definition = macro.definition.c_str();
    d3dMacros.push_back(d3dMacro);
  }

  // 邨らｫｯ
  D3D_SHADER_MACRO endMacro = {nullptr, nullptr};
  d3dMacros.push_back(endMacro);

  return d3dMacros;
}

// ===========================================
// ShaderManager
// ===========================================
Utils::VoidResult ShaderManager::initialize(Device *device) {
  CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown,
                  "Device is null");
  CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown,
                  "Device is not valid");

  m_device = device;

  auto defaultShadersResult = createDefaultShaders();
  if (!defaultShadersResult) {
    return defaultShadersResult;
  }

  auto defaultPipelinesResult = createDefaultPipelines();
  if (!defaultPipelinesResult) {
    return defaultPipelinesResult;
  }

  m_initialized = true;
  Utils::log_info("ShaderManager initialized successfully");
  return {};
}

std::shared_ptr<Shader>
ShaderManager::loadShader(const ShaderCompileDesc &desc) {
  if (!m_initialized) {
    Utils::log_warning("ShaderManager not initialized");
    return nullptr;
  }

  const std::string key = generateShaderKey(desc);

  if (hasShader(key)) {
    return getShader(key);
  }

  const std::string ext =
      std::filesystem::path(desc.filePath).extension().string();
  Utils::Result<std::shared_ptr<Shader>> shaderResult;

  if (ext == ".cso") {
    Utils::log_info(
        std::format("Loading precompiled shader: {}", desc.filePath));
    shaderResult = Shader::loadFromCSO(desc.filePath, desc.type);
  } else {
    Utils::log_info(std::format("Compiling shader: {}", desc.filePath));
    shaderResult = Shader::compileFromFile(desc);
  }

  if (!shaderResult) {
    Utils::log_warning(std::format("Failed to load shader '{}': {}",
                                   desc.filePath,
                                   shaderResult.error().message));
    return nullptr;
  }

  m_shaders[key] = *shaderResult;
  Utils::log_info(std::format("Shader loaded and cached: {}", desc.filePath));
  return *shaderResult;
}

std::shared_ptr<Shader>
ShaderManager::getShader(const std::string &name) const {
  auto it = m_shaders.find(name);
  return (it != m_shaders.end()) ? it->second : nullptr;
}

bool ShaderManager::hasShader(const std::string &name) const {
  return m_shaders.find(name) != m_shaders.end();
}

void ShaderManager::removeShader(const std::string &name) {
  auto it = m_shaders.find(name);
  if (it != m_shaders.end()) {
    m_shaders.erase(it);
  }
}

Utils::VoidResult ShaderManager::createDefaultShaders() {
  auto &settings = Engine::Core::ProjectSettings::get();
  ShaderCompileDesc vsDesc;
  vsDesc.filePath = settings.getEngineAssetPath("shaders/PBR_VS.cso").string();
  vsDesc.entryPoint = "main";
  vsDesc.type = ShaderType::Vertex;

  auto vsResult = Shader::loadFromCSO(vsDesc.filePath, vsDesc.type);
  if (!vsResult) {
    return std::unexpected(vsResult.error());
  }

  ShaderCompileDesc psDesc;
  psDesc.filePath = settings.getEngineAssetPath("shaders/PBR_PS.cso").string();
  psDesc.entryPoint = "main";
  psDesc.type = ShaderType::Pixel;

  auto psResult = Shader::loadFromCSO(psDesc.filePath, psDesc.type);
  if (!psResult) {
    return std::unexpected(psResult.error());
  }

  m_shaders["DefaultPBR_VS"] = *vsResult;
  m_shaders["DefaultPBR_PS"] = *psResult;

  Utils::log_info("Default PBR shaders loaded successfully");
  return {};
}

std::shared_ptr<Shader>
ShaderManager::compileFromString(const std::string &shaderCode,
                                 const std::string &entryPoint, ShaderType type,
                                 const std::string &shaderName) {
  if (!m_initialized) {
    Utils::log_warning("ShaderManager not initialized");
    return nullptr;
  }

  auto shaderResult = Shader::compileFromString(shaderCode, entryPoint, type);
  if (!shaderResult) {
    Utils::log_warning(std::format("Failed to compile inline shader '{}': {}",
                                   shaderName, shaderResult.error().message));
    return nullptr;
  }

  return *shaderResult;
}

std::string
ShaderManager::generateShaderKey(const ShaderCompileDesc &desc) const {
  std::string key = desc.filePath + "_" + desc.entryPoint + "_" +
                    std::to_string(static_cast<int>(desc.type));

  for (const auto &macro : desc.macros) {
    key += "_" + macro.name + "=" + macro.definition;
  }

  if (desc.enableDebug) {
    key += "_DEBUG";
  }

  return key;
}

//=========================================================================
// StandardInputLayouts
//=========================================================================

namespace StandardInputLayouts {
const std::vector<D3D12_INPUT_ELEMENT_DESC> Position = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

const std::vector<D3D12_INPUT_ELEMENT_DESC> PositionUV = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12,
     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

const std::vector<D3D12_INPUT_ELEMENT_DESC> PositionNormalUV = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,
     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

const std::vector<D3D12_INPUT_ELEMENT_DESC> PBRVertex = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,
     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32,
     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};
} // namespace StandardInputLayouts

Utils::Result<std::string> readShaderFile(const std::string &filePath) {
  std::ifstream file(filePath, std::ios::binary);
  if (!file.is_open()) {
    return std::unexpected(Utils::make_error(
        Utils::ErrorType::FileI0,
        std::format("Cannot open shader file: {}", filePath)));
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();

  return content;
}

std::string processIncludes(const std::string &shaderCode,
                            const std::string &baseDir) {
  std::string result = shaderCode;
  std::string includePattern = "#include \"";

  size_t pos = 0;
  while ((pos = result.find(includePattern, pos)) != std::string::npos) {
    size_t startQuote = pos + includePattern.length();
    size_t endQuote = result.find("\"", startQuote);

    if (endQuote != std::string::npos) {
      std::string includeFile =
          result.substr(startQuote, endQuote - startQuote);
      std::string fullPath =
          baseDir.empty() ? includeFile : baseDir + "/" + includeFile;

      auto includeResult = readShaderFile(fullPath);
      if (includeResult) {
        size_t lineEnd = result.find("\n", pos);
        if (lineEnd == std::string::npos)
          lineEnd = result.length();

        result.replace(pos, lineEnd - pos, *includeResult);
        pos += includeResult->length();
      } else {
        Utils::log_warning(std::format("Failed to include file: {}", fullPath));
        pos = endQuote + 1;
      }
    } else {
      pos += includePattern.length();
    }
  }

  return result;
}
} // namespace Renderer
