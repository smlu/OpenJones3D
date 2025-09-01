#include "common.hlsli"




struct Light
{
    float3 position;
    float minRadius;
    float maxRadius;
    float3 color;
};





// Main function to compute lighting
float3 ComputeLighting(float3 vertexPos, float3 vertexNormal)
{
    Light fixedLight =
    {
        float3(-1.24180996f, 0.90427703f, 4.60324001f), // Replace with your fixed light position
        1.395146f, // Replace with your fixed minimum radius
        0.348787f, // Replace with your fixed maximum radius
        float3(1.0f, 0.0f, 0.0f) // Replace with your fixed light color
    };
    
    // Subtract the vectors to get the direction vector
    float3 vecDir = fixedLight.position - vertexPos;

    // Compute the length of the direction vector
    float dist = length(vecDir);

    // Initialize the color increment
    float3 colorIncrement = float3(0.4f, 0.4f, 0.4f);

    // Check if the distance is less than the minimum radius
    if (dist < fixedLight.minRadius)
    {
        // Normalize the direction vector
        float3 lightDir = normalize(vecDir);

        // Compute the dot product between the vertex normal and the direction vector
        float ldot = dot(vertexNormal, lightDir);

        // Check if the dot product is positive
        if (ldot > 0.0f)
        {
            // Check if the distance is less than the maximum radius
            if (dist < fixedLight.maxRadius)
            {
                // Add the light color, scaled by the dot product, to the color increment
                colorIncrement += fixedLight.color * ldot;
            }
            else
            {
                // Compute the distance range and attenuation factor
                float drad = fixedLight.minRadius - fixedLight.maxRadius;
                float atten = (dist - fixedLight.maxRadius) / drad;

                // Add the attenuated light color, scaled by the dot product, to the color increment
                colorIncrement += (fixedLight.color - atten * fixedLight.color) * ldot;
            }
        }
    }

    // Return the color increment
    return colorIncrement;
}


float3 ComputePositionBasedLighting2(float3 vertexPos)
{
    Light fixedLight =
    {
        float3(-1.24180996f, 0.90427703f, 4.60324001f),
        1.395146f, // minRadius - outer boundary (light fades to zero)
        0.348787f, // maxRadius - inner boundary (maximum intensity)
        float3(1.0f, 0.0f, 0.0f)
    };
    
    float3 lightVector = fixedLight.position - vertexPos;
    float dist = length(lightVector);
    
    // Base ambient lighting
    float3 colorIncrement = float3(0.1f, 0.1f, 0.1f);
    
    // Only compute if within light range (using your original logic)
    if (dist < fixedLight.minRadius)
    {
        // Use world position gradients instead of normals for smooth lighting
        float heightFactor = saturate((vertexPos.y + 5.0f) / 10.0f); // Adjust range as needed
        
        // You can also use distance from a plane or other position-based factors:
        // float planeFactor = saturate(abs(vertexPos.z - 2.0f) / 3.0f);
        // float sphereFactor = saturate(length(vertexPos - float3(0,0,0)) / 5.0f);
        
        // Replace the normal dot product with position-based factor
        if (heightFactor > 0.0f) // Instead of ldot > 0.0f
        {
            float attenuation = 1.0f;
            
            if (dist > fixedLight.maxRadius)
            {
                // Your original attenuation calculation
                float range = fixedLight.minRadius - fixedLight.maxRadius;
                attenuation = (fixedLight.minRadius - dist) / range;
                attenuation = saturate(attenuation);
            }
            // If dist <= maxRadius, attenuation stays at 1.0 (full intensity)
            
            // Use heightFactor instead of ldot for smooth position-based lighting
            colorIncrement += fixedLight.color * heightFactor * attenuation;
        }
    }
    
    return colorIncrement;
}



