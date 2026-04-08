// src/Graphics/Material.hpp
#pragma once

#include "../engine/Math/Math.hpp"
#include "../engine/Utils/Common.hpp"
#include "Device.hpp"
#include <d3d12.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <wrl.h>


using Microsoft::WRL::ComPtr;

namespace Renderer {
using namespace Engine;

// 前方宣言
class Texture;

//======================================================================
// テクスチャタイプ列挙型
//======================================================================
enum class TextureType {
  Albedo,    // ベースカラー（拡散反射色）
  Normal,    // 法線マップ
  Metallic,  // メタリック
  Roughness, // ラフネス
  AO,        // アンビエントオクルージョン
  Emissive,  // 発光
  Height,    // 高さマップ
  Count      // テクスチャ数の計算用
};

//======================================================================
// マテリアルプロパティ構造体
//======================================================================
struct MaterialProperties {
  // PBRパラメータ
  Math::Vector3 albedo = Math::Vector3(1.0f, 1.0f, 1.0f);
  float metallic = 0.0f;
  float roughness = 0.5f;
  float ao = 1.0f;

  // 発光
  Math::Vector3 emissive = Math::Vector3(0.0f, 0.0f, 0.0f);
  float emissiveStrength = 1.0f;

  // その他
  float normalStrength = 1.0f;
  float heightScale = 0.05f;

  // アルファ関連
  float alpha = 1.0f;
  bool useAlphaTest = false;
  float alphaTestThreshold = 0.5f;

  // テクスチャのタイリング
  Math::Vector2 uvScale = Math::Vector2(1.0f, 1.0f);
  Math::Vector2 uvOffset = Math::Vector2(0.0f, 0.0f);

  int useAlbedoTex = 0; // 0=使わない, 1=使う
  int pad[3]{};         // アラインメント用
};

//======================================================================
// マテリアルクラス
//
// 【設計方針】
// DescriptorHeap は Device が持つグローバル SRV ヒープを使う。
// Material は独自の DescriptorHeap を作らない。
// テクスチャの SRV ハンドルは Texture クラス自身が保持しているため、
// Material は getTexture() で取得した Texture* から getSRVHandle() を
// 呼ぶだけでよい。
//======================================================================
class Material {
public:
  explicit Material(const std::string &name = "DefaultMaterial");
  ~Material() = default;

  Material(const Material &) = delete;
  Material &operator=(const Material &) = delete;
  Material(Material &&) = default;
  Material &operator=(Material &&) = default;

  // 初期化（定数バッファの作成のみ行う）
  [[nodiscard]] Utils::VoidResult initialize(Device *device);

  // 基本情報
  const std::string &getName() const { return m_name; }
  void setName(const std::string &n) { m_name = n; }

  // マテリアルプロパティ
  const MaterialProperties &getProperties() const { return m_properties; }
  MaterialProperties &getProperties() { return m_properties; }
  void setProperties(const MaterialProperties &properties);

  // 個別プロパティアクセス
  void setAlbedo(const Math::Vector3 &v) {
    m_properties.albedo = v;
    m_isDirty = true;
  }
  void setMetallic(float v) {
    m_properties.metallic = v;
    m_isDirty = true;
  }
  void setRoughness(float v) {
    m_properties.roughness = v;
    m_isDirty = true;
  }
  void setEmissive(const Math::Vector3 &v) {
    m_properties.emissive = v;
    m_isDirty = true;
  }

  // テクスチャ管理
  // テクスチャの SRV は Texture クラスが自前で保持している。
  // 描画時は getTexture(type)->getSRVHandle() を使うこと。
  void setTexture(TextureType type, std::shared_ptr<Texture> texture);
  std::shared_ptr<Texture> getTexture(TextureType type) const;
  bool hasTexture(TextureType type) const;
  void removeTexture(TextureType type);

  // GPU 定数バッファ
  [[nodiscard]] Utils::VoidResult updateConstantBuffer();
  ID3D12Resource *getConstantBuffer() const { return m_constantBuffer.Get(); }

  // 有効性チェック
  bool isValid() const { return m_initialized && m_device != nullptr; }

  // 定数バッファを指定スロットにバインドする
  void bind(ID3D12GraphicsCommandList *commandList,
            UINT rootParameterIndex) const;

  void setDirty(bool dirty = true) { m_isDirty = dirty; }
  friend class MaterialManager;

  // ファイル入出力（未実装）
  [[nodiscard]] Utils::VoidResult saveToFile(const std::string &filePath) const;
  [[nodiscard]] Utils::VoidResult loadFromFile(const std::string &filePath);

private:
  std::string m_name;
  MaterialProperties m_properties;
  Device *m_device = nullptr;
  bool m_initialized = false;
  bool m_isDirty = true;

  // テクスチャマップ（SRV ハンドルは Texture 側が保持）
  std::unordered_map<TextureType, std::shared_ptr<Texture>> m_textures;

  // GPU 定数バッファ
  ComPtr<ID3D12Resource> m_constantBuffer;
  void *m_constantBufferData = nullptr;

  // 定数バッファ作成
  [[nodiscard]] Utils::VoidResult createConstantBuffer();

  // GPU 側へ転送する定数バッファレイアウト
  struct MaterialConstantBuffer {
    Math::Vector4 albedo; // xyz: ベースカラー, w: メタリック
    Math::Vector4 roughnessAoEmissiveStrength; // x: ラフネス, y: AO, z:
                                               // 発光強度, w: (未使用)
    Math::Vector4 emissive;                    // xyz: 発光色, w: 法線強度
    Math::Vector4
        alphaParams; // x: alpha, y: useAlphaTest, z: threshold, w: heightScale
    Math::Vector4 uvTransform; // xy: scale, zw: offset
    int hasAlbedoTexture;
    int pad[3];
  };
};

//======================================================================
// マテリアルマネージャー
//======================================================================
class MaterialManager {
public:
  MaterialManager() = default;
  ~MaterialManager() = default;

  MaterialManager(const MaterialManager &) = delete;
  MaterialManager &operator=(const MaterialManager &) = delete;

  [[nodiscard]] Utils::VoidResult initialize(Device *device);

  // マテリアル管理
  std::shared_ptr<Material> createMaterial(const std::string &name);
  std::shared_ptr<Material> getMaterial(const std::string &name) const;
  bool hasMaterial(const std::string &name) const;
  void removeMaterial(const std::string &name);

  std::shared_ptr<Material> getDefaultMaterial() const {
    return m_defaultMaterial;
  }

  const std::unordered_map<std::string, std::shared_ptr<Material>> &
  getAllMaterial() const {
    return m_materials;
  }

  void updateAllMaterials();

  bool isValid() const { return m_initialized && m_device != nullptr; }

private:
  Device *m_device = nullptr;
  bool m_initialized = false;

  std::unordered_map<std::string, std::shared_ptr<Material>> m_materials;
  std::shared_ptr<Material> m_defaultMaterial;

  [[nodiscard]] Utils::VoidResult createDefaultMaterial();
};

//======================================================================
// ユーティリティ関数
//======================================================================
std::string textureTypeToString(TextureType type);
TextureType stringToTextureType(const std::string &str);
} // namespace Renderer