#pragma once
// src/Graphics/VertexTypes.hpp
#pragma once

namespace Engine::Graphics
{
    // 頂点データ構造体
    struct Vertex
    {
        float position[3];  // x, y, z
        float color[3];     // r, g, b
        float uv[2];
    };

    // 将来的にはテクスチャ座標付きの頂点も追加予定
    struct VertexPosTexture
    {
        float position[3];  // x, y, z
        float texCoord[2];  // u, v
    };

    // 法線付きの頂点（ライティング用）
    struct VertexPosNormal
    {
        float position[3];  // x, y, z
        float normal[3];    // nx, ny, nz
        float color[3];     // r, g, b
    };
}
