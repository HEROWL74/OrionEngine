// src/engine/EngineAssets/FontAsset.hpp
#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include "../Graphics/Texture.hpp"
#include "../Math/Math.hpp"
#include "../Utils/Common.hpp"

namespace Engine::EngineAssets
{
    // グリフ情報（1文字分のデータ）
    struct Glyph
    {
        Math::Vector2 size;      // グリフのサイズ (正規化済み 0-1)
        Math::Vector2 bearing;   // ベースラインからのオフセット (正規化済み)
        float advance;           // 次の文字への送り幅 (正規化済み)
        Math::Vector2 uvMin;     // テクスチャのUV座標 (左上)
        Math::Vector2 uvMax;     // テクスチャのUV座標 (右下)

        Glyph()
            : size(0.0f, 0.0f)
            , bearing(0.0f, 0.0f)
            , advance(0.0f)
            , uvMin(0.0f, 0.0f)
            , uvMax(0.0f, 0.0f)
        {
        }
    };

    // フォントアセット
    class FontAsset
    {
    public:
        FontAsset() = default;
        ~FontAsset() = default;

        // BMFont形式のフォントファイルを読み込む
        [[nodiscard]]
        Utils::VoidResult loadFromBMFont(
            Graphics::TextureManager* textureManager,
            const std::string& fntPath,
            const std::string& texturePath
        );

        // 文字のグリフ情報を取得
        const Glyph& getGlyph(char c) const;

        // フォントテクスチャを取得
        Graphics::Texture* getTexture() const { return m_texture.get(); }

        // テクスチャサイズを取得
        float getTextureWidth() const { return static_cast<float>(m_textureWidth); }
        float getTextureHeight() const { return static_cast<float>(m_textureHeight); }

    private:
        std::unordered_map<char, Glyph> m_glyphs;
        std::shared_ptr<Graphics::Texture> m_texture;

        int m_textureWidth = 0;
        int m_textureHeight = 0;
    };
}