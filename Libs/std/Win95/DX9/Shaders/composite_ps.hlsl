// composite_ps.hlsl - Sample composite shader for multi-pass rendering
#include "common.hlsli"

// Input textures from previous passes
sampler2D g_albedoTexture :   register(s0); // Base color/texture pass
sampler2D g_lightingTexture : register(s1); // Lighting pass  
sampler2D g_effectsTexture :  register(s2); // Effects pass

struct PS_INPUT
{
    float4 position : POSITION;
    float4 color : COLOR0;
    float4 specular : COLOR1;
    float2 texCoord : TEXCOORD0;
};

float4 main(PS_INPUT input) : COLOR
{
    // Sample from each render target
    float4 albedo  = tex2D(g_albedoTexture, input.texCoord);
    //float4 lighting = tex2D(g_lightingTexture, input.texCoord);
    float4 effects  = tex2D(g_effectsTexture, input.texCoord);
    
    // Simple additive compositing
    float4 finalColor = albedo /* * lighting */ + effects;
    
    // Apply fog if enabled
    //finalColor = ApplyFog(finalColor, input.position.z);
    
    return finalColor;
}