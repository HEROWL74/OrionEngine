// UITextPS.hlsl - 3D World Space用
Texture2D fontTexture : register(t0);
SamplerState fontSampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
};

float4 main(PSInput input) : SV_TARGET
{
    // フォントテクスチャからアルファ値をサンプリング
    float alpha = fontTexture.Sample(fontSampler, input.texCoord).r;
    
    // テキストカラーとアルファを適用
    float4 finalColor = input.color;
    finalColor.a *= alpha;
    
    // アルファが0なら破棄（透明部分）
    if (finalColor.a < 0.01f)
        discard;
    
    return finalColor;
}

