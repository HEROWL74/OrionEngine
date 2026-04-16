cbuffer CameraConstants : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 viewProjectionMatrix;
    float3 cameraPosition;
    float padding;
};

cbuffer ObjectConstants : register(b1)
{
    float4x4 worldMatrix;
    float4x4 worldViewProjectionMatrix;
    float3 objectPosition;
    float padding2;
};

struct VertexInput
{
    float3 position : POSITION;
    float3 color : COLOR;
    float2 uv : TEXCOORD;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 worldPos : WORLD_POSITION;
    float3 color : COLOR;
    float2 uv : TEXCOORD;
};

[shader("vertex")]
VertexOutput main(VertexInput input)
{
    VertexOutput output;

    float4 worldPos = mul(worldMatrix, float4(input.position, 1.0f));
    output.position = mul(viewProjectionMatrix, worldPos);

    output.color = input.color;
    output.uv = input.uv;

    return output;
}

