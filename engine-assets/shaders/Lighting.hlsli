//assets/shaders/Lighting.hlsli
//PBRLightingFunc

#include "Common.hlsli"

//MathConstant
static const float PI = 3.14159265359;
static const float EPSILON = 1e-6f;

//Frenel
float3 fresnelSchlick(float3 cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

//GGX/Trowbridge-Retiz
float distributtonGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return a2 / denom;
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float denom = NdotV * (1.0 - k) + k;
    
    return NdotV / denom;
}

float geometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

//Cook-Torrance BRDF
float3 cookTorranceBRDF(float3 N, float3 V, float3 L, float3 albedo, float metallic, float roughness)
{
    float3 H = normalize(V + L);
    
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
    
    
    float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    float D = distributtonGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    
    //Cook-Torrance BRDF
    float3 numrator = D * G * F;
    float denomirator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + EPSILON;
    float3 specular = numrator / denomirator;
    
    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - metallic;
    
    float3 diffuse = kD * albedo / PI;
    
    float NdoL = max(dot(N, L), 0.0);
    return (diffuse + specular) * NdoL;

}

float3 calculatePBRLighting(float3 worldPos, float3 normal, float3 albedo,
                            float metallic, float roughness, float ao)
{
    float3 V = normalize(cameraPos - worldPos);
    float3 L = normalize(directionalLightDir);
    
    //PBR BRDF
    float3 color = cookTorranceBRDF(normal, V, L, albedo, metallic, roughness);
    color *= directionalLightColor * directionalLightIntensity;
    
    //Ambient
    float3 ambient = ambientColor * ambientInsensity * albedo * ao;
    
    return color + ambient;
}

//Reinhard
float3 reinhardToneMapping(float3 color)
{
    return color / (color + float3(1.0, 1.0, 1.0));
}

//ACES
float3 acesToneMapping(float3 color)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    
    return saturate((color * (a * color + b)) / (color * (c * color + d) + e));
}


