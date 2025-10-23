#include "common.hlsli"

struct PS_INPUT 
{
    float4 diffuse : COLOR0;
};

float4 g_wireframeColor : register(PS_CONSTANT_REGISTER(0));
float4 main(PS_INPUT input) : COLOR 
{
    return g_wireframeColor * input.diffuse;
};