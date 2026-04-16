// engine-assets/shaders/SplashVertex.clang
// Splash Screen Vertex Shader - Fullscreen Quad

struct VSInput
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};
[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output;

    // Pass through position (already in NDC space)
    output.position = float4(input.position, 1.0f);
    output.texcoord = input.texcoord;

    return output;
}

