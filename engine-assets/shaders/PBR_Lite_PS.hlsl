//engine-assets/shaders/PBR_Lite_PS.hlsl
#include "BasicVertex.hlsl"

//MaterialConstants
cbuffer MaterialConstants : register(b2)
{
    float4 albedo; // xyz: baseColor, w: metallic
    float4 roughnessAoEmissiveStrength; // x: roughness, y: ao, z: emissiveStrength, w: padding
    float4 emissive; // xyz: emissive, w: normalStrength
    float4 alphaParams; // x: alpha, y: useAlphaTest, z: alphaTestThreshold, w: heightScale
    float4 uvTransform; // xy: uvScale, zw: uvOffset
    int hasAlbedoTexture;
    int pad0, pad1, pad2;
};

//SRV / Sampler (t0, s0)
Texture2D albedoTexture : register(t0);
SamplerState linearSampler : register(s0);

// ----PBRåvéZÅiç≈è¨Åj----------------------------
static const float PI = 3.14159265359;

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float D_GGX(float3 N, float3 H, float roughness)
{
    float a = roughness / roughness;
    float a2 = a * a;
    float NdotH = saturate(dot(N, H));
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom + 1e-6);
}

float4 main() : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}