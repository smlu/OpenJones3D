#include "common.hlsli"

struct VS_INPUT 
{
    float4 position : POSITION;
    float4 diffuse : COLOR0;
    float4 specular : COLOR1;
    float2 texCoord : TEXCOORD0;
};

struct VS_OUTPUT 
    {
    float4 position : POSITION;
    float4 diffuse : COLOR0;
    float4 specular : COLOR1;
    float2 texCoord : TEXCOORD0;
    float rhw : TEXCOORD1;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = ScreenToClip(input.position);
    output.diffuse  = input.diffuse;
    output.specular = input.specular;
    output.texCoord = input.texCoord;
    output.rhw      = output.position.w;
    
    return output;
};