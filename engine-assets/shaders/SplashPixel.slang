// SplashPixel.slang
// Splash Screen Pixel Shader - Logo fade in/out

Texture2D logoTexture : register(t0);
SamplerState logoSampler : register(s0);

cbuffer SplashConstants : register(b0)
{
    float fadeAlpha;    // 0.0 to 1.0
    float logoScale;    // Scale factor
    float screenAspect; // Screen aspect ratio
    float splashPad;    // padding (float1でfloat4境界を合わせる)
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

[shader("fragment")]
float4 main(PSInput input) : SV_TARGET
{
    float3 backgroundColor = float3(0.0f, 0.0f, 0.0f);

    // Center UV & apply aspect / scale
    float2 centeredUV = input.texcoord - 0.5f;
    centeredUV.x *= screenAspect;
    centeredUV /= logoScale;
    centeredUV += 0.5f;

    float4 logoColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    if (centeredUV.x >= 0.0f && centeredUV.x <= 1.0f &&
        centeredUV.y >= 0.0f && centeredUV.y <= 1.0f)
    {
        logoColor = logoTexture.Sample(logoSampler, centeredUV);
    }

    float3 finalColor = lerp(backgroundColor, logoColor.rgb, logoColor.a);

    return float4(finalColor, fadeAlpha);
}
