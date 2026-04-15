#include "rdPolyline.h"
#include <j3dcore/j3dhook.h>
#include <rdroid/RTI/symbols.h>

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Engine/rdClip.h>
#include <rdroid/Engine/rdMaterial.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Raster/rdCache.h>
#include <rdroid/RTI/symbols.h>

#include <sith/World/sithWorld.h>

#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

rdVector3 rdPolyline_aView[4];

void rdPolyline_InstallHooks(void)
{
    J3D_HOOKFUNC(rdPolyline_New);
    J3D_HOOKFUNC(rdPolyline_NewEntry);
    J3D_HOOKFUNC(rdPolyline_Free);
    J3D_HOOKFUNC(rdPolyline_FreeEntry);
    J3D_HOOKFUNC(rdPolyline_Draw);
    J3D_HOOKFUNC(rdPolyline_DrawFace);
}

void rdPolyline_ResetGlobals(void)
{}

rdPolyline* J3DAPI rdPolyline_New(const char* pName, const char* pMatFilename, const char* pMatFilename2, float length, float baseRadius, float tipRadius, rdGeometryMode geoMode, rdLightMode lightMode, const rdVector4* pColor)
{
    J3D_UNUSED(pMatFilename2);
    return rdPolyline_NewEx(pName, pMatFilename, length, baseRadius, tipRadius, geoMode, lightMode, pColor, (rdPolylineFlags)0);
}

rdPolyline* J3DAPI rdPolyline_NewEx(const char* pName, const char* pMatFilename, float length, float baseRadius, float tipRadius, rdGeometryMode geoMode, rdLightMode lightMode, const rdVector4* pColor, rdPolylineFlags flags)
{
    rdPolyline* pLine = (rdPolyline*)STDMALLOC(sizeof(rdPolyline));
    if ( !pLine )
    {
        RDLOG_ERROR("Error allocating memory for polyline.\n");
        return NULL;
    }

    rdPolyline_NewEntry(pLine, pName, pMatFilename, length, baseRadius, tipRadius, geoMode, lightMode, pColor);
    pLine->flags = flags;

    return pLine;
}

int J3DAPI rdPolyline_NewEntry(rdPolyline* pLine, const char* pName, const char* pMatFilename, float length, float baseRadius, float tipRadius, rdGeometryMode geoMode, rdLightMode lightMode, const rdVector4* pColor)
{
    if ( pName )
    {
        STD_STRCPY(pLine->aName, pName);
    }

    pLine->length            = length;
    pLine->baseRadius        = baseRadius;
    pLine->tipRadius         = tipRadius;
    pLine->face.flags        = RD_FF_FOG_ENABLED | RD_FF_TEX_TRANSLUCENT | RD_FF_DOUBLE_SIDED;
    pLine->face.geometryMode = geoMode;
    pLine->face.lightingMode = lightMode;
    pLine->geoMode           = geoMode;
    pLine->lightMode         = lightMode;
    pLine->face.extraLight   = *pColor;

    pLine->face.pMaterial = rdMaterial_Load(pMatFilename);
    if ( !pLine->face.pMaterial )
    {
        return 0;
    }

    pLine->face.numVertices = 4;
    pLine->face.aVertices   = (int*)STDMALLOC(sizeof(*pLine->face.aVertices) * pLine->face.numVertices);
    if ( !pLine->face.aVertices )
    {
        goto alloc_error;
    }

    for ( size_t i = 0; i < pLine->face.numVertices; ++i )
    {
        pLine->face.aVertices[i] = i;
    }

    if ( pLine->face.geometryMode == RD_GEOMETRY_FULL )
    {
        pLine->face.aTexVertices = (int*)STDMALLOC(sizeof(*pLine->face.aTexVertices) * pLine->face.numVertices);
        if ( !pLine->face.aTexVertices )
        {
            goto alloc_error;
        }

        for ( size_t i = 0; i < pLine->face.numVertices; ++i )
        {
            pLine->face.aTexVertices[i] = i;
        }

        pLine->apUVs = (rdVector2*)STDMALLOC(sizeof(*pLine->apUVs) * pLine->face.numVertices);
        if ( !pLine->apUVs )
        {
            goto alloc_error;
        }

        pLine->apUVs[0].x = 1.0f;
        pLine->apUVs[0].y = 0.0f;

        pLine->apUVs[1].x = 1.0f;
        pLine->apUVs[1].y = 1.0f;

        pLine->apUVs[2].x = 0.0f;
        pLine->apUVs[2].y = 1.0f;

        pLine->apUVs[3].x = 0.0f;
        pLine->apUVs[3].y = 0.0f;
    }

    // Success
    pLine->face.texVertOffset.y = 0.0f;
    pLine->face.texVertOffset.x = 0.0f;
    return 1;

alloc_error:
    RDLOG_ERROR("Error: Bad memory allocation for polyline '%'.\n", pName);
    return 0;
}

