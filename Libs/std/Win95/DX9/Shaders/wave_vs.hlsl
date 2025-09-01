#include "common.hlsli"

// Vertex Shader Input (from a pre-transformed TL vertex)
struct VS_INPUT
{
    float4 position : POSITION0; // sx, sy, sz
    float rhw : BLENDWEIGHT0;
    float4 color : COLOR0;
    float2 texCoord : TEXCOORD0;
};

// Data passed from Vertex to Pixel Shader
struct VS_OUTPUT
{
    float4 position : POSITION0;
    float2 texCoord : TEXCOORD0;
    float4 screenPos : TEXCOORD1; // For sampling the scene texture
    float  waveFactor : TEXCOORD2; // For distortion strength and alpha
};

float3 g_waveCenter = float3(-1.24180996, 0.90427703, 4.60324001); // Center of the wave in world space, set to position in sector 212 in canyon level 
float waveStrength = 0.6;

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;

    // 1. Convert the input screen-space vertex back to world space
    float3 worldPos = ScreenToWorld(input.position);

    // 2. Apply the wave effect in world space
    // This is your custom function that displaces the vertex.
    // For this example, we'll simulate a simple spherical expansion.
    float dist = distance(worldPos, g_waveCenter);
    float waveRadius = 1.0; //g_time * g_waveParams.z;
    float falloff = 1.0 - saturate(abs(dist - waveRadius) * 5.0);
    float3 displacement = normalize(worldPos - g_waveCenter) * falloff * waveStrength; //g_waveParams.x;
    float3 displacedWorldPos = worldPos + displacement;
    
    // 3. Project the newly displaced world position back to screen space
    float4 newScreenPos = WorldToScreen(displacedWorldPos);

    // 4. Convert the final screen position to clip space for the hardware rasterizer
    output.position = ScreenToClip(newScreenPos);
    
    // 5. Pass necessary data to the pixel shader
    output.texCoord   = input.texCoord;
    output.screenPos  = output.position; // Pass the final clip-space position
    output.waveFactor = falloff; // Pass the calculated wave intensity

    return output;
}

