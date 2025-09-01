#include "common.hlsli"
struct PS_INPUT
{
    float4 position : POSITION0;
    float2 texCoord : TEXCOORD0;
    float4 screenPos : TEXCOORD1; // For sampling the scene texture
    float waveFactor : TEXCOORD2; // For distortion strength and alpha
};

sampler2D g_sceneTexture : register(s0); // Scene rendered before this effect

float4 main(PS_INPUT input) : COLOR0
{
    float4 finalColor = float4(1.0, 1.0, 1.0, 1.0);
    float3 earthquakeColor = lerp(finalColor.rgb,
                                 finalColor.rgb * float3(1.1, 0.95, 0.9),
                                 input.waveFactor * 0.3);
    finalColor.rgb = earthquakeColor;
    return finalColor;
}