float3 ComputePositionBasedLighting3(float3 vertexPos)
{
    Light fixedLight =
    {
        float3(-1.24180996f, 0.90427703f, 4.60324001f),
        1.395146f, // minRadius
        0.348787f, // maxRadius  
        float3(1.0f, 0.0f, 0.0f)
    };
    
    float3 lightVector = fixedLight.position - vertexPos;
    float dist = length(lightVector);
    
    float3 colorIncrement = float3(0.1f, 0.1f, 0.1f);
    
    if (dist < fixedLight.minRadius)
    {
        // Choose one of these position-based factors:
        
        // Option 1: Height-based
        float positionFactor = saturate((vertexPos.y + 2.0f) / 4.0f);
        
        // Option 2: Distance from a reference point
        // float3 referencePoint = float3(0, 1, 2);
        // float positionFactor = 1.0f - saturate(length(vertexPos - referencePoint) / 3.0f);
        
        // Option 3: Combination of axes
        // float positionFactor = saturate((vertexPos.x + vertexPos.y + 10.0f) / 20.0f);
        
        // Option 4: Directional gradient
        // float3 gradientDir = normalize(float3(1, 1, 0));
        // float positionFactor = saturate(dot(normalize(vertexPos), gradientDir) * 0.5f + 0.5f);
        
        if (positionFactor > 0.0f)
        {
            float attenuation = 1.0f;
            
            if (dist > fixedLight.maxRadius)
            {
                // Your original attenuation system
                float range = fixedLight.minRadius - fixedLight.maxRadius;
                attenuation = (fixedLight.minRadius - dist) / range;
                attenuation = saturate(attenuation);
            }
            
            colorIncrement += fixedLight.color * positionFactor * attenuation;
        }
    }
    
    return colorIncrement;
}


float3 ComputeHybridLighting(float3 vertexPos, float3 vertexNormal)
{
    Light fixedLight =
    {
        float3(-1.24180996f, 0.90427703f, 4.60324001f),
        1.395146f, // minRadius
        0.348787f, // maxRadius  
        float3(1.0f, 0.0f, 0.0f)
    };
    
    float3 lightVector = fixedLight.position - vertexPos;
    float dist = length(lightVector);
    
    float3 colorIncrement = float3(0.1f, 0.1f, 0.1f);
    
    if (dist < fixedLight.minRadius)
    {
        float3 lightDir = normalize(lightVector);
        
        // Position-based factor for smooth base lighting
        float heightFactor = saturate((vertexPos.y + 2.0f) / 4.0f);
        
        // Normal factor but softened
        float NdotL = dot(vertexNormal, lightDir);
        float softNormal = saturate(NdotL * 0.7f + 0.3f); // Softer falloff
        
        // Combine both factors
        float combinedFactor = heightFactor * softNormal;
        
        if (combinedFactor > 0.0f)
        {
            float attenuation = 1.0f;
            
            if (dist > fixedLight.maxRadius)
            {
                float range = fixedLight.minRadius - fixedLight.maxRadius;
                attenuation = (fixedLight.minRadius - dist) / range;
                attenuation = saturate(attenuation);
            }
            
            colorIncrement += fixedLight.color * combinedFactor * attenuation;
        }
    }
    
    return colorIncrement;
}

float3 ComputeOcclusionLighting(float3 vertexPos, float3 vertexNormal)
{
    Light fixedLight =
    {
        float3(-1.24180996f, 0.90427703f, 4.60324001f),
        1.395146f, // minRadius
        0.348787f, // maxRadius  
        float3(1.0f, 0.0f, 0.0f)
    };
    
    float3 lightVector = fixedLight.position - vertexPos;
    float dist = length(lightVector);
    
    float3 colorIncrement = float3(0.6f, 0.6f, 0.6f);
    
    if (dist < fixedLight.minRadius)
    {
        float3 lightDir = normalize(lightVector);
        
        // Normal check - but only for basic occlusion (not intensity)
        float NdotL = dot(vertexNormal, lightDir);
        float normalOcclusion = step(0.1f, NdotL); // Sharp cutoff for back faces
        
        // Position-based intensity (smooth)
        float positionIntensity = saturate((vertexPos.y + 2.0f) / 4.0f);
        
        // Combine: position controls brightness, normal controls visibility
        float finalFactor = positionIntensity * normalOcclusion;
        
        if (finalFactor > 0.0f)
        {
            float attenuation = 1.0f;
            
            if (dist > fixedLight.maxRadius)
            {
                float range = fixedLight.minRadius - fixedLight.maxRadius;
                attenuation = (fixedLight.minRadius - dist) / range;
                attenuation = saturate(attenuation);
            }
            
            colorIncrement += fixedLight.color * finalFactor * attenuation;
        }
    }
    
    return colorIncrement;
}


