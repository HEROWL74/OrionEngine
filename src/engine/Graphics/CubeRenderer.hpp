// src/Graphics/CubeRenderer.hpp
#pragma once

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <array>
#include "Device.hpp"
#include "ShaderManager.hpp"
#include "ConstantBuffer.hpp"
#include "Material.hpp"
#include "Camera.hpp"
#include "VertexTypes.hpp"
#include "../Math/Math.hpp"
#include "../Utils/Common.hpp"
#include "Texture.hpp"

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

        // 初期化
        [[nodiscard]] Utils::VoidResult initialize(Device* device, ShaderManager* shaderManager);

        // 立方体を描画
        void render(ID3D12GraphicsCommandList* commandList, const Camera& camera, UINT frameIndex);

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

        // 有効かチェック
        [[nodiscard]] bool isValid() const noexcept {
            return m_rootSignature != nullptr &&
                m_pipelineState != nullptr &&
                m_vertexBuffer != nullptr &&
                m_indexBuffer != nullptr &&
                m_constantBufferManager.isValid();
        }

    private:
        Device* m_device = nullptr;
        ShaderManager* m_shaderManager = nullptr;
        ConstantBufferManager m_constantBufferManager;
        bool m_usePBRPipeline = false;

        // 3D変換パラメータ
        Math::Vector3 m_position = Math::Vector3::zero();
        Math::Vector3 m_rotation = Math::Vector3::zero();
        Math::Vector3 m_scale = Math::Vector3::one();
        Math::Matrix4 m_worldMatrix;

        //マテリアル管理ポインタ
        std::shared_ptr<Graphics::Material> m_material;
        Graphics::MaterialManager* m_materialManager = nullptr;

        // 描画リソース
        ComPtr<ID3D12RootSignature> m_rootSignature;
        ComPtr<ID3D12PipelineState> m_pipelineState;
        ComPtr<ID3D12Resource> m_vertexBuffer;
        ComPtr<ID3D12Resource> m_indexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};
        D3D12_INDEX_BUFFER_VIEW m_indexBufferView{};

        // 立方体の頂点データ（24頂点 - 各面に4頂点）
        std::array<Vertex, 24> m_cubeVertices;

        // 立方体のインデックスデータ（36インデックス - 12三角形 * 3頂点）
        std::array<uint16_t, 36> m_cubeIndices;

        // 初期化用メソッド
        [[nodiscard]] Utils::VoidResult createRootSignature();
        [[nodiscard]] Utils::VoidResult createPBRRootSignature();
        [[nodiscard]] Utils::VoidResult createShaders();
        [[nodiscard]] Utils::VoidResult createPipelineState();
        [[nodiscard]] Utils::VoidResult createVertexBuffer();
        [[nodiscard]] Utils::VoidResult createIndexBuffer();

        // 立方体の頂点データを設定
        void setupCubeVertices();

        // ワールド行列を更新
        void updateWorldMatrix();
    };
}
