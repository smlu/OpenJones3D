#include "rdLight.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Main/rdroid.h>
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/RTI/symbols.h>

#include <std/General/std.h>
#include <std/General/stdMemory.h>

void rdLight_InstallHooks(void)
{
    J3D_HOOKFUNC(rdLight_NewEntry);
    J3D_HOOKFUNC(rdLight_CalcVertexIntensities);
    J3D_HOOKFUNC(rdLight_CalcFaceIntensity);
    J3D_HOOKFUNC(rdLight_GetIntensity);
}

void rdLight_ResetGlobals(void)
{

}

rdLight* rdLight_New(void)
{
    rdLight* pLight = (rdLight*)STDMALLOC(sizeof(rdLight));
    if ( !pLight )
    {
        RDLOG_ERROR("Error allocating memory for light.\n");
        return NULL;
    }

    rdLight_NewEntry(pLight);
    return pLight;
}

int J3DAPI rdLight_NewEntry(rdLight* pLight)
{
    // type 2 - probably point light
    // Mots 3 - probably spot light
    pLight->type = 2;

    pLight->bEnabled = 1;

    pLight->direction.x = 0.0f;
    pLight->direction.y = 0.0f;
    pLight->direction.z = 0.0f;

    pLight->color.red   = 1.0f;
    pLight->color.green = 1.0f;
    pLight->color.blue  = 1.0f;
    pLight->color.alpha = 0.0f;

    pLight->minRadius = 0.0f;
    pLight->maxRadius = 0.0f;
    return 1;
}

void J3DAPI rdLight_Free(rdLight* pLight)
{
    if ( !pLight )
    {
        RDLOG_ERROR("Warning: Attempt to free NULL light ptr.\n");
        return;
    }

    rdLight_FreeEntry(pLight);
    stdMemory_Free(pLight);
}

void rdLight_FreeEntry(rdLight* pLight)
{
    J3D_UNUSED(pLight);
}

void J3DAPI rdLight_CalcVertexIntensities(const rdLight** apLights, const rdVector3* aLightPos, size_t numLights, const rdVector3* aVertexNormal, const rdVector3* aVertices, const rdVector4* aVertexColors, rdVector4* aColors, size_t numVertices)
{
    for ( int i = numVertices - 1; i >= 0; --i )
    {
        aColors[i] = aVertexColors[i];

        for ( int j = numLights - 1; j >= 0; --j )
        {
            const rdLight* pLight = apLights[j];

            rdVector3 lightDir;
            rdVector_Sub3(&lightDir, &aLightPos[j], &aVertices[i]);
            float dist = rdVector_Len3(&lightDir);
            if ( dist < (double)pLight->minRadius )
            {
                rdVector_Normalize3Acc(&lightDir);
                float dot = rdVector_Dot3(&aVertexNormal[i], &lightDir);
                if ( dot > 0.0f )
                {
                    if ( dist < (double)pLight->maxRadius )
                    {
                        aColors[i].red   += pLight->color.red * dot;
                        aColors[i].green += pLight->color.green * dot;
                        aColors[i].blue  += pLight->color.blue * dot;
                    }
                    else
                    {
                        float drad  = pLight->minRadius - pLight->maxRadius;
                        float atten = (dist - pLight->maxRadius) / drad;

                        aColors[i].red   += (pLight->color.red - atten * pLight->color.red) * dot;
                        aColors[i].green += (pLight->color.green - atten * pLight->color.green) * dot;
                        aColors[i].blue  += (pLight->color.blue - atten * pLight->color.blue) * dot;
                    }
                }
            }
        }
    }
}

void J3DAPI rdLight_CalcDistVertexIntensities(const rdLight** apLights, const rdVector3* aLightPos, size_t numLights, const rdVector3* aVertexNormal, const rdVector3* aVertices, const rdVector4* aVertexColors, rdVector4* aLightColors, size_t numVertices, float attenuation)
{
    J3D_UNUSED(aVertexNormal);
    for ( size_t i = 0; i < numVertices; ++i )
    {
        aLightColors[i] = aVertexColors[i];

        for ( size_t j = 0; j < numLights; ++j )
        {
            const rdLight* pLight = apLights[j];

            rdVector3 dpos;
            rdVector_Sub3(&dpos, &aLightPos[j], &aVertices[i]);
            float dist = rdVector_Len3(&dpos);
            if ( dist < (double)pLight->minRadius )
            {
                attenuation = dist * attenuation;
                attenuation = pLight->color.red - attenuation;
                aLightColors[i].red   += attenuation;
                aLightColors[i].green += attenuation;
                aLightColors[i].blue  += attenuation;
            }
        }
    }
}

void J3DAPI rdLight_CalcFaceIntensity(const rdLight** apLights, const rdVector3* apLightPos, size_t numLights, const rdFace* pFace, const rdVector3* pNormal, const rdVector3* apVertices, float attenuation, rdVector4* pOutColor)
{
    rdVector_Set4(pOutColor, 0.0f, 0.0f, 0.0f, 0.0f);

    for ( size_t i = 0; i < numLights; ++i )
    {
        const rdLight* pLight = apLights[i];
        if ( pLight->bEnabled )
        {
            rdVector3 dir;
            rdVector_Sub3(&dir, &apLightPos[i], &apVertices[*pFace->aVertices]);

            float dist = rdMath_DistancePointToPlane(&apLightPos[i], pNormal, &apVertices[*pFace->aVertices]);
            if ( dist < (double)pLight->minRadius )
            {
                rdVector_Normalize3Acc(&dir);
                float dot = rdVector_Dot3(pNormal, &dir);
                if ( dot > 0.0f )
                {
                    float atten = dist * attenuation;
                    pOutColor->red   += (pLight->color.red - atten) * dot;
                    pOutColor->green += (pLight->color.green - atten) * dot;
                    pOutColor->blue  += (pLight->color.blue - atten) * dot;
                }
            }
        }
    }
}

void J3DAPI rdLight_CalcDistFaceIntensity(const rdLight** apLights, const rdVector3* apLightPos, size_t numLights, const rdFace* pFace, const rdVector3* aVerts, float attenuation, rdVector4* pOutColor)
{
    rdVector_Set4(pOutColor, 0.0f, 0.0f, 0.0f, 0.0f);

    for ( size_t i = 0; i < numLights; ++i )
    {
        const rdLight* pLight = apLights[i];
        if ( pLight->bEnabled )
        {
            float dist = rdMath_DistancePointToPlane(&apLightPos[i], &pFace->normal, &aVerts[*pFace->aVertices]);
            if ( dist < (double)pLight->minRadius )
            {
                attenuation = dist * attenuation;
                pOutColor->red   += pLight->color.red - attenuation;
                pOutColor->green += pLight->color.green - attenuation;
                pOutColor->blue  += pLight->color.blue - attenuation;
            }
        }
    }
}

float J3DAPI rdLight_GetIntensity(const rdVector4* pLight)
{
    return (pLight->red + pLight->green + pLight->blue) / 3.0f;
}