float3 ComputeWeightedLighting(float3 vertexPos, float3 vertexNormal)
{
    Light fixedLight =
    {
        float3(-1.24180996f, 0.90427703f, 4.60324001f),
        1.395146f, // minRadius
        0.348787f, // maxRadius  
        float3(1.0f, 0.0f, 0.0f)
    };
    
    float3 lightVector = fixedLight.position - vertexPos;
    float dist = length(lightVector);
    
    float3 colorIncrement = float3(0.1f, 0.1f, 0.1f);
    
    if (dist < fixedLight.minRadius)
    {
        float3 lightDir = normalize(lightVector);
        
        // Position-based lighting (smooth)
        float positionFactor = saturate((vertexPos.y + 2.0f) / 4.0f);
        
        // Normal-based lighting (can show polygons)
        float NdotL = saturate(dot(vertexNormal, -lightDir));
        
        // Weight based on distance - closer = more normal influence
        float normalWeight = saturate(1.0f - (dist / fixedLight.maxRadius));
        float positionWeight = 1.0f - normalWeight;
        
        // Blend the two approaches
        float blendedFactor = lerp(positionFactor, NdotL, normalWeight * 0.5f);
        
        if (blendedFactor > 0.0f)
        {
            float attenuation = 1.0f;
            
            if (dist > fixedLight.maxRadius)
            {
                float range = fixedLight.minRadius - fixedLight.maxRadius;
                attenuation = (fixedLight.minRadius - dist) / range;
                attenuation = saturate(attenuation);
            }
            
            colorIncrement += fixedLight.color * blendedFactor * attenuation;
        }
    }
    
    return colorIncrement;
}

float3 ComputeMultiSampleLighting(float3 vertexPos, float3 vertexNormal)
{
    Light fixedLight =
    {
        float3(-1.24180996f, 0.90427703f, 4.60324001f),
        1.395146f, 0.348787f,
        float3(1.0f, 0.0f, 0.0f)
    };
    
    float3 lightVector = fixedLight.position - vertexPos;
    float dist = length(lightVector);
    
    float3 colorIncrement = float3(0.1f, 0.1f, 0.1f);
    
    if (dist < fixedLight.minRadius)
    {
        float3 lightDir = normalize(lightVector);
        
        // Create multiple samples around the point for smoother result
        float totalFactor = 0.0f;
        float sampleRadius = 0.1f;
        
        // Center sample
        totalFactor += saturate(dot(vertexNormal, lightDir)) * 0.5f;
        
        // Offset samples (simulating neighboring vertices)
        float3 offset1 = float3(sampleRadius, 0, 0);
        float3 offset2 = float3(0, sampleRadius, 0);
        float3 offset3 = float3(-sampleRadius, 0, 0);
        float3 offset4 = float3(0, -sampleRadius, 0);
        
        // Sample lighting at offset positions
        totalFactor += saturate(dot(vertexNormal, normalize((fixedLight.position - (vertexPos + offset1))))) * 0.125f;
        totalFactor += saturate(dot(vertexNormal, normalize((fixedLight.position - (vertexPos + offset2))))) * 0.125f;
        totalFactor += saturate(dot(vertexNormal, normalize((fixedLight.position - (vertexPos + offset3))))) * 0.125f;
        totalFactor += saturate(dot(vertexNormal, normalize((fixedLight.position - (vertexPos + offset4))))) * 0.125f;
        
        if (totalFactor > 0.0f)
        {
            float attenuation = 1.0f;
            if (dist > fixedLight.maxRadius)
            {
                float range = fixedLight.minRadius - fixedLight.maxRadius;
                attenuation = (fixedLight.minRadius - dist) / range;
                attenuation = saturate(attenuation);
            }
            
            colorIncrement += fixedLight.color * totalFactor * attenuation;
        }
    }
    
    return colorIncrement;
}


