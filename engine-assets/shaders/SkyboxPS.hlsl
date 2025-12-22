// engine-assets/shaders/SkyboxVS.hlsl
TextureCube skyTex : register(t0);
SamplerState samplerState : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float3 texCoord : TEXCOORD;
};

float4 main(PSInput input) : SV_TARGET
{
    return skyTex.Sample(samplerState, input.texCoord);
}