void J3DAPI rdPolyline_Free(rdPolyline* pLine)
{
    if ( !pLine )
    {
        RDLOG_ERROR("Warning: Attempt to free NULL polyline ptr.\n");
        return;
    }

    rdPolyline_FreeEntry(pLine);
    stdMemory_Free(pLine);
}

void J3DAPI rdPolyline_FreeEntry(rdPolyline* pPolyline)
{
    if ( pPolyline->apUVs )
    {
        stdMemory_Free(pPolyline->apUVs);
        pPolyline->apUVs = NULL;
    }

    if ( pPolyline->face.aVertices )
    {
        stdMemory_Free(pPolyline->face.aVertices);
        pPolyline->face.aVertices = NULL;
    }

    if ( pPolyline->face.aTexVertices )
    {
        stdMemory_Free(pPolyline->face.aTexVertices);
        pPolyline->face.aTexVertices = NULL;
    }
}

int J3DAPI rdPolyline_Draw(const rdThing* pLine, const rdMatrix34* pOrient)
{
    rdPolyline* pPolyline = pLine->data.pPolyline;

    // Combine polyline model matrix with camera view matrix
    rdMatrix34 viewMatrix;
    rdMatrix_Multiply34(&viewMatrix, &rdCamera_g_pCurCamera->viewMatrix, pOrient);

    rdVector3 vecFwd;
    vecFwd.x = 0.0f;
    vecFwd.y = pPolyline->length;
    vecFwd.z = 0.0f;

    // Transform polyline end point to view space
    rdVector3 vecEnd;
    rdMatrix_TransformPoint34(&vecEnd, &vecFwd, &viewMatrix);

    float deX   = vecEnd.x - viewMatrix.dvec.x;
    float deZ   = vecEnd.z - viewMatrix.dvec.z;
    float xzLen = sqrtf(deX * deX + deZ * deZ);

    float sinv  = -deX / xzLen;
    float angle = stdMath_ArcSin3(sinv);
    if ( deZ < 0.0f )
    {
        if ( deX <= 0.0f )
        {
            angle = -(angle + 180.0f);
        }
        else
        {
            angle = 180.0f - angle;
        }
    }

    float cosv;
    stdMath_SinCos(angle, &sinv, &cosv);

    rdPolyline_aView[0].x = pPolyline->tipRadius * cosv - xzLen * sinv + viewMatrix.dvec.x;
    rdPolyline_aView[0].y = vecEnd.y;
    rdPolyline_aView[0].z = pPolyline->tipRadius * sinv + xzLen * cosv + viewMatrix.dvec.z;

    rdPolyline_aView[1].x = -pPolyline->tipRadius * cosv - xzLen * sinv + viewMatrix.dvec.x;
    rdPolyline_aView[1].y = vecEnd.y;
    rdPolyline_aView[1].z = -pPolyline->tipRadius * sinv + xzLen * cosv + viewMatrix.dvec.z;

    rdPolyline_aView[2].x = -pPolyline->baseRadius * cosv - 0.0f * sinv + viewMatrix.dvec.x;
    rdPolyline_aView[2].y = viewMatrix.dvec.y;
    rdPolyline_aView[2].z = -pPolyline->baseRadius * sinv + 0.0f * cosv + viewMatrix.dvec.z;

    rdPolyline_aView[3].x = pPolyline->baseRadius * cosv - 0.0f * sinv + viewMatrix.dvec.x;
    rdPolyline_aView[3].y = viewMatrix.dvec.y;
    rdPolyline_aView[3].z = pPolyline->baseRadius * sinv + 0.0f * cosv + viewMatrix.dvec.z;

    // Added: Bias vertices them slightly toward camera (view space) to resolve Z-fighting
    float depthBias = 0.0001f;
    for ( int i = 0; i < 2; ++i )
    {
        rdVector3 toCamera;
        rdVector_Normalize3(&toCamera, &rdPolyline_aView[i]);
        rdPolyline_aView[i].x -= toCamera.x * depthBias;
        rdPolyline_aView[i].y -= toCamera.y * depthBias;
        rdPolyline_aView[i].z -= toCamera.z * depthBias;
    }

    rdPolyline_DrawFace(pLine, &pPolyline->face, rdPolyline_aView, pPolyline->apUVs);
    return 1;
}
#ifdef J3D_OPENGL

