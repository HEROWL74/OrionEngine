// engine-assets/shaders/SkyboxVS.clang
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
[shader("vertex")]
PSInput main(VSInput input)
{
    PSInput output;
    float4 pos = mul(viewNoTrans, float4(input.position, 1.0f));
    pos = mul(proj, pos);

    output.position = pos.xyww;

    output.texCoord = input.position;

    return output;
}

