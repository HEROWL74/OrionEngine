// src/Graphics/CubeRenderer.hpp
#pragma once

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <array>
#include "Device.hpp"
#include "ShaderManager.hpp"
#include "PipelineStateCache.hpp"
#include "ConstantBuffer.hpp"
#include "Material.hpp"
#include "Camera.hpp"
#include "VertexTypes.hpp"
#include "../Math/Math.hpp"
#include "../Utils/Common.hpp"
#include "Texture.hpp"
#include "../Utils/RenderContext.hpp"

using Microsoft::WRL::ComPtr;

namespace Engine::Graphics
{
    // 立方体描画専用のレンダラー
    class CubeRenderer
    {
    public:
        CubeRenderer() = default;
        ~CubeRenderer() = default;

        // コピー・ムーブ禁止
        CubeRenderer(const CubeRenderer&) = delete;
        CubeRenderer& operator=(const CubeRenderer&) = delete;
        CubeRenderer(CubeRenderer&&) = delete;
        CubeRenderer& operator=(CubeRenderer&&) = delete;

        // 初期化 (PSO は render() 時に RenderContext から取得)
        [[nodiscard]] Utils::VoidResult initialize(Device* device);

        // 立方体を描画
        void render(const Utils::RenderContext& context);

        // 3D空間での位置・回転・スケールを設定
        void setPosition(const Math::Vector3& position) { m_position = position; updateWorldMatrix(); }
        void setRotation(const Math::Vector3& rotation) { m_rotation = rotation; updateWorldMatrix(); }
        void setScale(const Math::Vector3& scale) { m_scale = scale; updateWorldMatrix(); }

        void setMaterial(std::shared_ptr<Graphics::Material> material) { m_material = material; }
        void setMaterialManager(Graphics::MaterialManager* manager) { m_materialManager = manager; }

        // ゲッター
        const Math::Vector3& getPosition() const { return m_position; }
        const Math::Vector3& getRotation() const { return m_rotation; }
        const Math::Vector3& getScale() const { return m_scale; }

        // 有効性チェック
        [[nodiscard]] bool isValid() const noexcept
        {
            return m_vertexBuffer != nullptr
                && m_indexBuffer != nullptr
                && m_constantBufferManager.isValid();
        }

    private:
        Device* m_device = nullptr;
        ConstantBufferManager m_constantBufferManager;

        // 3D変換パラメータ
        Math::Vector3 m_position = Math::Vector3::zero();
        Math::Vector3 m_rotation = Math::Vector3::zero();
        Math::Vector3 m_scale = Math::Vector3::one();
        Math::Matrix4 m_worldMatrix;

        // マテリアル
        std::shared_ptr<Graphics::Material> m_material;
        Graphics::MaterialManager* m_materialManager = nullptr;

        // 頂点・インデックスバッファ
        ComPtr<ID3D12Resource>       m_vertexBuffer;
        ComPtr<ID3D12Resource>       m_indexBuffer;
        D3D12_VERTEX_BUFFER_VIEW     m_vertexBufferView{};
        D3D12_INDEX_BUFFER_VIEW      m_indexBufferView{};

        // 頂点データ（24頂点: 各面4頂点）
        std::array<Vertex, 24>    m_cubeVertices;
        // インデックスデータ（36個: 12三角形 × 3頂点）
        std::array<uint16_t, 36>  m_cubeIndices;

        // バッファ生成
        [[nodiscard]] Utils::VoidResult createVertexBuffer();
        [[nodiscard]] Utils::VoidResult createIndexBuffer();

        // 頂点データのセットアップ
        void setupCubeVertices();

        // ワールド行列の更新
        void updateWorldMatrix();
    };
}