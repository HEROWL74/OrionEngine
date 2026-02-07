// engine-assets/shaders/FXAA_PS.hlsl
Texture2D sourceTexture : register(t0);
SamplerState sourceSampler : register(s0);

cbuffer FXAAConstants : register(b0)
{
    float2 rcpFrame;
    float fxaaQualitySubpix;
    float fxaaQualityEdgeThreshold;
}

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

float rgb2luma(float3 rgb)
{
    return dot(rgb, float3(0.299, 0.587, 0.114));
}

float4 main(PSInput input) : SV_TARGET
{
    float2 uv = input.texCoord;
    
    // 中心とその周辺のルミナンスを取得
    float3 rgbM = sourceTexture.Sample(sourceSampler, uv).rgb;
    float lumaM = rgb2luma(rgbM);
    
    float lumaS = rgb2luma(sourceTexture.Sample(sourceSampler, uv + float2(0, rcpFrame.y)).rgb);
    float lumaE = rgb2luma(sourceTexture.Sample(sourceSampler, uv + float2(rcpFrame.x, 0)).rgb);
    float lumaN = rgb2luma(sourceTexture.Sample(sourceSampler, uv + float2(0, -rcpFrame.y)).rgb);
    float lumaW = rgb2luma(sourceTexture.Sample(sourceSampler, uv + float2(rcpFrame.x, 0)).rgb);
    
    // エッジ検出
    float lumaMin = min(lumaM, min(min(lumaN, lumaS), min(lumaE, lumaW)));
    float lumaMax = max(lumaM, max(max(lumaN, lumaS), max(lumaE, lumaW)));
    float lumaRange = lumaMax - lumaMin;
    
    // エッジが弱い場合は早期リターン
    if (lumaRange < max(fxaaQualityEdgeThreshold, lumaMax * 0.125))
    {
        return float4(rgbM, 1.0);
    }
    
    // 対角線のルミナンス
    float lumaNW = rgb2luma(sourceTexture.Sample(sourceSampler, uv + float2(-rcpFrame.x, -rcpFrame.y)).rgb);
    float lumaNE = rgb2luma(sourceTexture.Sample(sourceSampler, uv + float2(rcpFrame.x, -rcpFrame.y)).rgb);
    float lumaSW = rgb2luma(sourceTexture.Sample(sourceSampler, uv + float2(-rcpFrame.x, rcpFrame.y)).rgb);
    float lumaSE = rgb2luma(sourceTexture.Sample(sourceSampler, uv + float2(rcpFrame.x, rcpFrame.y)).rgb);
    
    // エッジ方向を決定
    float lumaNS = lumaN + lumaS;
    float lumaWE = lumaW + lumaE;
    float lumaNESE = lumaNE + lumaSE;
    float lumaNWSW = lumaNW + lumaSW;
    
    float edgeHorz = abs(lumaNW + lumaNE - 2.0 * lumaN) +
                     2.0 * abs(lumaW + lumaE - 2.0 * lumaM) +
                     abs(lumaSW + lumaSE - 2.0 * lumaS);
    
    float edgeVert = abs(lumaNW + lumaSW - 2.0 * lumaW) +
                     2.0 * abs(lumaN + lumaS - 2.0 * lumaM) +
                     abs(lumaNE + lumaSE - 2.0 * lumaE);
    
    bool isHorizontal = edgeHorz >= edgeVert;
    
    // ブレンド係数を計算
    float lengthSign = isHorizontal ? rcpFrame.y : rcpFrame.x;
    float subpixA = 2.0 * (lumaNS + lumaWE) + lumaNESE + lumaNWSW;
    float subpixB = (1.0 / 12.0) * subpixA;
    
    float gradientScaled = max(abs(isHorizontal ? (lumaN - lumaS) : (lumaW - lumaE)),
                               abs(isHorizontal ? (lumaM - lumaN) : (lumaM - lumaW))) * 0.25;
    
    float blendL = max(0.0, (subpixB - gradientScaled) / lumaRange);
    blendL = min(fxaaQualitySubpix, blendL);
    
    // エッジに沿ってサンプリング
    float2 offset = isHorizontal ? float2(0, lengthSign * blendL) : float2(lengthSign * blendL, 0);
    float3 result = sourceTexture.Sample(sourceSampler, uv + offset).rgb;
    
    return float4(result, 1.0);

}