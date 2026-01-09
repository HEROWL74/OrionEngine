// engine-assets/shaders/GizmoVS.hlsl

cbuffer GizmoConstants : register(b0)
{
    float4x4 worldViewProjection;
    float4 selectedColor;
    float scale;
    float3 padding;
};

struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput main(VSInput input)
{
    PSInput output;
    
    // スケールを適用
    float3 scaledPos = input.position * scale;
    
    // ワールド・ビュー・プロジェクション変換
    output.position = mul(float4(scaledPos, 1.0f), worldViewProjection);
    
    // カラーをそのまま渡す
    output.color = input.color;
    
    return output;
}