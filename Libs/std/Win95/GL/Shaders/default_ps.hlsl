#include "common.hlsli"

struct PS_INPUT 
{
    float4 diffuse : COLOR0;
    float4 specular : COLOR1;
    float2 texCoord : TEXCOORD0;
    float rhw : TEXCOORD1;
};

sampler2D g_texture : register(s0);

float4 main(PS_INPUT input) : COLOR 
{
    // Use texture coordinates directly - they should already be perspective correct
    // from the original TL vertex calculation
    float4 texColor   = tex2D(g_texture, input.texCoord.xy);

    // TODO: In case of texture being 0 nothing will be rendered!
    float4 finalColor = texColor * input.diffuse;
    finalColor = ApplyFog(finalColor, input.rhw);
    return finalColor;
};