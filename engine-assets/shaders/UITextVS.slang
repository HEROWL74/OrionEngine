// engine-assets/shaders/UITextVS.slang
cbuffer TextConstants : register(b0)
{
    float4x4 world;
    float4x4 viewProjection;
    float4 color; // カラー
};

struct VSInput
{
    float3 position : POSITION; // ローカル座標（3D）
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
};
[shader("vertex")]
PSInput main(VSInput input)
{
    PSInput output;

    float4 localPos = float4(input.position, 1.0f);
    float4 worldPos = mul(world, localPos);
    output.position = mul(viewProjection, worldPos);

    output.texCoord = input.texCoord;
    output.color = input.color;

    return output;
}

