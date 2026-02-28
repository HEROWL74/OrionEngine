//src/Graphics/Texture.hpp
#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include "../Utils/Common.hpp"
#include "Device.hpp"
#include "editor/UI/imgui.h"

using Microsoft::WRL::ComPtr;

namespace Engine::Graphics
{
    //=========================================================================
    // テクスチャフォーマット列挙型
    //=========================================================================
    enum class TextureFormat
    {
        Unknown,
        R8G8B8A8_UNORM,     // 一般的なRGBAフォーマット
        R8G8B8A8_SRGB,      // sRGB色空間
        R8G8B8_UNORM,       // RGB（アルファなし）
        R8_UNORM,           // グレースケール
        R16G16B16A16_FLOAT, // HDR用
        BC1_UNORM,          // DXT1圧縮
        BC3_UNORM,          // DXT5圧縮
        BC7_UNORM,          // 高品質圧縮
        D32_FLOAT,          // 深度バッファ用
        D24_UNORM_S8_UINT   // 深度ステンシル用
    };

    //=========================================================================
    // テクスチャタイプ列挙型
    //=========================================================================
    enum class TextureDimension
    {
        Texture2D,
        TextureCube,
        Texture2DArray,
        Texture3D
    };

    //=========================================================================
    // テクスチャ使用法フラグ
    //=========================================================================
    enum class TextureUsage : uint32_t
    {
        None = 0,
        ShaderResource = 1 << 0,    // シェーダーリソースとして使用
        RenderTarget = 1 << 1,      // レンダーターゲットとして使用
        DepthStencil = 1 << 2,      // 深度ステンシルとして使用
        UnorderedAccess = 1 << 3,   // コンピュートシェーダーでの読み書き
        CopySource = 1 << 4,        // コピー元として使用
        CopyDestination = 1 << 5    // コピー先として使用
    };

