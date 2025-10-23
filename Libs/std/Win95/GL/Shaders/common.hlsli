#ifndef J3D_COMMON_HLSL
#define J3D_COMMON_HLSL

#define MERGE_TOKENS(a, b) a##b

// ===== VERTEX SHADER CONSTANTS (c24-c255) =====
#define VS_VIEWPORT_REGISTER c0

#define VS_CONSTANTS_START_REGISTER 24  // Start after common matrices/lights

#define VS_CONSTANT_REGISTER(RelativeIndex) MERGE_TOKENS(c, VS_CONSTANTS_START_##RelativeIndex)

// VS constant register definitions (232 total application registers)
#define VS_CONSTANTS_START_0   24
#define VS_CONSTANTS_START_1   25
#define VS_CONSTANTS_START_2   26
#define VS_CONSTANTS_START_3   27
#define VS_CONSTANTS_START_4   28
#define VS_CONSTANTS_START_5   29
#define VS_CONSTANTS_START_6   30
#define VS_CONSTANTS_START_7   31
#define VS_CONSTANTS_START_8   32
#define VS_CONSTANTS_START_9   33
#define VS_CONSTANTS_START_10  34
#define VS_CONSTANTS_START_11  35
#define VS_CONSTANTS_START_12  36
#define VS_CONSTANTS_START_13  37
#define VS_CONSTANTS_START_14  38
#define VS_CONSTANTS_START_15  39
#define VS_CONSTANTS_START_16  40
#define VS_CONSTANTS_START_17  41
#define VS_CONSTANTS_START_18  42
#define VS_CONSTANTS_START_19  43
#define VS_CONSTANTS_START_20  44
#define VS_CONSTANTS_START_21  45
#define VS_CONSTANTS_START_22  46
#define VS_CONSTANTS_START_23  47
#define VS_CONSTANTS_START_24  48
#define VS_CONSTANTS_START_25  49
#define VS_CONSTANTS_START_26  50
#define VS_CONSTANTS_START_27  51
#define VS_CONSTANTS_START_28  52
#define VS_CONSTANTS_START_29  53
#define VS_CONSTANTS_START_30  54
#define VS_CONSTANTS_START_31  55


// ===== PIXEL SHADER CONSTANTS (c8-c223) =====
#define PS_FOGPARAMS_REGISTER c0
#define PS_FOGCOLOR_REGISTER  c1

#define PS_CONSTANTS_START_REGISTER 8   // Start after common material/light constants

#define PS_CONSTANT_REGISTER(RelativeIndex) MERGE_TOKENS(c, PS_CONSTANTS_START_##RelativeIndex)

// PS constant register definitions (216 total application registers)
#define PS_CONSTANTS_START_0   8
#define PS_CONSTANTS_START_1   9
#define PS_CONSTANTS_START_2   10
#define PS_CONSTANTS_START_3   11
#define PS_CONSTANTS_START_4   12
#define PS_CONSTANTS_START_5   13
#define PS_CONSTANTS_START_6   14
#define PS_CONSTANTS_START_7   15
#define PS_CONSTANTS_START_8   16
#define PS_CONSTANTS_START_9   17
#define PS_CONSTANTS_START_10  18
#define PS_CONSTANTS_START_11  19
#define PS_CONSTANTS_START_12  20
#define PS_CONSTANTS_START_13  21
#define PS_CONSTANTS_START_14  22
#define PS_CONSTANTS_START_15  23
#define PS_CONSTANTS_START_16  24
#define PS_CONSTANTS_START_17  25
#define PS_CONSTANTS_START_18  26
#define PS_CONSTANTS_START_19  27
#define PS_CONSTANTS_START_20  28
#define PS_CONSTANTS_START_21  29
#define PS_CONSTANTS_START_22  30
#define PS_CONSTANTS_START_23  31

// ===== VERTEX SHADER CONSTANTS =====
// Global constant registers - RESERVED, do not use in custom shaders!
float4 g_viewport : register(VS_VIEWPORT_REGISTER); // x1, y1, x2, y2

// ===== VERTEX PIXEL CONSTANTS =====
float4 g_fogParams : register(PS_FOGPARAMS_REGISTER); // start, end, factor, enabled
float4 g_fogColor : register(PS_FOGCOLOR_REGISTER); // r, g, b, a

// ===== HELPER FUNCTIONS =====

// Convert screen space position to clip space
float4 ScreenToClip(float4 screenPos)
{
    float width  = g_viewport.z - g_viewport.x;
    float height = g_viewport.w - g_viewport.y;
    
     // Convert screen -> NDC in D3D9 (note the Y flip)
    float ndc_x  = ((screenPos.x - g_viewport.x) / width) * 2.0 - 1.0;
    float ndc_y  = (1.0 - (screenPos.y - g_viewport.y) / height) * 2.0 - 1.0; 
    float clip_w = 1.0 / screenPos.w; // rhw = 1/w, so clip_w = 1/rhw
    
    // Recreate clip-space
    float4 clip;
    clip.x = ndc_x * clip_w;
    clip.y = ndc_y * clip_w;
    clip.z = screenPos.z * clip_w; // D3D z_ndc is [0..1], so clip_z = z_ndc * w
    clip.w = clip_w;

    return clip;
}

// Apply fog to a color based on depth
float4 ApplyFog(float4 color, float depth) // depth should be vertex rhw in screen space
{
    // Apply linear fog if enabled
    if (g_fogParams.w > 0.0f) // Note, branching might impact performance
    {
       // Calculate linear fog factor
       // fogFactor = (rhw - fogStart) / (fogEnd - fogStart)
       // Using pre-calculated 1/(end-start) in g_fogParams.z for efficiency
        float fogFactor = (depth - g_fogParams.x) * g_fogParams.z;
        fogFactor = saturate(fogFactor); // Clamp fog factor to [0, 1] range

       // Apply fog
        color.rgb = lerp(color.rgb, g_fogColor.rgb, fogFactor);
    }
    
    return color;
}

// Convert world position to screen space
float4 WorldToScreen(float4 worldPos, float4x4 viewProjMatrix)
{
    float4 clipPos = mul(worldPos, viewProjMatrix);
    
    float4 screenPos;
    screenPos.x = (clipPos.x / clipPos.w + 1.0) * 0.5 * (g_viewport.z - g_viewport.x) + g_viewport.x;
    screenPos.y = (1.0 - clipPos.y / clipPos.w) * 0.5 * (g_viewport.w - g_viewport.y) + g_viewport.y;
    screenPos.z = clipPos.z / clipPos.w;
    screenPos.w = 1.0 / clipPos.w; // rhw

    return screenPos;
}

#endif // J3D_COMMON_HLSL