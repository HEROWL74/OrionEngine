// engine-assets/shaders/SplashPixel.hlsl
// Splash Screen Pixel Shader - Logo fade in/out

Texture2D logoTexture : register(t0);
SamplerState logoSampler : register(s0);

cbuffer SplashConstants : register(b0)
{
    float fadeAlpha;      // 0.0 to 1.0 for fade in/out
    float logoScale;      // Scale factor for logo
    float screenAspect; // logo Screen aspect
    float2 padding;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    // Background color (black)
    float3 backgroundColor = float3(0.0f, 0.0f, 0.0f);
    
    // Calculate centered UV coordinates for logo
    float2 centeredUV = input.texcoord - 0.5f;
    centeredUV.x *= screenAspect;
    
    // Apply logo scaling
    centeredUV /= logoScale;
    centeredUV += 0.5f;
    
    // Sample logo texture
    float4 logoColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    if (centeredUV.x >= 0.0f && centeredUV.x <= 1.0f &&
        centeredUV.y >= 0.0f && centeredUV.y <= 1.0f)
    {
        logoColor = logoTexture.Sample(logoSampler, centeredUV);
    }
    
    // Blend logo over background
    float3 finalColor = lerp(backgroundColor, logoColor.rgb, logoColor.a);
    
    // Apply fade
    float finalAlpha = fadeAlpha;
    
    return float4(finalColor, finalAlpha);
}


