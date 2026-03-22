// engine/UI/UITextRenderer.hpp
#pragma once
#include "Device.hpp"
#include "ShaderManager.hpp"
#include "../engine/Utils/Common.hpp"
#include "RenderContext.hpp"
#include "../engine/UI/UIComponent.hpp"
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <unordered_map>

using Microsoft::WRL::ComPtr;

namespace Renderer
{
    using namespace Engine;

    struct FontAtlas
    {
        ComPtr<ID3D12Resource> texture;
        D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle;
        uint32_t width = 0;
        uint32_t height = 0;

        struct GlyphInfo
        {
            float x0, y0, x1, y1;
            float xoff, yoff;
            float xadvance;
            float width, height;
        };
        std::unordered_map<int, GlyphInfo> glyphs;

        float fontSize = 0.0f;
        float ascent = 0.0f;
        float descent = 0.0f;
        float lineGap = 0.0f;
    };

    struct TextVertex
    {
        Math::Vector3 position;
        Math::Vector2 texCoord;
        Math::Vector4 color;
    };

    struct TextConstants
    {
        Math::Matrix4 world;
        Math::Matrix4 viewProjection;
        Math::Vector4 color;
    };

    class UITextRenderer
    {
    public:
        UITextRenderer() = default;
        ~UITextRenderer();

        [[nodiscard]] Utils::VoidResult initialize(
            Device* device,
            ShaderManager* shaderManager);

        void beginFrame()
        {
            m_currentVertexOffset = 0;
            m_currentUITextIndex = 0;
        }

        void draw(
            Utils::RenderContext& context,
            const EngineUI::UIText& text,
            uint32_t screenWidth,
            uint32_t screenHeight,
            const World::Camera* camera);

        void release();
        bool isInitialized() const { return m_initialized; }

    private:
        bool m_initialized = false;
        Device* m_device = nullptr;
        ShaderManager* m_shaderManager = nullptr;

        std::unordered_map<int, std::unique_ptr<FontAtlas>> m_fontAtlases;
        std::vector<uint8_t> m_fontData;

        ComPtr<ID3D12RootSignature> m_rootSignature;
        ComPtr<ID3D12PipelineState> m_pipelineState;

        // 蜍慕噪鬆らせ繝舌ャ繝輔ぃ・亥推謠冗判縺ｧ菴ｿ逕ｨ・・
        ComPtr<ID3D12Resource> m_vertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};
        uint32_t m_maxVertices = 10000;
        TextVertex* m_vertexBufferMapped = nullptr;

        ComPtr<ID3D12DescriptorHeap> m_srvHeap;
        uint32_t m_srvDescriptorSize = 0;
        uint32_t m_currentSrvIndex = 0;

        uint32_t m_currentVertexOffset = 0;

        // 螳壽焚繝舌ャ繝輔ぃ・医ヵ繝ｬ繝ｼ繝縺斐→・・
        static constexpr uint32_t kFrameCount = 3;
        static constexpr uint32_t kMaxUITexts = 100;
        struct FrameConstantBuffer
        {
            ComPtr<ID3D12Resource> buffer;
            TextConstants* mapped = nullptr;
        };
        std::array<std::array<FrameConstantBuffer, kMaxUITexts>, kFrameCount> m_frameCBs;
        uint32_t m_currentUITextIndex = 0;

    private:
        [[nodiscard]] Utils::VoidResult loadFont(const std::string& fontPath);
        [[nodiscard]] Utils::VoidResult createFontAtlas(float fontSize);
        [[nodiscard]] Utils::VoidResult createRootSignature();
        [[nodiscard]] Utils::VoidResult createPipelineState();
        [[nodiscard]] Utils::VoidResult createVertexBuffer();
        [[nodiscard]] Utils::VoidResult createConstantBuffer();
        [[nodiscard]] Utils::VoidResult createDescriptorHeap();

        FontAtlas* getOrCreateAtlas(float fontSize);
    };
}

