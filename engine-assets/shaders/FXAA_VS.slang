// engine-assets/shaders/FXAA_VS.slang
struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};
[shader("vertex")]
VSOutput main(uint vertexID: SV_VertexID)
{
    static const float2 positions[3] =
        {
            float2(-1.0, -1.0),
            float2(-1.0, 3.0),
            float2(3.0, -1.0),
        };

    VSOutput output;
    float2 pos = positions[vertexID];

    output.position = float4(pos, 0.0, 1.0);
    output.texCoord = float2(
        (pos.x + 1.0) * 0.5,
        1.0 - (pos.y + 1.0) * 0.5);
    return output;
}

