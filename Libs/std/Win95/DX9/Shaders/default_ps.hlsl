#include "common.hlsli"

struct PS_INPUT 
{
    float4 diffuse : COLOR0;
    float4 specular : COLOR1;
    float2 texCoord : TEXCOORD0;
    float4 rhw : TEXCOORD1; // Altered: float4 so Wine's shader compiler can't pack it into the same register as TEXCOORD0.
};

sampler2D g_texture : register(s0);

float4 main(PS_INPUT input) : COLOR 
{
    float4 texColor   = tex2D(g_texture, input.texCoord.xy);
    float4 finalColor = texColor * input.diffuse;
    finalColor = ApplyFog(finalColor, input.rhw.x);
    return finalColor;
};