#pragma once
// renderer/VertexTypes.hpp
#pragma once

namespace Renderer {
// 頂点データ構造体
struct Vertex {
  float position[3]; // x, y, z
  float color[3];    // r, g, b
  float uv[2];
};

//======================================================================
// モデル描画用頂点 (position + normal + uv + tangent)
//
// 【なぜ CubeRenderer の Vertex と分けるのか】
//   CubeRenderer は頂点カラーを使う簡易描画が目的であり、
//   法線やタンジェントを必要としない。
//   一方 3D モデル（glTF / OBJ）は PBR ライティングのために
//   法線・UV・タンジェントが必須。
//   両者を同じ構造体に押し込むと、どちらかに不要なフィールドが
//   生まれ GPU のメモリ帯域が無駄になる。
//
// 【GPU 側のセマンティクス対応】
//   POSITION  (float3, offset  0)
//   NORMAL    (float3, offset 12)
//   TEXCOORD  (float2, offset 24)
//   TANGENT   (float4, offset 32) ← w で従法線の向きを符号化する
//
// 【サイズ】
//   12 + 12 + 8 + 16 = 48 bytes / vertex
//======================================================================
struct ModelVertex {
  float position[3]; // x, y, z
  float normal[3];   // nx, ny, nz
  float uv[2];       // ux, uy
  float tangent[4];  // tx, ty, tz. sign
};

//======================================================================
// 将来追加予定の頂点型
//   VertexPosTexture  : 2D スプライト / フルスクリーンパス用
//   VertexPosNormal   : 法線のみのシンプルライティング用
//======================================================================

struct VertexPosTexture {
  float position[3]; // x, y, z
  float texCoord[2]; // u, v
};

struct VertexPosNormal {
  float position[3]; // x, y, z
  float normal[3];   // nx, ny, nz
  float color[3];    // r, g, b
};
} // namespace Renderer
