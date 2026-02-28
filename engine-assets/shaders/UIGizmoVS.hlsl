cbuffer GizmoCB : register(b0)
{
    float4x4 wvp;
};

struct VSInput
{
    float3 pos : POSITION;
};

struct PSInput
{
    float4 pos : SV_POSITION;
};

PSInput main(VSInput input)
{
    PSInput o;
    o.pos = mul(float4(input.pos, 1.0), wvp);
    return o;
}


