// assets/shaders/PBR_PS.hlsl
// PBRピクセルシェーダー

#include "Common.hlsli"
#include "Lighting.hlsli"

float4 main(VertexOutput input) : SV_TARGET
{
    //TransformUV
    float2 uv = transformUV(input.uv);
    
// AlbedoSampling
    float3 baseColor;
    if (hasAlbedoTexture > 0.5f)
    {
        float3 albedoSample = albedoTexture.Sample(linearSampler, uv).rgb;
        baseColor = albedo.rgb * albedoSample;
    }
    else
    {
        baseColor = albedo.rgb; //No texture 
    }
    
    //Transform NormalMap
    float3 normalMap = normalTexture.Sample(linearSampler, uv).rgb;
    float3 normal = transformNormal(normalMap, input.tangent, input.bitangent, input.normal);
    
    // MetallicMS
    float metallicSample = metallicTexture.Sample(linearSampler, uv).r;
    float metallic = albedo.a * metallicSample;
    
    // RoughnessMS
    float roughnessSample = roughnessTexture.Sample(linearSampler, uv).r;
    float roughness = roughnessAoEmissiveStrength.x * roughnessSample;
    
    // AOMS
    float aoSample = aoTexture.Sample(linearSampler, uv).r;
    float ao = roughnessAoEmissiveStrength.y * aoSample;
    
    // EmissiveMS
    float3 emissiveSample = emissiveTexture.Sample(linearSampler, uv).rgb;
    float3 emission = emissive.rgb * emissiveSample * roughnessAoEmissiveStrength.z;
    

    float3 finalColor = calculatePBRLighting(input.worldPos, normal, baseColor, metallic, roughness, ao);
    

    finalColor += emission;
    

    finalColor = acesToneMapping(finalColor);
    

    finalColor = toSRGB(finalColor);

    float alpha = alphaParams.x;
    if (alphaParams.y > 0.5) // useAlphaTest
    {
        if (alpha < alphaParams.z) // alphaTestThreshold
        {
            discard;
        }
    }
    
    return float4(finalColor, alpha);
}