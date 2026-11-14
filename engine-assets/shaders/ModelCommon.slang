// shaders/ModelCommon.slang
// VS/PS 共通の定数バッファ構造体定義

// ------------------------------
// b0: カメラ定数
// ------------------------------
struct CameraConstants
{
  float4x4 view;
  float4x4 proj;
  float4x4 viewProj;
  float3 cameraPos;
  float _pad0;
};

// ------------------------------
// b1: オブジェクト定数
// ------------------------------
struct ObjectConstants
{
  float4x4 world;
  float4x4 worldInverseTranspose;
};

// ------------------------------
// b2: マテリアル定数
// ------------------------------
struct MaterialConstants
{
  float4 baseColor;
  float metallic;
  float roughness;
  float emissiveIntensity;
  float _pad0;

  // ディレクショナルライト
  float3 lightDir; 
  float lightIntensity;
  float3 lightColor;
  float _pad1;
}