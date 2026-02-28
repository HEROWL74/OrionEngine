// engine-assets/shaders/UITextVS.hlsl - 3D空間対応版（修正）
cbuffer TextConstants : register(b0)
{
    float4x4 world; 
    float4x4 viewProjection;
    float4 color; // カラー
    uint4 padding; // アライメント用
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

PSInput main(VSInput input)
{
    PSInput output;

    float4 localPos = float4(input.position, 1.0f);
    float4 worldPos = mul(localPos, world);

    output.position = mul(worldPos, viewProjection);
    
    output.texCoord = input.texCoord;
    output.color = input.color;
    
    return output;
}