float3 ComputeSmoothedNormalLighting(float3 vertexPos, float3 vertexNormal)
{
    Light fixedLight =
    {
        float3(-1.24180996f, 0.90427703f, 4.60324001f),
        1.395146f, // minRadius
        0.348787f, // maxRadius  
        float3(1.0f, 0.0f, 0.0f)
    };
    
    float3 lightVector = fixedLight.position - vertexPos;
    float dist = length(lightVector);
    
    float3 colorIncrement = float3(0.1f, 0.1f, 0.1f);
    
    if (dist < fixedLight.minRadius)
    {
        float3 lightDir = normalize(lightVector);
        
        // Smooth normal influence using a curve
        float NdotL = dot(vertexNormal, -lightDir);
        float smoothNormal = saturate(NdotL);
        smoothNormal = pow(smoothNormal, 0.5f); // Softer curve
        smoothNormal = smoothNormal * 0.8f + 0.2f; // Add base level
        
        if (smoothNormal > 0.0f)
        {
            float attenuation = 1.0f;
            
            if (dist > fixedLight.maxRadius)
            {
                float range = fixedLight.minRadius - fixedLight.maxRadius;
                attenuation = (fixedLight.minRadius - dist) / range;
                attenuation = saturate(attenuation);
            }
            
            colorIncrement += fixedLight.color * smoothNormal * attenuation;
        }
    }
    
    return colorIncrement;
}

float3 Compute2DCircleLight(float3 vertexPos)
{
    Light fixedLight =
    {
        float3(-1.24180996f, 0.90427703f, 4.60324001f),
        1.395146f, // Light circle radius
        0.348787f, // Inner radius (full intensity)
        float3(1.0f, 0.0f, 0.0f)
    };
    
    // Project light to 2D plane (ignore one axis, e.g., Y for top-down circle)
    float2 lightPos2D = fixedLight.position.xz; // Top-down circle
    float2 pixelPos2D = vertexPos.xz;
    
    // Calculate 2D distance
    float dist2D = length(lightPos2D - pixelPos2D);
    
    float3 colorIncrement = float3(0.1f, 0.1f, 0.1f); // Ambient
    
    // Simple circle - inside = lit, outside = not lit
    if (dist2D < fixedLight.minRadius)
    {
        float intensity = 1.0f;
        
        // Optional: soft edge
        if (dist2D > fixedLight.maxRadius)
        {
            float edge = (fixedLight.minRadius - dist2D) / (fixedLight.minRadius - fixedLight.maxRadius);
            intensity = saturate(edge);
        }
        
        // Uniform lighting - no normal consideration at all!
        colorIncrement += fixedLight.color * intensity;
    }
    
    return colorIncrement;
}

float3 ComputePlaneProjectedLight(float3 vertexPos)
{
    Light fixedLight =
    {
        float3(-1.24180996f, 0.90427703f, 4.60324001f),
        1.395146f,
        0.348787f,
        float3(1.0f, 0.0f, 0.0f)
    };
    
    // Define projection plane (e.g., XZ plane at Y=0)
    float3 planeNormal = float3(0, 1, 0);
    float planeDistance = 0.0f;
    
    // Project light position onto plane
    float lightDistToPlane = dot(fixedLight.position - planeNormal * planeDistance, planeNormal);
    float3 projectedLightPos = fixedLight.position - planeNormal * lightDistToPlane;
    
    // Project pixel position onto same plane
    float pixelDistToPlane = dot(vertexPos - planeNormal * planeDistance, planeNormal);
    float3 projectedPixelPos = vertexPos - planeNormal * pixelDistToPlane;
    
    // 2D distance on the projected plane
    float projectedDist = length(projectedLightPos - projectedPixelPos);
    
    float3 colorIncrement = float3(0.1f, 0.1f, 0.1f);
    
    if (projectedDist < fixedLight.minRadius)
    {
        float intensity = 1.0f;
        
        // Soft edge
        if (projectedDist > fixedLight.maxRadius)
        {
            float edge = (fixedLight.minRadius - projectedDist) / (fixedLight.minRadius - fixedLight.maxRadius);
            intensity = smoothstep(0.0f, 1.0f, edge);
        }
        
        colorIncrement += fixedLight.color * intensity;
    }
    
    return colorIncrement;
}


