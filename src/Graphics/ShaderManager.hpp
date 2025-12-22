//src/Graphics/ShaderManager.hpp
#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <wrl.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include "../Utils/Common.hpp"
#include "Device.hpp"

using Microsoft::WRL::ComPtr;

namespace Engine::Graphics
{
    //=========================================================================
    // シェーダータイプ列挙型
    //=========================================================================
    enum class ShaderType
    {
        Vertex,
        Pixel,
        Geometry,
        Hull,
        Domain,
        Compute
    };

    //=========================================================================
    // シェーダーマクロ構造体
    //=========================================================================
    struct ShaderMacro
    {
        std::string name;
        std::string definition;

        ShaderMacro(const std::string& n, const std::string& d) : name(n), definition(d) {}
    };

    //=========================================================================
    // シェーダーコンパイル設定
    //=========================================================================
    struct ShaderCompileDesc
    {
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
    class Shader
    {
    public:
        Shader() = default;
        ~Shader() = default;

        // コピー・ムーブ
        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;
        Shader(Shader&&) = default;
        Shader& operator=(Shader&&) = default;

        // ファイルからのコンパイル
        [[nodiscard]] static Utils::Result<std::shared_ptr<Shader>> compileFromFile(
            const ShaderCompileDesc& desc
        );

        // 文字列からのコンパイル
        [[nodiscard]] static Utils::Result<std::shared_ptr<Shader>> compileFromString(
            const std::string& shaderCode,
            const std::string& entryPoint,
            ShaderType type,
            const std::vector<ShaderMacro>& macros = {},
            bool enableDebug = false
        );

        // 基本情報の取得
        ShaderType getType() const { return m_type; }
        const std::string& getEntryPoint() const { return m_entryPoint; }
        const std::string& getFilePath() const { return m_filePath; }

        // バイトコードの取得
        const void* getBytecode() const { return m_bytecode->GetBufferPointer(); }
        size_t getBytecodeSize() const { return m_bytecode->GetBufferSize(); }
        ID3DBlob* getBytecodeBlob() const { return m_bytecode.Get(); }

        // 有効性チェック
        bool isValid() const { return m_bytecode != nullptr; }

    private:
        ShaderType m_type = ShaderType::Vertex;
        std::string m_entryPoint;
        std::string m_filePath;
        ComPtr<ID3DBlob> m_bytecode;

        // 内部初期化
        [[nodiscard]] Utils::VoidResult initialize(
            const std::string& shaderCode,
            const std::string& entryPoint,
            ShaderType type,
            const std::vector<ShaderMacro>& macros,
            bool enableDebug,
            const std::string& filePath = ""
        );

        // ヘルパー関数
        static std::string shaderTypeToTarget(ShaderType type);
        static std::vector<D3D_SHADER_MACRO> convertMacros(const std::vector<ShaderMacro>& macros);
    };

    //=========================================================================
    // ルートシグネチャ記述
    //=========================================================================
    struct RootParameterDesc
    {
        enum Type
        {
            ConstantBufferView,
            ShaderResourceView,
            UnorderedAccessView,
            DescriptorTable,
            Constants
        };

        Type type;
        uint32_t shaderRegister;
        uint32_t registerSpace = 0;
        D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL;

        // DescriptorTable用
        std::vector<D3D12_DESCRIPTOR_RANGE1> ranges;

        // Constants用
        uint32_t numConstants = 0;
    };

    struct StaticSamplerDesc
    {
        uint32_t shaderRegister;
        uint32_t registerSpace = 0;
        D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_FILTER filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        D3D12_TEXTURE_ADDRESS_MODE addressModeU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        D3D12_TEXTURE_ADDRESS_MODE addressModeV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        D3D12_TEXTURE_ADDRESS_MODE addressModeW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    };

    //=========================================================================
    // パイプラインステート記述
    //=========================================================================
    struct PipelineStateDesc
    {
        std::shared_ptr<Shader> vertexShader;
        std::shared_ptr<Shader> pixelShader;
        std::shared_ptr<Shader> geometryShader;

        // 入力レイアウト
        std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;

        // ルートシグネチャ
        std::vector<RootParameterDesc> rootParameters;
        std::vector<StaticSamplerDesc> staticSamplers;

        // レンダーステート
        D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        std::vector<DXGI_FORMAT> rtvFormats = { DXGI_FORMAT_R8G8B8A8_UNORM };
        DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT;

        // ブレンドステート
        bool enableBlending = false;
        D3D12_BLEND srcBlend = D3D12_BLEND_ONE;
        D3D12_BLEND destBlend = D3D12_BLEND_ZERO;
        D3D12_BLEND_OP blendOp = D3D12_BLEND_OP_ADD;

        // ラスタライザーステート
        D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
        D3D12_FILL_MODE fillMode = D3D12_FILL_MODE_SOLID;
        bool enableDepthClip = true;

        // 深度ステンシルステート
        bool enableDepthTest = true;
        bool enableDepthWrite = true;
        D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS;

        std::string debugName;
    };

