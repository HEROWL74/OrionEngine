//src/Graphics/Material.hpp
#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <wrl.h>
#include <d3d12.h>
#include "../Math/Math.hpp"
#include "../Utils/Common.hpp"
#include "Device.hpp"

using Microsoft::WRL::ComPtr;

namespace Engine::Graphics
{
	//前方宣言
	class Texture;

	//======================================================================
	//テクスチャタイプ列挙型
	//======================================================================
	enum class TextureType
	{
		Albedo, //ベースカラー（拡散反射色）
		Normal, //法線マップ
		Metallic, //メタリック
		Roughness,//ラフネス
		AO,      //アンビエントオクルージョン
		Emissive, //発光
		Height,   //高さマップ
		Count  //テクスチャの数の計算用
	};

	//======================================================================
	//マテリアルプロパティ構造体
	//======================================================================
	struct MaterialProperties
	{
		//PBRパラメータ
		Math::Vector3 albedo = Math::Vector3(1.0f, 1.0f, 1.0f);  //ベースカラー
		float metallic = 0.0f;  //メタリック値（0.0 == 非金属, 1.0 == 金属）
		float roughness = 0.5f; //ラフネス値（0.0 == 完全に滑らか, 1.0 == 完全に粗い）
		float ao = 1.0f;  //アンビエントオクルージョン

		//発光
		Math::Vector3 emissive = Math::Vector3(0.0f, 0.0f, 0.0f); //発光色
		float emissiveStrength = 1.0f;  //発光強度

		//その他のプロパティ
		float normalStrength = 1.0f;  //法線マップの強度
		float heightScale = 0.05f;    //高さマップのスケール

		//アルファ関連
		float alpha = 1.0f;    //透明度
		bool useAlphaTest = false; //アルファテストを使用するかしないか
		float alphaTestThreshold = 0.5f;  //アルファテストの閾値（いきち）

		//テクスチャのタイリング
		Math::Vector2 uvScale = Math::Vector2(1.0f, 1.0f);  //UVスケール
		Math::Vector2 uvOffset = Math::Vector2(0.0f, 0.0f); //UVオフセット

		int useAlbedoTex = 0;  //0=使わない, 1=使う
		int pad[3]{};   //アラインメント用
	};

	//======================================================================
	//マテリアルクラス
	//======================================================================
	class Material
	{
	public:
		Material(const std::string& name = "DefaultMaterial");
		~Material() = default;

		// コピー・ムーブ
		Material(const Material&) = delete;
		Material& operator=(const Material&) = delete;
		Material(Material&&) = default;
		Material& operator=(Material&&) = default;

		// 初期化
		[[nodiscard]] Utils::VoidResult initialize(Device* device);

		// 基本情報
		const std::string& getName() const { return m_name; }
		void setName(const std::string& name) { m_name = name; }

		// マテリアルプロパティ
		const MaterialProperties& getProperties() const { return m_properties; }
		MaterialProperties& getProperties() { return m_properties; }
		void setProperties(const MaterialProperties& properties);

		// 個別プロパティアクセス
		void setAlbedo(const Math::Vector3& albedo) { m_properties.albedo = albedo; m_isDirty = true; }
		void setMetallic(float metallic) { m_properties.metallic = metallic; m_isDirty = true; }
		void setRoughness(float roughness) { m_properties.roughness = roughness; m_isDirty = true; }
		void setEmissive(const Math::Vector3& emissive) { m_properties.emissive = emissive; m_isDirty = true; }

		// テクスチャ管理
		void setTexture(TextureType type, std::shared_ptr<Texture> texture);
		std::shared_ptr<Texture> getTexture(TextureType type) const;
		bool hasTexture(TextureType type) const;
		void removeTexture(TextureType type);

		// GPU定数バッファの更新
		[[nodiscard]] Utils::VoidResult updateConstantBuffer();
		ID3D12Resource* getConstantBuffer() const { return m_constantBuffer.Get(); }

		// シェーダーリソースビューの取得
		D3D12_GPU_DESCRIPTOR_HANDLE getSRVGpuHandle() const { return m_srvGpuHandle; }
		D3D12_CPU_DESCRIPTOR_HANDLE getSRVCpuHandle() const { return m_srvCpuHandle; }