float3 Compute2DLightSoftBackFace(float3 vertexPos, float3 vertexNormal)
{
    Light fixedLight =
    {
        float3(-1.24180996f, 0.90427703f, 4.60324001f),
        1.395146f,
        0.348787f,
        float3(1.0f, 0.0f, 0.0f)
    };
    
    float3 lightVector = fixedLight.position - vertexPos;
    float3 lightDir = normalize(lightVector);
    
    // Soft back-face factor instead of hard cutoff
    float NdotL = dot(vertexNormal, lightDir);
    float backFaceFactor = saturate(NdotL * 0.8f + 0.2f); // Softer transition
    
    // 2D circle calculation
    float2 lightPos2D = fixedLight.position.xz;
    float2 pixelPos2D = vertexPos.xz;
    float dist2D = length(fixedLight.position - vertexPos); // changed to 3D as it has better litting
    
    float3 colorIncrement = float3(0.1f, 0.1f, 0.1f);
    
    if (dist2D < fixedLight.minRadius)
    {
        float intensity = 1.0f;
        
        if (dist2D > fixedLight.maxRadius)
        {
            float edge = (fixedLight.minRadius - dist2D) / (fixedLight.minRadius - fixedLight.maxRadius);
            intensity = saturate(edge);
        }
        
        // Combine 2D intensity with back-face factor
        colorIncrement += fixedLight.color * intensity * backFaceFactor;
    }
    
    return colorIncrement;
}



float3 CalculatePixelNormal(float3 worldPos)
{
    // This works but can be inaccurate at triangle edges
    float3 dPdx = ddx(worldPos);
    float3 dPdy = ddy(worldPos);
    
    // Try both orientations to get consistent results
    float3 normal1 = cross(dPdx, dPdy);
    float3 normal2 = cross(dPdy, dPdx);
    
    // Choose the one pointing away from origin (rough heuristic)
    float3 normal = (dot(normal1, worldPos) > 0) ? normal1 : normal2;
    return normalize(normal);
}

//struct PS_INPUT
//{
//    float4 diffuse : COLOR0;
//    float4 specular : COLOR1;
//    float2 texCoord : TEXCOORD0;
//    float rhw : TEXCOORD1;
//    float3 positionWS : TEXCOORD2;
//    float3 normalWS : NORMAL0;
//    float bNormal : COLOR3;
    
//};

//sampler2D g_texture : register(s0);


//float4 main(PS_INPUT input) : COLOR
//{
//    float3 g_lightPosition = float3(-1.24180996, 0.90427703, 4.60324001); // Center of the wave in world space, set to position in sector 212 in canyon level 
//    float3 g_lightColor = float3(1.0, 0.0, 0.0); // Center of the wave in world space, set to position in sector 212 in canyon level 
//    float3 AmbientColor = float3(0.1, 0.1, 0.1);

//    // Use texture coordinates directly - they should already be perspective correct
//    // from the original TL vertex calculation
//    float4 texColor = tex2D(g_texture, input.texCoord.xy);

//    // TODO: In case of texture being 0 nothing will be rendered!
//    float4 finalColor = texColor;// * input.diffuse;
    
    
//    float3 normalWS = normalize(input.normalWS);
//    //if (input.bNormal < 0.5)
//    //    normalWS = CalculatePixelNormal(input.positionWS);

