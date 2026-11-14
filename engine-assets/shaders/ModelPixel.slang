// shaders/ModelPixel.slang

import ModelCommon;

ConstantBuffer<CameraConstants> gCamera : register(b0);

ConstantBuffer<MaterialConstants> gMaterial : register(b2);

// t0-t6: 7スロット確保 (余裕スロット含む)
Texture2D gTexAlbedo    : register(t0);
Texture2D gTexNormal    : register(t1);
Texture2D gTexMetallic  : register(t2);
Texture2D gTexRoughness : register(t3);
Texture2D gTexAO        : register(t4);
Texture2D gTexEmissive  : register(t5);
Texture2D gTexReserved  : register(t6); // 予約スロット

SamplerState gSampler : register(s0);

// -----------------------------------------------
// Pixel Input (VSOutput と一致)
// -----------------------------------------------
struct PSInput
{
    float4 positionCS  : SV_Position;
    float3 positionWS  : TEXCOORD0;
    float3 normalWS    : TEXCOORD1;
    float2 uv          : TEXCOORD2;
    float3 tangentWS   : TEXCOORD3;
    float3 bitangentWS : TEXCOORD4;
};

// -----------------------------------------------
// Pixel Output
// -----------------------------------------------
struct PSOutput
{
    float4 color : SV_Target0;
};

// -----------------------------------------------
// ノーマルマップからワールド法線を再構成
// -----------------------------------------------
float3 perturbNormal(PSInput input)
{
    float3 tangentNormal = gTexNormal.Sample(gSampler, input.uv).xyz;
    tangentNormal = tangentNormal * 2.0 - 1.0; // [0,1] -> [-1,1]

    float3x3 TBN = float3x3(
        normalize(input.tangentWS),
        normalize(input.bitangentWS),
        normalize(input.normalWS)
    );
    return normalize(mul(tangentNormal, TBN));
}

// -----------------------------------------------
// 簡易PBR (Cook-Torrance)
// -----------------------------------------------
static const float PI = 3.14159265359;

float distributionGGX(float3 N, float3 H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float geometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return geometrySchlickGGX(NdotV, roughness)
         * geometrySchlickGGX(NdotL, roughness);
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

// -----------------------------------------------
// main
// -----------------------------------------------
[shader("pixel")]
PSOutput main(PSInput input)
{
    PSOutput output;

    // テクスチャサンプル
    float4 albedoSample    = gTexAlbedo.Sample(gSampler, input.uv);
    float  metallicSample  = gTexMetallic.Sample(gSampler, input.uv).r;
    float  roughnessSample = gTexRoughness.Sample(gSampler, input.uv).r;
    float  aoSample        = gTexAO.Sample(gSampler, input.uv).r;
    float3 emissiveSample  = gTexEmissive.Sample(gSampler, input.uv).rgb;

    // マテリアル値 (テクスチャ × CBパラメータ)
    float3 albedo    = albedoSample.rgb * gMaterial.baseColor.rgb;
    float  metallic  = metallicSample  * gMaterial.metallic;
    float  roughness = roughnessSample * gMaterial.roughness;
    float  ao        = aoSample;

    float3 N = perturbNormal(input);
    float3 V = normalize(gCamera.cameraPos - input.positionWS);

    // F0: 非金属=0.04, 金属=albedo
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    // 単一ディレクショナルライト (MaterialCBから取得)
    float3 L  = normalize(-gMaterial.lightDir);
    float3 H  = normalize(V + L);
    float3 radiance = gMaterial.lightColor * gMaterial.lightIntensity;

    float  NDF = distributionGGX(N, H, roughness);
    float  G   = geometrySmith(N, V, L, roughness);
    float3 F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 numerator   = NDF * G * F;
    float  denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular    = numerator / denominator;

    float3 kD = (float3(1.0, 1.0, 1.0) - F) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);
    float3 Lo   = (kD * albedo / PI + specular) * radiance * NdotL;

    // アンビエント
    float3 ambient = float3(0.03, 0.03, 0.03) * albedo * ao;

    float3 color = ambient + Lo + emissiveSample * gMaterial.emissiveIntensity;

    // トーンマッピング (Reinhard) + ガンマ補正
    color = color / (color + float3(1.0, 1.0, 1.0));
    color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));

    output.color = float4(color, albedoSample.a);
    return output;
}