    // ビット演算子のオーバーロード
    constexpr TextureUsage operator|(TextureUsage a, TextureUsage b)
    {
        return static_cast<TextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    constexpr TextureUsage operator&(TextureUsage a, TextureUsage b)
    {
        return static_cast<TextureUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    //=========================================================================
    // テクスチャ作成設定
    //=========================================================================
    struct TextureDesc
    {
        uint32_t width = 1;
        uint32_t height = 1;
        uint32_t depth = 1;
        uint32_t mipLevels = 1;
        uint32_t arraySize = 1;
        TextureFormat format = TextureFormat::R8G8B8A8_UNORM;
        TextureDimension dimension = TextureDimension::Texture2D;
        TextureUsage usage = TextureUsage::ShaderResource;
        bool generateMips = false;
        std::string debugName;
    };

    //=========================================================================
    // 画像データ構造体
    //=========================================================================
    struct ImageData
    {
        std::vector<uint8_t> pixels;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t channels = 0;
        TextureFormat format = TextureFormat::Unknown;
        uint32_t rowPitch = 0;      // 1行のバイト数
        uint32_t slicePitch = 0;    // 1スライスのバイト数
    };

    //=========================================================================
    // テクスチャクラス
    //=========================================================================
    class Texture
    {
    public:
        Texture() = default;
        ~Texture() = default;

        // コピー・ムーブ
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&&) = default;
        Texture& operator=(Texture&&) = default;

        // ファイルからの作成
        [[nodiscard]] static Utils::Result<std::shared_ptr<Texture>> createFromFile(
            Device* device,
            const std::string& filePath,
            bool generateMips = true,
            bool sRGB = false
        );

        // メモリからの作成
        [[nodiscard]] static Utils::Result<std::shared_ptr<Texture>> createFromMemory(
            Device* device,
            const ImageData& imageData,
            const TextureDesc& desc
        );

        // 空のテクスチャの作成
        [[nodiscard]] static Utils::Result<std::shared_ptr<Texture>> create(
            Device* device,
            const TextureDesc& desc
        );

        // 基本情報の取得
        const TextureDesc& getDesc() const { return m_desc; }
        uint32_t getWidth() const { return m_desc.width; }
        uint32_t getHeight() const { return m_desc.height; }
        uint32_t getMipLevels() const { return m_desc.mipLevels; }
        TextureFormat getFormat() const { return m_desc.format; }

        // D3D12リソースの取得
        ID3D12Resource* getResource() const { return m_resource.Get(); }
        D3D12_GPU_DESCRIPTOR_HANDLE getSRVHandle() const { return m_srvHandle; }
        D3D12_GPU_DESCRIPTOR_HANDLE getRTVHandle() const { return m_rtvHandle; }
        D3D12_GPU_DESCRIPTOR_HANDLE getDSVHandle() const { return m_dsvHandle; }

        // 有効性チェック
        bool isValid() const { return m_resource != nullptr; }

        // デバッグ名の設定
        void setDebugName(const std::string& name);

    private:
        Device* m_device = nullptr;
        TextureDesc m_desc;
        ComPtr<ID3D12Resource> m_resource;

        // デスクリプタハンドル
        D3D12_GPU_DESCRIPTOR_HANDLE m_srvHandle{};
        D3D12_GPU_DESCRIPTOR_HANDLE m_rtvHandle{};
        D3D12_GPU_DESCRIPTOR_HANDLE m_dsvHandle{};


        // 初期化（内部使用）
        [[nodiscard]] Utils::VoidResult initialize(Device* device, const TextureDesc& desc);
        [[nodiscard]] Utils::VoidResult createResource();
        [[nodiscard]] Utils::VoidResult createViews();
        [[nodiscard]] Utils::VoidResult uploadData(const ImageData& imageData);
    };

    //=========================================================================
    // テクスチャローダークラス
    //=========================================================================
    class TextureLoader
    {
    public:
        TextureLoader() = default;
        ~TextureLoader() = default;

        // コピー・ムーブ禁止
        TextureLoader(const TextureLoader&) = delete;
        TextureLoader& operator=(const TextureLoader&) = delete;

        // 画像ファイルの読み込み
        [[nodiscard]] static Utils::Result<ImageData> loadFromFile(const std::string& filePath);

        // 各フォーマット別の読み込み
        [[nodiscard]] static Utils::Result<ImageData> loadPNG(const std::string& filePath);
        [[nodiscard]] static Utils::Result<ImageData> loadJPEG(const std::string& filePath);
        [[nodiscard]] static Utils::Result<ImageData> loadDDS(const std::string& filePath);
        [[nodiscard]] static Utils::Result<ImageData> loadTGA(const std::string& filePath);


        /*
        // ミップマップ生成
        [[nodiscard]] static Utils::Result<std::vector<ImageData>> generateMipmaps(const ImageData& baseImage);

        // フォーマット変換
        [[nodiscard]] static Utils::Result<ImageData> convertFormat(
            const ImageData& sourceImage,
            TextureFormat targetFormat
        );
        */
        // ファイル形式の判定
        static std::string getFileExtension(const std::string& filePath);
        static bool isSupportedFormat(const std::string& extension);

    private:
        // 内部ヘルパー関数
        static uint32_t calculateRowPitch(uint32_t width, TextureFormat format);
        static uint32_t calculateSlicePitch(uint32_t rowPitch, uint32_t height);
        static DXGI_FORMAT textureFormatToDXGI(TextureFormat format);
        static TextureFormat dxgiToTextureFormat(DXGI_FORMAT format);
    };

    //=========================================================================
    // テクスチャマネージャークラス
    //=========================================================================
    class TextureManager
    {
    public:
        TextureManager() = default;
        ~TextureManager() = default;

        // コピー・ムーブ禁止
        TextureManager(const TextureManager&) = delete;
        TextureManager& operator=(const TextureManager&) = delete;

        // 初期化
        [[nodiscard]] Utils::VoidResult initialize(Device* device);

        // テクスチャの作成・取得
        [[nodiscard]] std::shared_ptr<Texture> loadTexture(
            const std::string& filePath,
            bool generateMips = true,
            bool sRGB = false
        );

        std::shared_ptr<Texture> getTexture(const std::string& name) const;
        bool hasTexture(const std::string& name) const;
        void removeTexture(const std::string& name);

        // デフォルトテクスチャ
        std::shared_ptr<Texture> getWhiteTexture() const { return m_whiteTexture; }
        std::shared_ptr<Texture> getBlackTexture() const { return m_blackTexture; }
        std::shared_ptr<Texture> getDefaultNormalTexture() const { return m_defaultNormalTexture; }

        // 統計情報
        size_t getTextureCount() const { return m_textures.size(); }
        size_t getTotalMemoryUsage() const;

        // キャッシュクリア
        void clearCache();

        // 有効性チェック
        bool isValid() const { return m_initialized && m_device != nullptr; }

    private:
        Device* m_device = nullptr;
        bool m_initialized = false;

        // テクスチャキャッシュ
        std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;

        // デフォルトテクスチャ
        std::shared_ptr<Texture> m_whiteTexture;
        std::shared_ptr<Texture> m_blackTexture;
        std::shared_ptr<Texture> m_defaultNormalTexture;

        // 初期化ヘルパー
        [[nodiscard]] Utils::VoidResult createDefaultTextures();
        [[nodiscard]] Utils::VoidResult createSolidColorTexture(
            std::shared_ptr<Texture>& texture,
            uint32_t color,
            const std::string& name
        );
    };

    //=========================================================================
    // ユーティリティ関数
    //=========================================================================

    // フォーマット情報の取得
    uint32_t getBytesPerPixel(TextureFormat format);
    bool isCompressedFormat(TextureFormat format);
    bool isSRGBFormat(TextureFormat format);
    bool isDepthFormat(TextureFormat format);

    // DXGI_FORMAT変換
    DXGI_FORMAT textureFormatToDXGI(TextureFormat format);
    TextureFormat dxgiToTextureFormat(DXGI_FORMAT format);

    // テクスチャ使用法からD3D12フラグへの変換
    D3D12_RESOURCE_FLAGS textureUsageToD3D12Flags(TextureUsage usage);
    D3D12_RESOURCE_STATES textureUsageToD3D12State(TextureUsage usage);
}