//    // Vector from the surface fragment to the light source
//    float3 lightDirection = normalize(g_lightPosition - input.positionWS);

//    // Calculate diffuse factor
//    // Clamp to 0 to prevent negative lighting (when light is behind the surface)
//    float diffuseFactor = max(0.0f, dot(normalWS, lightDirection));

//    // Calculate diffuse color
//    float3 lightColor = ComputeLighting(input.positionWS, normalWS); //g_lightColor * diffuseFactor + AmbientColor;
//    //float3 lightColor = Compute2DCircleLight(input.positionWS);
//    finalColor = float4(finalColor.rgb * lightColor, finalColor.a);

    
    
//    finalColor = ApplyFog(finalColor, input.rhw);
//    return finalColor;
//};









// Wave 

//#include "common.hlsli"

//struct PS_INPUT
//{
//    float4 diffuse : COLOR0;
//    float4 specular : COLOR1;
//    float2 texCoord : TEXCOORD0;
//    float rhw : TEXCOORD1;
//};

//sampler2D g_texture : register(s0);

//float4 main(PS_INPUT input) : COLOR
//{
//    // Wave effect constants for UV distortion (optional enhancement)
//    float3 waveCenter = float3(0.5, 0.5, 0.0); // UV center of wave (0.5, 0.5 = center of texture)
//    float uvWaveRadius = 0.3; // Maximum radius in UV space
//    float uvWaveAmplitude = 0.02; // UV distortion strength
//    float uvWaveFrequency = 8.0; // UV wave frequency
//    float time = 10.0; // Time value (should match vertex shader)
//    float uvWaveSpeed = 2.0; // Speed of UV wave
//    bool enableUVDistortion = true; // Toggle UV distortion effect
    
//    float2 finalTexCoord = input.texCoord;
    
//    // Optional: Add UV distortion for extra wave effect in texture sampling
//    if (enableUVDistortion)
//    {
//        float2 toUV = input.texCoord - waveCenter.xy;
//        float uvDistance = length(toUV);
        
//        if (uvDistance < uvWaveRadius)
//        {
//            float uvWavePhase = (uvDistance * uvWaveFrequency) - (time * uvWaveSpeed);
//            float uvFalloff = 1.0 - (uvDistance / uvWaveRadius);
//            uvFalloff = uvFalloff * uvFalloff; // Smooth falloff
            
//            float uvDisplacement = sin(uvWavePhase) * uvWaveAmplitude * uvFalloff;
            
//            // Apply radial distortion
//            if (uvDistance > 0.001)
//            {
//                float2 uvDirection = normalize(toUV);
//                finalTexCoord += uvDirection * uvDisplacement;
//            }
//        }
//    }
    
//    // Sample texture with potentially distorted coordinates
//    float4 texColor = tex2D(g_texture, finalTexCoord);
    
//    // Apply base diffuse color
//    float4 finalColor = texColor * input.diffuse;
    
//    // Optional: Add some color modification based on wave effect
//    // You could pass wave intensity from vertex shader via an additional texcoord
//    // For now, we'll just apply a subtle effect based on UV distortion
//    if (enableUVDistortion)
//    {
//        float2 toCenter = input.texCoord - float2(0.5, 0.5);
//        float distFromCenter = length(toCenter);
//        if (distFromCenter < uvWaveRadius)
//        {
//            // Subtle blue tint for wave effect
//            float waveIntensity = 1.0 - (distFromCenter / uvWaveRadius);
//            finalColor.rgb += float3(0.05, 0.1, 0.2) * waveIntensity * 0.3;
//        }
//    }
    
//    // Apply fog
//    finalColor = ApplyFog(finalColor, input.rhw);
    
//    return finalColor;
//}



// ORIGINAL

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
    float4 texColor = tex2D(g_texture, input.texCoord.xy);

    //TODO: In case of texture being 0 nothing will be rendered!
    float4 finalColor = texColor * input.diffuse;
    finalColor = ApplyFog(finalColor, input.rhw);
    return finalColor;
};