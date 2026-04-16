// engine-assets/shaders/GridVS.clang
cbuffer CameraConstants : register(b0)
{
    float4x4 viewProjection;
}

struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};
[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output;
    output.position = mul(viewProjection, float4(input.position, 1.0f));
    output.color = input.color;
    return output;
}

