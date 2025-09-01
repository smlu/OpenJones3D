#include "common.hlsli"

//struct VS_INPUT
//{
//    float4 position : POSITION0;
//    float4 diffuse : COLOR0;
//    float4 specular : COLOR1;
//    float2 texCoord : TEXCOORD0;
//    float3 positionWS : POSITION1;
//    float3 normalWS : NORMAL0;
//};

//struct VS_OUTPUT
//{
//    float4 position : POSITION;
//    float4 diffuse : COLOR0;
//    float4 specular : COLOR1;
//    float2 texCoord : TEXCOORD0;
//    float rhw : TEXCOORD1;
//    float3 positionWS : TEXCOORD2;
//    float3 normalWS : NORMAL0;
//    float bNormal : COLOR3;
//};

//VS_OUTPUT main(VS_INPUT input)
//{
//    VS_OUTPUT output;
//    //output.position = ScreenToClip(WorldToScreen(ScreenToWorld(input.position)));
//    //output.position = ScreenToClip(mul(float4(ScreenToWorld(input.position), 1), g_viewProjMatrix));
//    //output.position = mul(float4(ScreenToWorld(input.position), 1), g_viewProjMatrix);
    
    
//    //float4 testVec = float4(0, 0, 0, 1);
//    //float4 result1 = mul(testVec, g_viewProjMatrix);
//    //float4 result2 = mul(result1, g_invViewProjMatrix);
    
//    //// result2 should be close to testVec if matrices are inverses
//    //output.diffuse = float4(abs(result2.xyz - testVec.xyz), 1);
//    output.position = ScreenToClip(input.position);
//    output.positionWS = input.positionWS;
//    //output.positionWS = ClipToWorld(input.position);

//    output.diffuse = input.diffuse;
//    output.specular = input.specular;
//    output.texCoord = input.texCoord;
//    output.rhw = output.position.w;
//    output.normalWS = input.normalWS;
//    output.bNormal = length(input.normalWS) > 0.001f ? 1.0 : 0.0;
    
//    return output;
//};


/// WAVE
//#include "common.hlsli"

//struct VS_INPUT
//{
//    float4 position : POSITION;
//    float4 diffuse : COLOR0;
//    float4 specular : COLOR1;
//    float2 texCoord : TEXCOORD0;
//};

//struct VS_OUTPUT
//{
//    float4 position : POSITION;
//    float4 diffuse : COLOR0;
//    float4 specular : COLOR1;
//    float2 texCoord : TEXCOORD0;
//    float rhw : TEXCOORD1;
//};

//VS_OUTPUT main(VS_INPUT input)
//{
//    VS_OUTPUT output;
    
//    // Wave effect constants (for testing - replace with registers later)
//    float3 waveCenter = float3(-1.24180996, 0.90427703, 4.60324001); // World position of wave center
//    float waveRadius = 1.0; // Maximum radius of effect
//    float waveAmplitude = 2.0; // Height/strength of the wave
//    float waveFrequency = 0.5; // Wave frequency (higher = more waves)
//    float time = 10.0; // Time value (animate this for moving waves)
//    float waveSpeed = 5.0; // Speed of wave propagation
//    float falloffPower = 2.0; // How quickly the effect fades with distance
    
//    // Start with original position
//    float3 worldPos = ScreenToWorld(input.position);
    
//    // Calculate distance from wave center (assuming input.position is in world space)
//    // If your position is in screen space, you'll need to transform it to world space first
//    float3 toVertex = worldPos.xyz - waveCenter;
//    float distanceFromCenter = length(toVertex.xy); // Using XZ for ground-based waves
    
//    // Only apply effect within wave radius
//    if (distanceFromCenter < waveRadius)
//    {
//        // Calculate wave phase based on distance and time
//        float wavePhase = (distanceFromCenter * waveFrequency) - (time * waveSpeed);
        
//        // Calculate wave intensity (fades out at edges)
//        float falloff = 1.0 - pow(distanceFromCenter / waveRadius, falloffPower);
//        falloff = max(0.0, falloff);
        
//        // Calculate wave displacement
//        float waveHeight = sin(wavePhase) * waveAmplitude * falloff;
        
//        // For shock wave effect, you might want a more sharp wave front:
//        // float waveHeight = sin(wavePhase) * exp(-abs(wavePhase) * 0.5) * waveAmplitude * falloff;
        
//        // Apply vertical displacement (assuming Y is up)
//        worldPos.y += waveHeight;
        
//        // Optional: Add some horizontal ripple effect
//        float horizontalWave = cos(wavePhase) * 0.3 * waveAmplitude * falloff;
//        if (distanceFromCenter > 0.01) // Avoid division by zero
//        {
//            float3 direction = normalize(toVertex);
//            worldPos.xz += direction.xz * horizontalWave;
//        }
//    }
    
//    // Transform the modified position to clip space
//    output.position = ScreenToClip(WorldToScreen(worldPos));
//    output.diffuse = input.diffuse;
//    output.specular = input.specular;
//    output.texCoord = input.texCoord;
//    output.rhw = output.position.w;
    
//    return output;
//}


// ORIGINAL
struct VS_INPUT
{
    float4 position : POSITION0;
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

    output.diffuse = input.diffuse;
    output.specular = input.specular;
    output.texCoord = input.texCoord;
    output.rhw = output.position.w;
    return output;
};