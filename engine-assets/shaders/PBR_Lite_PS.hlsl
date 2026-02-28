// engine-assets/shaders/PBR_Lite_PS.hlsl
#include "BasicVertex.hlsl"  

// Material Constants
cbuffer MaterialConstants : register(b2)
{
    float4 albedo; // xyz: baseColor, w: metallic
    float4 roughnessAoEmissiveStrength; // x: roughness, y: ao, z: emissiveStrength, w: padding
    float4 emissive; // xyz: emissive, w: normalStrength(未使用)
    float4 alphaParams; // x: alpha, y: useAlphaTest, z: alphaTestThreshold, w: heightScale(未使用)
    float4 uvTransform; // xy: uvScale, zw: uvOffset
    int hasAlbedoTexture;
    int pad0, pad1, pad2;
};

// SRV / Sampler（t0 / s0）
Texture2D albedoTexture : register(t0);
SamplerState linearSampler : register(s0);

// ---- PBR計算（最小） ----------------------------------------
static const float PI = 3.14159265359;

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float D_GGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = saturate(dot(N, H));
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom + 1e-6);
}

float G_SchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k + 1e-6);
}

float G_Smith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    return G_SchlickGGX(NdotV, roughness) * G_SchlickGGX(NdotL, roughness);
}

float3 ACES(float3 c)
{
    const float a = 2.51, b = 0.03, d = 0.59, e = 0.14, cc = 2.43;
    return saturate((c * (a * c + b)) / (c * (cc * c + d) + e));
}

float3 toSRGB(float3 linearColor)
{
    return pow(abs(linearColor), 1.0 / 2.2);
}

// ---- Main ----------------------------------------------------
struct VertexOutput2
{
    float4 position : SV_POSITION;
    float3 worldPos : WORLD_POSITION;
    float3 color : COLOR;
    float2 uv : TEXCOORD0;
};

float4 main(VertexOutput2 input) : SV_TARGET
{
    // 最小の法線（面法線が無いので view-spaceのZを仮定 → 局所で(0,0,1) ）
    // ※ 本格導入時は頂点法線/タンジェントから再計算します
    float3 N = float3(0.0, 0.0, 1.0);

    // カメラ／行列は b0/b1 で受け取っている（BasicVertex.hlslの定義）
    // カメラ位置は CameraConstants.cameraPosition
    float3 V = normalize(cameraPosition - input.worldPos);

    // 方向光（仮）: 上から斜め（お好みで調整可）
    float3 L = normalize(float3(0.4, -1.0, 0.2));
    float3 H = normalize(V + L);

    // マテリアル
    float3 baseColor = albedo.rgb;
    if (hasAlbedoTexture > 0)
    {
        float2 uv = input.uv * uvTransform.xy + uvTransform.zw;
        baseColor *= albedoTexture.Sample(linearSampler, uv).rgb;
    }
    float metallic = saturate(albedo.a);
    float roughn = saturate(roughnessAoEmissiveStrength.x);
    float ao = saturate(roughnessAoEmissiveStrength.y);

    // F0
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), baseColor, metallic);

    // BRDF
    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));

    float3 F = FresnelSchlick(saturate(dot(H, V)), F0);
    float D = D_GGX(N, H, roughn);
    float G = G_Smith(N, V, L, roughn);

    float3 spec = (D * G * F) / max(4.0 * NdotV * NdotL, 1e-6);
    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - metallic);
    float3 diff = kD * baseColor / PI;

    float3 lightColor = float3(1.0, 0.97, 0.94); // 少し暖色
    float lightInt = 3.0;
    float3 direct = (diff + spec) * lightColor * lightInt * NdotL;

    float3 ambient = baseColor * 0.03 * ao; // IBL導入前の簡易アンビエント

    float3 color = direct + ambient + emissive.rgb * roughnessAoEmissiveStrength.z;

    // トーンマッピング → sRGB
    color = ACES(color);
    color = toSRGB(color);

    // アルファ（アルファテスト省略／必要ならBasicPixelのロジックを流用）
    return float4(color, alphaParams.x);
}