    //=========================================================================
    // パイプラインステートクラス
    //=========================================================================
    class PipelineState
    {
    public:
        PipelineState() = default;
        ~PipelineState() = default;

        // コピー・ムーブ
        PipelineState(const PipelineState&) = delete;
        PipelineState& operator=(const PipelineState&) = delete;
        PipelineState(PipelineState&&) = default;
        PipelineState& operator=(PipelineState&&) = default;

        // 作成
        [[nodiscard]] static Utils::Result<std::shared_ptr<PipelineState>> create(
            Device* device,
            const PipelineStateDesc& desc
        );

        // D3D12オブジェクトの取得
        ID3D12PipelineState* getPipelineState() const { return m_pipelineState.Get(); }
        ID3D12RootSignature* getRootSignature() const { return m_rootSignature.Get(); }

        // 基本情報
        const PipelineStateDesc& getDesc() const { return m_desc; }

        // 有効性チェック
        bool isValid() const { return m_pipelineState != nullptr && m_rootSignature != nullptr; }

        // デバッグ名の設定
        void setDebugName(const std::string& name);

    private:
        Device* m_device = nullptr;
        PipelineStateDesc m_desc;
        ComPtr<ID3D12PipelineState> m_pipelineState;
        ComPtr<ID3D12RootSignature> m_rootSignature;

        // 初期化
        [[nodiscard]] Utils::VoidResult initialize(Device* device, const PipelineStateDesc& desc);
        [[nodiscard]] Utils::VoidResult createRootSignature();
        [[nodiscard]] Utils::VoidResult createPipelineState();
    };

    //=========================================================================
    // シェーダーマネージャークラス
    //=========================================================================
    class ShaderManager
    {
    public:
        ShaderManager() = default;
        ~ShaderManager() = default;

        // コピー・ムーブ禁止
        ShaderManager(const ShaderManager&) = delete;
        ShaderManager& operator=(const ShaderManager&) = delete;

        // 初期化
        [[nodiscard]] Utils::VoidResult initialize(Device* device);

        // シェーダー管理
        std::shared_ptr<Shader> loadShader(const ShaderCompileDesc& desc);
        std::shared_ptr<Shader> getShader(const std::string& name) const;
        bool hasShader(const std::string& name) const;
        void removeShader(const std::string& name);

        // パイプラインステート管理
        std::shared_ptr<PipelineState> createPipelineState(
            const std::string& name,
            const PipelineStateDesc& desc
        );
        std::shared_ptr<PipelineState> getPipelineState(const std::string& name) const;
        bool hasPipelineState(const std::string& name) const;
        void removePipelineState(const std::string& name);

        std::shared_ptr<Shader> compileFromString(
            const std::string& shaderCode,
            const std::string& entryPoint,
            ShaderType type,
            const std::string& shaderName = "InlineShader"
        );

        // デフォルトシェーダーの取得
        std::shared_ptr<PipelineState> getDefaultPBRPipeline() const { return m_defaultPBRPipeline; }
        std::shared_ptr<PipelineState> getDefaultUnlitPipeline() const { return m_defaultUnlitPipeline; }

        // 統計情報
        size_t getShaderCount() const { return m_shaders.size(); }
        size_t getPipelineStateCount() const { return m_pipelineStates.size(); }

        // 有効性チェック
        bool isValid() const { return m_initialized && m_device != nullptr; }

    private:
        Device* m_device = nullptr;
        bool m_initialized = false;

        // シェーダーキャッシュ
        std::unordered_map<std::string, std::shared_ptr<Shader>> m_shaders;

        // パイプラインステートキャッシュ
        std::unordered_map<std::string, std::shared_ptr<PipelineState>> m_pipelineStates;

        // デフォルトパイプライン
        std::shared_ptr<PipelineState> m_defaultPBRPipeline;
        std::shared_ptr<PipelineState> m_defaultUnlitPipeline;

        // 初期化ヘルパー
        [[nodiscard]] Utils::VoidResult createDefaultShaders();
        [[nodiscard]] Utils::VoidResult createDefaultPipelines();

        // ユーティリティ
        std::string generateShaderKey(const ShaderCompileDesc& desc) const;
    };

    //=========================================================================
    // 標準入力レイアウト定義
    //=========================================================================
    namespace StandardInputLayouts
    {
        // 位置のみ
        extern const std::vector<D3D12_INPUT_ELEMENT_DESC> Position;

        // 位置 + UV
        extern const std::vector<D3D12_INPUT_ELEMENT_DESC> PositionUV;

        // 位置 + 法線 + UV
        extern const std::vector<D3D12_INPUT_ELEMENT_DESC> PositionNormalUV;

        // PBR用（位置 + 法線 + UV + 接線）
        extern const std::vector<D3D12_INPUT_ELEMENT_DESC> PBRVertex;
    }

    //=========================================================================
    // ユーティリティ関数
    //=========================================================================

    // ファイルの読み込み
    Utils::Result<std::string> readShaderFile(const std::string& filePath);

    // インクルードファイルの処理
    std::string processIncludes(const std::string& shaderCode, const std::string& baseDir);
}
