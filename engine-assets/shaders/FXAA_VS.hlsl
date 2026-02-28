// engine-assets/shaders/FXAA_VS.hlsl
struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output;

    float2 pos;

    if (vertexID == 0)
        pos = float2(-1.0, -1.0);
    if (vertexID == 1)
        pos = float2(-1.0, 3.0);
    if (vertexID == 2)
        pos = float2(3.0, -1.0);

    output.position = float4(pos, 0.0, 1.0);

    // UV
    output.texCoord = float2(
        (pos.x + 1.0) * 0.5,
        1.0 - (pos.y + 1.0) * 0.5
    );

    return output;
}