static void J3DAPI rdPolyline_DrawFaceStatic(const rdThing* pLine, const rdFace* pFace, const rdVector3* aVerts, const rdVector2* aTVerts) // aVerts should be transformed to view space
{
    // Added: When UV tile flag set, tile polyline UVs instead of stretch as done by default.
    //        This will make texture to repeat multiple times throughout the polyline.
    //
    //        When UV rotate flag is set, rotate UV for 90 degrees.
    if ( (pLine->data.pPolyline->flags & (RDPOLYLINE_UVTILE | RDPOLYLINE_UVROTATE)) != 0 )
    {
        rdPolyline* pPolyline = pLine->data.pPolyline;
        float texRepeat       = 1.0f; // Default for non-tiling

        if ( pPolyline->flags & RDPOLYLINE_UVTILE )
        {
            float actualLength = rdVector_Dist3(&aVerts[0], &aVerts[3]);
            if ( actualLength > 0.0f )
            {
                // Tile texture based on length
                float avgRadius = (pPolyline->baseRadius + pPolyline->tipRadius) * 0.5f;
                float circumf   = STDMATH_CIRCLE_CIRCUMF(avgRadius);
                texRepeat       = actualLength / circumf;
                if ( texRepeat < 0.1f )
                {
                    texRepeat = 0.1f;
                }
            }
        }

        rdVector2 aUVs[4];

        // Check if UVs should be rotated 90 degrees
        if ( pPolyline->flags & RDPOLYLINE_UVROTATE )
        {
            // Rotated 90 degrees clockwise: swap U/V and adjust orientation
            // Original mapping: U along length, V around circumference
            // Rotated mapping: V along length, U around circumference
            aUVs[0].x = 0.0f;
            aUVs[0].y = texRepeat; // tip right
            aUVs[1].x = 1.0f;
            aUVs[1].y = texRepeat; // tip left
            aUVs[2].x = 1.0f;
            aUVs[2].y = 0.0f; // base left
            aUVs[3].x = 0.0f;
            aUVs[3].y = 0.0f; // base right
        }
        else
        {
            // Standard mapping: U along length, V around circumference
            aUVs[0].x = texRepeat;
            aUVs[0].y = 0.0f;
            aUVs[1].x = texRepeat;
            aUVs[1].y = 1.0f;
            aUVs[2].x = 0.0f;
            aUVs[2].y = 1.0f;
            aUVs[3].x = 0.0f;
            aUVs[3].y = 0.0f;
        }

        aTVerts = aUVs;
    }
    rdVector4 extraLight = pFace->extraLight;

    if ( (rdroid_g_curRenderOptions & RDROID_USE_AMBIENT_CAMERA_LIGHT) != 0 )
    {
        rdVector_Add4Acc(&extraLight, &rdCamera_g_pCurCamera->ambientLight);
        rdMath_ClampVector4Acc(&extraLight, 0.0f, 1.0f); // Added: Clamp to [0,1.0]
    }


    rdPayload* pPayload  = rdCache_GetTransparentDrawCall(RD_DRAW_POLYLINE);
    pPayload->extraLight = extraLight;
    pPayload->flags      = RD_FF_FOG_ENABLED | RD_FF_ZWRITE_DISABLED | RD_FF_TEX_TRANSLUCENT;
    // Fixed: Disable fog rendering for poly when fog is globally disabled
    //        OG: Poly fog rendering was enabled by default which lead to undesired render effect when fog is disabled in level (i.e.: fog color is applied)
    if ( !sithWorld_g_pCurrentWorld->fog.bEnabled ) // // TODO: add special function that will enable/disable fog rendering
    {
        pPayload->flags &= ~RD_FF_FOG_ENABLED;
    }
    pPayload->matCelNum          = pFace->matCelNum;
    pPayload->pMaterial          = pFace->pMaterial;
    rdPolyLinePayload* pDrawCall = &pPayload->polyLinePayload;
    for ( size_t i = 0; i < 4; i++ )
    {
        pDrawCall->vertices[i]  = aVerts[i];
        pDrawCall->texCoords[i] = aTVerts[i];
    }

    pPayload->lightingMode = RD_LIGHTING_NONE;

    float distance = FLT_MAX; // 3.4028235e38f;

    for ( size_t j = 0; j < 4; ++j )
    {
        rdVector3 min = aVerts[j];

        if ( min.y < distance )
        {
            distance = min.y;
        }
    }
    pPayload->distance = distance;
    rdCache_AddTransparentDrawCall();
}
#endif


