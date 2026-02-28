struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 worldPos : WORLD_POSITION;
    float3 color : COLOR;
    float2 uv : TEXCOORD0;
};

cbuffer MaterialConstants : register(b2)
{
    float4 albedo; // xyz: albedo, w: metallic
    float4 roughnessAoEmissiveStrength; // x: roughness, y: ao, z: emissiveStrength, w: padding
    float4 emissive; // xyz: emissive, w: normalStrength
    float4 alphaParams; // x: alpha, y: useAlphaTest, z: alphaTestThreshold, w: heightScale
    float4 uvTransform; // xy: uvScale, zw: uvOffset
    int hasAlbedoTexture;
    int pad[3];
};

Texture2D albedoTexture : register(t0);
SamplerState linearSampler : register(s0);

float4 main(VertexOutput input) : SV_TARGET
{
    float3 baseColor;

    if (hasAlbedoTexture > 0)
    {
        float3 texColor = albedoTexture.Sample(linearSampler, input.uv).rgb;
        baseColor = texColor * albedo.rgb;
    }
    else
    {
        baseColor = albedo.rgb;
    }

    return float4(baseColor, alphaParams.x);
}