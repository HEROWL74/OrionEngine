//src/Graphics/RenderComponent.hpp
#pragma once

#include "../Core/GameObject.hpp"
#include "Device.hpp"
#include "Camera.hpp"
#include "TriangleRenderer.hpp"
#include "CubeRenderer.hpp"
#include "Material.hpp"
#include "ShaderManager.hpp"
namespace Engine::Graphics
{
	//レンダリング可能なオブジェクトの種類
	enum class RenderableType
	{
		Triangle,
		Cube,
		//SphereやPlaneも追加予定
	};

	//=========================================
	//レンダリングコンポーネント
	//=========================================
	class RenderComponent : public Core::Component
	{
	public:
		explicit RenderComponent(RenderableType type = RenderableType::Cube);
		~RenderComponent() = default;

		//初期化
		[[nodiscard]] Utils::VoidResult initialize(Device* device, ShaderManager* shaderManager);

		//レンダリング
		void render(ID3D12GraphicsCommandList* commandList, const Camera& camera, UINT frameIndex);

		//レンダリングタイプ
		RenderableType getRenderableType() const { return m_renderableType; }
		void setRenderableType(RenderableType type);

		void setMaterial(std::shared_ptr<Graphics::Material> material) { m_material = material; }
		std::shared_ptr<Graphics::Material> getMaterial() const { return m_material; }
		void setMaterialManager(Graphics::MaterialManager* manager) { m_materialManager = manager; }

		//色の設定TODO:（マテリアルに発展する予定）
		void setColor(const Math::Vector3& color) { m_color = color; }
		const Math::Vector3& getColor() const { return m_color; }

		//有効か無効か
		void setVisible(bool visible) { m_visible = visible; }
		bool isVisible() const { return m_visible; }

		//有効性チェック
		[[nodiscard]] bool isValid() const;

	private:
		Device* m_device = nullptr;
		ShaderManager* m_shaderManager = nullptr;  // ← これを追加
		RenderableType m_renderableType;
		Math::Vector3 m_color = Math::Vector3(1.0f, 1.0f, 1.0f);
		bool m_visible = true;
		bool m_initialized = false;
		Graphics::MaterialManager* m_materialManager = nullptr;

		//レンダラー
		std::unique_ptr<TriangleRenderer> m_triangleRenderer;
		std::unique_ptr<CubeRenderer> m_cubeRenderer;
		std::shared_ptr<Graphics::Material> m_material;

		//初期化ヘルパー
		[[nodiscard]] Utils::VoidResult initializeRenderer();
	};
}
