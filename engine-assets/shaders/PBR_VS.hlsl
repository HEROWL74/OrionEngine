//assets/shaders/PBR_VS.hlsl
//PBR VertexShader

#include "Common.hlsli"

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    
    //Transform WorldPos
    float4 worldPos = mul(float4(input.position, 1.0), worldMatrix);
    output.worldPos = worldPos.xyz;
    
    //Transform ViewPos
    float4 viewPos = mul(worldPos, viewMatrix);
    output.position = mul(viewPos, projMatrix);
    
    //Normal To WorldPos
    output.normal = normalize(mul(input.normal, (float3x3) normalMatrix));
    
    //TangentToWorldPos
    output.tangent = normalize(mul(input.tangent, (float3x3) normalMatrix));
    
    output.bitangent = cross(output.normal, output.tangent);
    
    //UV
    output.uv = input.uv;
    
    return output;
}