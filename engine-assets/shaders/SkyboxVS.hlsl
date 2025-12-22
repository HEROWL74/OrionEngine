// engine-assets/shaders/SkyboxVS.hlsl
cbuffer CameraCB : register(b0)
{
    float4x4 viewNoTrans;
    float4x4 proj;
}

struct VSInput
{
    float3 position : POSITION;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 texCoord : TEXCOORD;
};

PSInput main(VSInput input)
{
    PSInput output;
    float4 pos = mul(float4(input.position, 1.0f), viewNoTrans);
    pos = mul(pos, proj);
    
    output.position = pos.xyww;
    
    output.texCoord = input.position;
    
    return output;
}