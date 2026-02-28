//assets/shaders/Common.hlsli
//Same struct and constantbuffe
#ifndef COMMON_HLSLI_INCLUDED
#define COMMON_HLSLI_INCLUDED

//VertexInput Struct
struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float3 tangent : TANGENT;
};

//VertexOutput struct
struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 worldPos : WORLDPOS;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

//Scene Constant Buffer
cbuffer SceneConstants : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float3 cameraPos;
    float padding1;

    float3 directionalLightDir;
    float directionalLightIntensity;
    float3 directionalLightColor;
    float ambientInsensity;
    float3 ambientColor;
    float padding2;
}

//Object Constant Buffer
cbuffer ObjectConstants : register(b1)
{
    float4x4 worldMatrix;
    float4x4 normalMatrix;
};

//Material Constant Buffer 
cbuffer MaterialConstants : register(b2)
{
    float4 albedo;                          //xyz: albedo, w: metallic
    int hasAlbedoTexture;                 //0.0 or 1.0
    float4 roughnessAoEmissiveStrength;     //x: roughness, y: ao, z: emissiveStrength, w: padding
    float4 emissive;                        //xyz: emissive, w: normalStrength
    float4 alphaParams;                     //x: alpha, y; useAlphaTest, z:  alphaTestThreshold, w: heightScale
    float4 uvTransform;                     //xy: uvScale, zw: uvOffset
};

//Texture and Sampler
Texture2D albedoTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D metallicTexture : register(t2);
Texture2D roughnessTexture : register(t3);
Texture2D aoTexture : register(t4);
Texture2D emissiveTexture : register(t5);
SamplerState linearSampler : register(s0);

//Utility Function
float2 transformUV(float2 uv)
{
    return uv * uvTransform.xy + uvTransform.zw;
}

//transformNormal
float3 transformNormal(float3 normalMap, float3 tangent, float3 bitangent, float3 normal)
{
    //
    float3x3 TBN = float3x3(tangent, bitangent, normal);
    normalMap = normalMap * 2.0 - 1.0; //[0,1] -> [-1,1]
    normalMap.xy *= emissive.w;  //Apply NormalStrength
    return normalize(mul(normalMap, TBN));
}

float3 toLinear(float3 srgb)
{
    return pow(abs(srgb), 2.2);
}

float3 toSRGB(float3 linearColor)
{
    return pow(abs(linearColor), 1.0 / 2.2);
}

#endif // COMMON_HLSLI_INCLUDED