		// 有効性チェック
		bool isValid() const { return m_initialized && m_device != nullptr; }

		// マテリアルの適用（レンダリング時に呼ぶ）
		void bind(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex) const;

		friend class MaterialManager;
		void setDirty(bool dirty = true) { m_isDirty = dirty; }

		// ファイル入出力（後で実装）
		[[nodiscard]] Utils::VoidResult saveToFile(const std::string& filePath) const;
		[[nodiscard]] Utils::VoidResult loadFromFile(const std::string& filePath);
	private:
		std::string m_name;
		MaterialProperties m_properties;
		Device* m_device = nullptr;
		bool m_initialized = false;
		bool m_isDirty = true;

		//テクスチャマップ
		std::unordered_map<TextureType, std::shared_ptr<Texture>> m_textures;

		//GPUリソース
		ComPtr<ID3D12Resource> m_constantBuffer;
		void* m_constantBufferData = nullptr;

		//ディスクリプタハンドル
		D3D12_CPU_DESCRIPTOR_HANDLE m_srvCpuHandle{};
		D3D12_GPU_DESCRIPTOR_HANDLE m_srvGpuHandle{};

		ComPtr<ID3D12DescriptorHeap> m_cbvDescriptorHeap;
		D3D12_GPU_DESCRIPTOR_HANDLE m_cbvGpuHandle{};

		ComPtr<ID3D12DescriptorHeap> m_srvDescriptorHeap;  // テクスチャ用SRVヒープ


		//初期化ヘルパー
	    [[nodiscard]] Utils::VoidResult createConstantBuffer();
		[[nodiscard]] Utils::VoidResult createDescriptors();

		//GPU定数バッファ用の構造体
		struct MaterialConstantBuffer
		{
			Math::Vector4 albedo; //xyz: ベースカラー w: メタリック
			Math::Vector4 roughnessAoEmissiveStrength; //x:ラフネス y:アンビエントオクルージョン z: 発光強度 w: パディング
			Math::Vector4 emissive;  //xyz: 発光 w: 法線強度
			Math::Vector4 alphaParams;  // x：アルファ値（透明度) // y：アルファテストを使うかどうか（true / false）// z：アルファテストのしきい値（カットオフ値)// w：ハイトマップの高さスケール
			Math::Vector4 uvTransform; //xy: UVスケール zw: UVオフセット
			int hasAlbedoTexture;
			int pad[3];
		};
	};

	//============================================================
	//マテリアルマネージジャークラス
	//============================================================
	class MaterialManager
	{
	public:
		MaterialManager() = default;
		~MaterialManager() = default;

		//コピー・ムーブ禁止
		MaterialManager(const MaterialManager&) = delete;
		MaterialManager& operator=(const MaterialManager&) = delete;

		//初期化
		[[nodiscard]] Utils::VoidResult initialize(Device* device);

		//マテリアル管理
		std::shared_ptr<Material> createMaterial(const std::string& name);
		std::shared_ptr<Material> getMaterial(const std::string& name) const;
		bool hasMaterial(const std::string& name) const;
		void removeMaterial(const std::string& name);

		//デフォルトマテリアル
          std::shared_ptr<Material> getDefaultMaterial() const { return m_defaultMaterial; }

		  //全マテリアルの取得
		  const std::unordered_map<std::string, std::shared_ptr<Material>>& getAllMaterial() const { return m_materials; };

		  //リソースの更新
		  void updateAllMaterials();

		

		  //有効性チェック
		  bool isValid() const { return m_initialized && m_device != nullptr; }

	private:
		Device* m_device = nullptr;
		bool m_initialized = false;

		std::unordered_map<std::string, std::shared_ptr<Material>> m_materials;
		std::shared_ptr<Material> m_defaultMaterial;

		[[nodiscard]] Utils::VoidResult createDefaultMaterial();
	};

	//============================================================
	//ユーティリティ関数
	//============================================================
    
	//TextureTypeを文字列に変換
	std::string textureTypeToString(TextureType type);

	//文字列をTextureTypeに変換
	TextureType stringToTextureType(const std::string& str);
}

