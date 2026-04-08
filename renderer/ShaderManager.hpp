// src/Graphics/ShaderManager.hpp
#pragma once

#include "../engine/Utils/Common.hpp"
#include "Device.hpp"
#include <d3d12.h>
#include <d3dcompiler.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

namespace Renderer {
using namespace Engine;
//=========================================================================
// シェーダータイプ列挙型
//=========================================================================
enum class ShaderType { Vertex, Pixel, Geometry, Hull, Domain, Compute };

//=========================================================================
// シェーダーマクロ構造体
//=========================================================================
struct ShaderMacro {
  std::string name;
  std::string definition;

  ShaderMacro(const std::string &n, const std::string &d)
      : name(n), definition(d) {}
};

//=========================================================================
// シェーダーコンパイル設定
//=========================================================================
struct ShaderCompileDesc {
  std::string filePath;
  std::string entryPoint;
  ShaderType type;
  std::vector<ShaderMacro> macros;
  bool enableDebug = false;
  bool enableOptimization = true;
};

//=========================================================================
// シェーダークラス
//=========================================================================
class Shader {
public:
  Shader() = default;
  ~Shader() = default;

  // コピー・ムーブ
  Shader(const Shader &) = delete;
  Shader &operator=(const Shader &) = delete;
  Shader(Shader &&) = default;
  Shader &operator=(Shader &&) = default;

  // ファイルからのコンパイル
  [[nodiscard]] static Utils::Result<std::shared_ptr<Shader>>
  compileFromFile(const ShaderCompileDesc &desc);

  // プリコンパイル済み .csoを読み込む
  [[nodiscard]] static Utils::Result<std::shared_ptr<Shader>>
  loadFromCSO(const std::string &csoFilePath, ShaderType type);

  // 文字列からのコンパイル
  [[nodiscard]] static Utils::Result<std::shared_ptr<Shader>>
  compileFromString(const std::string &shaderCode,
                    const std::string &entryPoint, ShaderType type,
                    const std::vector<ShaderMacro> &macros = {},
                    bool enableDebug = false);

  // 基本情報の取得
  ShaderType getType() const { return m_type; }
  const std::string &getEntryPoint() const { return m_entryPoint; }
  const std::string &getFilePath() const { return m_filePath; }

  // バイトコードの取得
  const void *getBytecode() const { return m_bytecode->GetBufferPointer(); }
  size_t getBytecodeSize() const { return m_bytecode->GetBufferSize(); }
  ID3DBlob *getBytecodeBlob() const { return m_bytecode.Get(); }

  // 有効性チェック
  bool isValid() const { return m_bytecode != nullptr; }

private:
  ShaderType m_type = ShaderType::Vertex;
  std::string m_entryPoint;
  std::string m_filePath;
  ComPtr<ID3DBlob> m_bytecode;

  // 内部初期化
  [[nodiscard]] Utils::VoidResult
  initialize(const std::string &shaderCode, const std::string &entryPoint,
             ShaderType type, const std::vector<ShaderMacro> &macros,
             bool enableDebug, const std::string &filePath = "");

  // ヘルパー関数
  static std::string shaderTypeToTarget(ShaderType type);
  static std::vector<D3D_SHADER_MACRO>
  convertMacros(const std::vector<ShaderMacro> &macros);
};

//=========================================================================
// シェーダーマネージャークラス
//=========================================================================
class ShaderManager {
public:
  ShaderManager() = default;
  ~ShaderManager() = default;

  // コピー・ムーブ禁止
  ShaderManager(const ShaderManager &) = delete;
  ShaderManager &operator=(const ShaderManager &) = delete;

  // 初期化
  [[nodiscard]] Utils::VoidResult initialize(Device *device);

  // シェーダー管理
  std::shared_ptr<Shader> loadShader(const ShaderCompileDesc &desc);
  std::shared_ptr<Shader> getShader(const std::string &name) const;
  bool hasShader(const std::string &name) const;
  void removeShader(const std::string &name);

  std::shared_ptr<Shader>
  compileFromString(const std::string &shaderCode,
                    const std::string &entryPoint, ShaderType type,
                    const std::string &shaderName = "InlineShader");

  // 統計情報
  size_t getShaderCount() const { return m_shaders.size(); }

  // 有効性チェック
  bool isValid() const { return m_initialized && m_device != nullptr; }

private:
  Device *m_device = nullptr;
  bool m_initialized = false;

  // シェーダーキャッシュ
  std::unordered_map<std::string, std::shared_ptr<Shader>> m_shaders;

  // 初期化ヘルパー
  [[nodiscard]] Utils::VoidResult createDefaultShaders();

  // ユーティリティ
  std::string generateShaderKey(const ShaderCompileDesc &desc) const;
};

//=========================================================================
// 標準入力レイアウト定義
//=========================================================================
namespace StandardInputLayouts {
// 位置のみ
extern const std::vector<D3D12_INPUT_ELEMENT_DESC> Position;

// 位置 + UV
extern const std::vector<D3D12_INPUT_ELEMENT_DESC> PositionUV;

// 位置 + 法線 + UV
extern const std::vector<D3D12_INPUT_ELEMENT_DESC> PositionNormalUV;

// PBR用（位置 + 法線 + UV + 接線）
extern const std::vector<D3D12_INPUT_ELEMENT_DESC> PBRVertex;
} // namespace StandardInputLayouts

//=========================================================================
// ユーティリティ関数
//=========================================================================

// ファイルの読み込み
Utils::Result<std::string> readShaderFile(const std::string &filePath);

// インクルードファイルの処理
std::string processIncludes(const std::string &shaderCode,
                            const std::string &baseDir);
} // namespace Renderer
