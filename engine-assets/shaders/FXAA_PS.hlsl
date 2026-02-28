Texture2D sourceTexture : register(t0);
SamplerState sourceSampler : register(s0);

cbuffer FXAAConstants : register(b0)
{
    float2 rcpFrame; // 1 / screen size
    float fxaaQualitySubpix; // 0.0 - 1.0
    float fxaaQualityEdgeThreshold; // 0.125
    float fxaaQualityEdgeThresholdMin; // 0.0312
};

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

    float3 rgbM = sourceTexture.Sample(sourceSampler, uv).rgb;

    float lumaM = rgb2luma(rgbM);
    float lumaN = rgb2luma(sourceTexture.Sample(sourceSampler, uv + float2(0, -rcpFrame.y)).rgb);
    float lumaS = rgb2luma(sourceTexture.Sample(sourceSampler, uv + float2(0, rcpFrame.y)).rgb);
    float lumaE = rgb2luma(sourceTexture.Sample(sourceSampler, uv + float2(rcpFrame.x, 0)).rgb);
    float lumaW = rgb2luma(sourceTexture.Sample(sourceSampler, uv + float2(-rcpFrame.x, 0)).rgb);

    float lumaMin = min(lumaM, min(min(lumaN, lumaS), min(lumaE, lumaW)));
    float lumaMax = max(lumaM, max(max(lumaN, lumaS), max(lumaE, lumaW)));
    float lumaRange = lumaMax - lumaMin;

    //  エッジ検出（Min制御付き）
    if (lumaRange < max(fxaaQualityEdgeThresholdMin, lumaMax * fxaaQualityEdgeThreshold))
    {
        return float4(rgbM, 1.0);
    }

    // --- 対角サンプル ---
    float lumaNW = rgb2luma(sourceTexture.Sample(sourceSampler, uv + float2(-rcpFrame.x, -rcpFrame.y)).rgb);
    float lumaNE = rgb2luma(sourceTexture.Sample(sourceSampler, uv + float2(rcpFrame.x, -rcpFrame.y)).rgb);
    float lumaSW = rgb2luma(sourceTexture.Sample(sourceSampler, uv + float2(-rcpFrame.x, rcpFrame.y)).rgb);
    float lumaSE = rgb2luma(sourceTexture.Sample(sourceSampler, uv + float2(rcpFrame.x, rcpFrame.y)).rgb);

    // --- エッジ方向計算 ---
    float edgeHorz =
        abs(lumaNW + lumaNE - 2.0 * lumaN) +
        abs(lumaSW + lumaSE - 2.0 * lumaS) +
        2.0 * abs(lumaW + lumaE - 2.0 * lumaM);

    float edgeVert =
        abs(lumaNW + lumaSW - 2.0 * lumaW) +
        abs(lumaNE + lumaSE - 2.0 * lumaE) +
        2.0 * abs(lumaN + lumaS - 2.0 * lumaM);

    bool isHorizontal = edgeHorz >= edgeVert;

    float2 stepDir = isHorizontal ? float2(rcpFrame.x, 0) : float2(0, rcpFrame.y);

    // --- エッジ探索（簡易2tap） ---
    float lumaNeg = rgb2luma(sourceTexture.Sample(sourceSampler, uv - stepDir).rgb);
    float lumaPos = rgb2luma(sourceTexture.Sample(sourceSampler, uv + stepDir).rgb);

    float gradientNeg = abs(lumaNeg - lumaM);
    float gradientPos = abs(lumaPos - lumaM);

    float gradient = max(gradientNeg, gradientPos);

    float blend = saturate((gradient / lumaRange) * fxaaQualitySubpix);

    // --- 最終ブレンド ---
    float2 offset = stepDir * blend * 0.5;

    float3 rgbA = sourceTexture.Sample(sourceSampler, uv + offset).rgb;
    float3 rgbB = sourceTexture.Sample(sourceSampler, uv - offset).rgb;

    float3 finalColor = (rgbA + rgbB) * 0.5;

    return float4(finalColor, 1.0);
}
