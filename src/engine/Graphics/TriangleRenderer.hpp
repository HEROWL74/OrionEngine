// src/Graphics/TriangleRenderer.hpp
#pragma once

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <array>
#include "Device.hpp"
#include "ShaderManager.hpp"
#include "ConstantBuffer.hpp"
#include "Camera.hpp"
#include "Material.hpp"
#include "VertexTypes.hpp"
#include "../Math/Math.hpp"
#include "../Utils/Common.hpp"

using Microsoft::WRL::ComPtr;

namespace Engine::Graphics
{
    // 三角形描画専用のレンダラー
    class TriangleRenderer
    {
    public:
        TriangleRenderer() = default;
        ~TriangleRenderer() = default;

        // コピー・ムーブ禁止
        TriangleRenderer(const TriangleRenderer&) = delete;
        TriangleRenderer& operator=(const TriangleRenderer&) = delete;
        TriangleRenderer(TriangleRenderer&&) = delete;
        TriangleRenderer& operator=(TriangleRenderer&&) = delete;

        // 初期化
        [[nodiscard]] Utils::VoidResult initialize(Device* device, ShaderManager* shaderManager);

        // 三角形を描画
        void render(ID3D12GraphicsCommandList* commandList, const Camera& camera, UINT frameIndex);

        // 3D空間での位置・回転・スケールを設定
        void setPosition(const Math::Vector3& position) { m_position = position; updateWorldMatrix(); }
        void setRotation(const Math::Vector3& rotation) { m_rotation = rotation; updateWorldMatrix(); }
        void setScale(const Math::Vector3& scale) { m_scale = scale; updateWorldMatrix(); }

        //マテリアル設定
        void setMaterial(std::shared_ptr<Graphics::Material> material) { m_material = material; }
        void setMaterialManager(Graphics::MaterialManager* manager) { m_materialManager = manager; }

        // ゲッター
        const Math::Vector3& getPosition() const { return m_position; }
        const Math::Vector3& getRotation() const { return m_rotation; }
        const Math::Vector3& getScale() const { return m_scale; }

        // 有効かチェック
        [[nodiscard]] bool isValid() const noexcept { return m_rootSignature != nullptr && m_constantBufferManager.isValid(); }

    private:
        Device* m_device = nullptr;
        ShaderManager* m_shaderManager = nullptr;
        ConstantBufferManager m_constantBufferManager;

        // 3D変換パラメータ
        Math::Vector3 m_position = Math::Vector3::zero();
        Math::Vector3 m_rotation = Math::Vector3::zero();
        Math::Vector3 m_scale = Math::Vector3::one();
        Math::Matrix4 m_worldMatrix;

        //マテリアル管理ポインタ
        std::shared_ptr<Graphics::Material> m_material;
        Graphics::MaterialManager* m_materialManager = nullptr;

        // 描画リソース
        ComPtr<ID3D12RootSignature> m_rootSignature;        // ルートシグネチャ
        ComPtr<ID3D12PipelineState> m_pipelineState;        // パイプラインステート
        ComPtr<ID3D12Resource> m_vertexBuffer;              // 頂点バッファ
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};      // 頂点バッファビュー

        // 三角形の頂点データ
        std::array<Vertex, 6> m_triangleVertices;

        // 初期化用メソッド
        [[nodiscard]] Utils::VoidResult createRootSignature();
        [[nodiscard]] Utils::VoidResult createShaders();
        [[nodiscard]] Utils::VoidResult createPipelineState();
        [[nodiscard]] Utils::VoidResult createVertexBuffer();

        // 三角形の頂点データを設定
        void setupTriangleVertices();

        // ワールド行列を更新
        void updateWorldMatrix();
    };
}
