// shaders/ModelVertex.slang

import ModelCommon;

ConstantBuffer<CameraConstants> gCamera : register(b0);

ConstantBuffer<ObjectConstants> gObject : register(b1);

// -----------------------------
// Vertex Input
// -----------------------------
struct VSInput
{
  float3 position : POSITION;
  float3 normal : NORMAL;
  float2 uv : TEXCOORD;
  float3 tangent : TANGENT;
};

// -----------------------------
// Vertex Output
// -----------------------------
struct VSOutput
{
  float4 positionCS : SV_POSITION; // Clip Space
  float3 positionWS : TEXCOORD0;   // World Space
  float3 normalWS : TEXCOORD1;
  float2 uv : TEXCOORD2;
  float3 tangentWS : TEXCOORD3;
  float3 bitangentWS : TEXCOORD4;
};

// -----------------------------
// main
// -----------------------------
[shader("vertex")]
VSOutput main(VSInput input)
{
   VSOutput output;

   float4 worldPos = mul(gObject.world, float4(input.position, 1.0));
   output.positionWS = worldPos.xyz;
   output.positionCS = mul(gCamera.viewProj, worldPos);
   // 法線・接線をワールド空間へ
   float3x3 normalMatrix = (float3x3)gObject.worldInverseTranspose;
   output.normalWS = normalize(mul(normalMatrix, input.normal));
   output.tangentWS = normalize(mul(normalMatrix, input.tangent));

   output.bitangentWS = cross(output.normalWS, output.tangentWS);
   output.uv = input.uv;
   return output;
}