void J3DAPI rdPolyline_DrawFace(const rdThing* pLine, const rdFace* pFace, const rdVector3* aVerts, const rdVector2* aTVerts) // aVerts should be transformed to view space
{
#ifdef J3D_OPENGL
    if ( !std3D_g_bUseLegacyRendering )
    {
        rdPolyline_DrawFaceStatic(pLine, pFace, aVerts, aTVerts);
        return;
    }
#endif
    J3D_UNUSED(pLine);
    rdCacheProcEntry* pPoly = rdCache_GetAlphaProcEntry();
    if ( !pPoly )
    {
        // TODO: Maybe log error?
        return;
    }


    // Added: When UV tile flag set, tile polyline UVs instead of stretch as done by default.
    //        This will make texture to repeat multiple times throughout the polyline.
    //
    //        When UV rotate flag is set, rotate UV for 90 degrees.
    if ( (pLine->data.pPolyline->flags & (RDPOLYLINE_UVTILE | RDPOLYLINE_UVROTATE)) != 0 )
    {
        rdPolyline* pPolyline = pLine->data.pPolyline;
        float texRepeat = 1.0f;  // Default for non-tiling

        if ( pPolyline->flags & RDPOLYLINE_UVTILE )
        {
            float actualLength = rdVector_Dist3(&aVerts[0], &aVerts[3]);
            if ( actualLength > 0.0f )
            {
                    // Tile texture based on length
                float avgRadius = (pPolyline->baseRadius + pPolyline->tipRadius) * 0.5f;
                float circumf = STDMATH_CIRCLE_CIRCUMF(avgRadius);
                texRepeat = actualLength / circumf;
                if ( texRepeat < 0.1f )
                {
                    texRepeat = 0.1f;
                }
            }
        }

        rdVector2 aUVs[4];

        // Check if UVs should be rotated 90 degrees
        if ( pPolyline->flags & RDPOLYLINE_UVROTATE )
        {
            // Rotated 90 degrees clockwise: swap U/V and adjust orientation
            // Original mapping: U along length, V around circumference
            // Rotated mapping: V along length, U around circumference
            aUVs[0].x = 0.0f;
            aUVs[0].y = texRepeat;  // tip right
            aUVs[1].x = 1.0f;
            aUVs[1].y = texRepeat;  // tip left
            aUVs[2].x = 1.0f;
            aUVs[2].y = 0.0f;       // base left
            aUVs[3].x = 0.0f;
            aUVs[3].y = 0.0f;       // base right
        }
        else
        {
            // Standard mapping: U along length, V around circumference
            aUVs[0].x = texRepeat;
            aUVs[0].y = 0.0f;
            aUVs[1].x = texRepeat;
            aUVs[1].y = 1.0f;
            aUVs[2].x = 0.0f;
            aUVs[2].y = 1.0f;
            aUVs[3].x = 0.0f;
            aUVs[3].y = 0.0f;
        }

        aTVerts = aUVs;
    }

// Transform verts to screen space and assign to poly
    if ( !rdClip_FaceToPlane(rdCamera_g_pCurCamera->pFrustum, pPoly, pFace, aVerts, aTVerts, NULL, NULL) )
    {
        // Polyline face is fully outside the camera frustum
        return;
    }

    for ( size_t i = 0; i < pFace->numVertices; ++i )
    {
        pPoly->aVertIntensities[i] = pFace->extraLight;
    }

    if ( (rdroid_g_curRenderOptions & RDROID_USE_AMBIENT_CAMERA_LIGHT) != 0 )
    {
        rdVector_Add4Acc(&pPoly->extraLight, &rdCamera_g_pCurCamera->ambientLight);
        rdMath_ClampVector4Acc(&pPoly->extraLight, 0.0f, 1.0f); // Added: Clamp to [0,1.0]
    }

    pPoly->flags = pFace->flags;
    pPoly->flags |= RD_FF_FOG_ENABLED | RD_FF_ZWRITE_DISABLED | RD_FF_TEX_TRANSLUCENT;

    // Fixed: Disable fog rendering for poly when fog is globally disabled
    //        OG: Poly fog rendering was enabled by default which lead to undesired render effect when fog is disabled in level (i.e.: fog color is applied)
    if ( !sithWorld_g_pCurrentWorld->fog.bEnabled ) // // TODO: add special function that will enable/disable fog rendering
    {
        pPoly->flags &= ~RD_FF_FOG_ENABLED;
    }

    pPoly->matCelNum    = pFace->matCelNum;
    pPoly->lightingMode = RD_LIGHTING_NONE;
    pPoly->pMaterial    = pFace->pMaterial;
    rdCache_AddAlphaProcFace(pFace->numVertices);
}
