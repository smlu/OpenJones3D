#include "rdPrimit3.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Engine/rdClip.h>
#include <rdroid/Primitives/rdPrimit2.h>
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/RTI/symbols.h>

void rdPrimit3_ResetGlobals(void)
{}

int J3DAPI rdPrimit3_DrawPoint(rdVector3* pPoint, int32_t* pPPoint, uint32_t color)
{
     // Transform point to view space
    rdVector3 tpoint;
    rdMatrix_TransformPoint34(&tpoint, pPoint, &rdCamera_g_pCurCamera->viewMatrix);
    if ( !rdClip_Point3(rdCamera_g_pCurCamera->pFrustum, &tpoint) )
    {
        return 0;
    }

    // Project point to screen space
    rdVector3 ppoint;
    rdCamera_g_pCurCamera->pfProject(&ppoint, &tpoint);

    int32_t x = (int32_t)ppoint.x;
    int32_t y = (int32_t)ppoint.y;
    if ( pPPoint )
    {
        *pPPoint   = x;
        pPPoint[1] = y;
        pPPoint[2] = (int32_t)ppoint.z;
    }

    if ( rdCamera_g_pCurCamera->pCanvas->pVBuffer->rasterInfo.colorInfo.bpp == 8 )
    {
        rdCamera_g_pCurCamera->pCanvas->pVBuffer->pPixels[rdCamera_g_pCurCamera->pCanvas->pVBuffer->rasterInfo.rowSize * y + x] = color;
    }
    else
    {
        // TODO: only 16 bpp is supported but the pPixels might be 24bit or 32

        *(uint16_t*)&rdCamera_g_pCurCamera->pCanvas->pVBuffer->pPixels[2 * rdCamera_g_pCurCamera->pCanvas->pVBuffer->rasterInfo.rowWidth * y + 2 * x] = color;
    }

    return 1;
}

int J3DAPI rdPrimit3_DrawClippedLine(rdVector3* pPoint1, rdVector3* pPoint2, uint32_t color)
{
    // Transform points to view space
    rdVector3 tpoint2, tpoint1;
    rdMatrix_TransformPoint34(&tpoint1, pPoint1, &rdCamera_g_pCurCamera->viewMatrix);
    rdMatrix_TransformPoint34(&tpoint2, pPoint2, &rdCamera_g_pCurCamera->viewMatrix);

    // Project points to screen space
    rdVector3 ppoint1, ppoint2;
    rdCamera_g_pCurCamera->pfProject(&ppoint1, &tpoint1);
    rdCamera_g_pCurCamera->pfProject(&ppoint2, &tpoint2);
    return rdPrimit2_DrawClippedLine2(ppoint1.x, ppoint1.y, ppoint2.x, ppoint2.y, color);
}

void J3DAPI rdPrimit3_DrawClippedCircle(const rdVector3* pPos, float radius, float step, uint32_t color, uint32_t pattern)
{
    // Transform center to view space
    rdVector3 center;
    rdMatrix_TransformPoint34(&center, pPos, &rdCamera_g_pCurCamera->viewMatrix);

    if ( center.y > 0.0f )
    {
        rdVector3 circlePoint = center;
        circlePoint.x += radius;

        // Project center and point to screen space
        rdVector3 projCenter, projCirclePoint;
        rdCamera_g_pCurCamera->pfProject(&projCenter, &center);
        rdCamera_g_pCurCamera->pfProject(&projCirclePoint, &circlePoint);

        int32_t x = (int32_t)(projCenter.x + 0.5f);
        int32_t y = (int32_t)(projCenter.y + 0.5f);
        float radius2D = projCirclePoint.x - projCenter.x;

        rdPrimit2_DrawClippedCircle(rdCamera_g_pCurCamera->pCanvas, x, y, radius2D, step, color, pattern);
    }
}

void J3DAPI rdPrimit3_ClipFace(const rdClipFrustum* pFrustrum, rdGeometryMode geoMode, rdLightMode lightMode, const rdPrimit3* pSrc, rdPrimit3* pDest, const rdVector2* pTexVertOffset)
{
    RD_ASSERT(pDest->aVertices); // Added: Ensure destination aVertices is not NULL
    RD_ASSERT(pSrc->aVertices); // Added: Ensure source aVertices is not NULL
    RD_ASSERT(pSrc->aVertIdxs); // Added: Ensure source aVertIdxs is not NULL

    switch ( geoMode )
    {
        case RD_GEOMETRY_NONE:
            for ( size_t i = 0; i < pSrc->numVertices; ++i )
            {
                pDest->aVertices[i] = pSrc->aVertices[pSrc->aVertIdxs[i]];
            }

            if ( rdCamera_g_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
            {
                pDest->numVertices = rdClip_Face3S(pFrustrum, pDest->aVertices, pSrc->numVertices);
            }
            else
            {
                pDest->numVertices = rdClip_Face3SOrtho(pFrustrum, pDest->aVertices, pSrc->numVertices);
            }

            break;

        case RD_GEOMETRY_VERTEX:
        case RD_GEOMETRY_WIREFRAME:
            for ( size_t i = 0; i < pSrc->numVertices; ++i )
            {
                pDest->aVertices[i] = pSrc->aVertices[pSrc->aVertIdxs[i]];
            }

            if ( rdCamera_g_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
            {
                pDest->numVertices = rdClip_Face3W(pFrustrum, pDest->aVertices, pSrc->numVertices);
            }
            else
            {
                pDest->numVertices = rdClip_Face3WOrtho(pFrustrum, pDest->aVertices, pSrc->numVertices);
            }

            break;

        case RD_GEOMETRY_SOLID:
            switch ( lightMode )
            {
                case RD_LIGHTING_NONE:
                case RD_LIGHTING_LIT:
                    for ( size_t i = 0; i < pSrc->numVertices; ++i )
                    {
                        pDest->aVertices[i] = pSrc->aVertices[pSrc->aVertIdxs[i]];
                    }

                    if ( rdCamera_g_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
                    {
                        pDest->numVertices = rdClip_Face3S(pFrustrum, pDest->aVertices, pSrc->numVertices);
                    }
                    else
                    {
                        pDest->numVertices = rdClip_Face3SOrtho(pFrustrum, pDest->aVertices, pSrc->numVertices);
                    }

                    break;

                case RD_LIGHTING_DIFFUSE:
                    for ( size_t i = 0; i < pSrc->numVertices; ++i )
                    {
                        pDest->aVertices[i] = pSrc->aVertices[pSrc->aVertIdxs[i]];
                    }

                    if ( rdCamera_g_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
                    {
                        pDest->numVertices = rdClip_Face3S(pFrustrum, pDest->aVertices, pSrc->numVertices);
                    }
                    else
                    {
                        pDest->numVertices = rdClip_Face3SOrtho(pFrustrum, pDest->aVertices, pSrc->numVertices);
                    }

                    break;

                case RD_LIGHTING_GOURAUD:
                    if ( pSrc->aVertIntensities )
                    {
                        for ( size_t i = 0; i < pSrc->numVertices; ++i )
                        {
                            pDest->aVertices[i] = pSrc->aVertices[pSrc->aVertIdxs[i]];

                            // TODO: verify if it's a bug assigning to pDest->aVertLights[i] instead of pSrc->aVertLights[pSrc->aVertIdxs[i]]
                            // Verify if not adding intensities alpha is ok.

                            pDest->aVertLights[i] = pSrc->aVertLights[pSrc->aVertIdxs[i]];
                            rdVector_Add4(&pDest->aVertLights[i], &pSrc->aVertLights[pSrc->aVertIdxs[i]], &pSrc->aVertIntensities[i]); // Fixed: Add aVertIntensities[i].alpha. OG set dest alpha to aVertLights[i].alpha
                            rdMath_ClampVector4Acc(&pDest->aVertLights[i], 0.0f, 1.0f);
                        }
                    }
                    else
                    {
                        for ( size_t i = 0; i < pSrc->numVertices; ++i )
                        {
                            pDest->aVertices[i] = pSrc->aVertices[pSrc->aVertIdxs[i]];

                            pDest->aVertLights[i] = pSrc->aVertLights[pSrc->aVertIdxs[i]];
                            rdMath_ClampVector4Acc(&pDest->aVertLights[i], 0.0f, 1.0f);
                        }
                    }

                    if ( rdCamera_g_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
                    {
                        pDest->numVertices = rdClip_Face3GS(pFrustrum, pDest->aVertices, pDest->aVertLights, pSrc->numVertices);
                    }
                    else
                    {
                        pDest->numVertices = rdClip_Face3GSOrtho(pFrustrum, pDest->aVertices, pDest->aVertLights, pSrc->numVertices);
                    }

                    break;

                default:
                    return;
            }

            break;

        case RD_GEOMETRY_FULL:
            RD_ASSERT(pDest->aVertLights); // Added: Ensure aVertLights is allocated
            if ( lightMode >= RD_LIGHTING_NONE )
            {
                if ( lightMode <= RD_LIGHTING_DIFFUSE )
                {
                    for ( size_t i = 0; i < pSrc->numVertices; ++i )
                    {
                        pDest->aVertices[i] = pSrc->aVertices[pSrc->aVertIdxs[i]];

                        pDest->aTexVertices[i] = pSrc->aTexVertices[pSrc->aTexVertIdxs[i]];
                        rdVector_Add2Acc(&pDest->aTexVertices[i], pTexVertOffset);

                        // Fixed: Set vertex intensities to vertex lights, fixes alpha value for transparent polygons with no litting
                        if ( pSrc->aVertIntensities )
                        {
                            pDest->aVertLights[i] = pSrc->aVertIntensities[i];
                        }
                    }

                    if ( rdCamera_g_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
                    {
                        pDest->numVertices = rdClip_Face3GT(pFrustrum, pDest->aVertices, pDest->aTexVertices, pSrc->numVertices);
                    }
                    else
                    {
                        pDest->numVertices = rdClip_Face3GTOrtho(pFrustrum, pDest->aVertices, pDest->aTexVertices, pSrc->numVertices);
                    }
                }
                else if ( lightMode == RD_LIGHTING_GOURAUD )
                {
                    if ( pSrc->aVertLights ) // Fixed: Add check null
                    {
                        if ( pSrc->aVertIntensities )
                        {
                            for ( size_t i = 0; i < pSrc->numVertices; ++i )
                            {
                                pDest->aVertices[i] = pSrc->aVertices[pSrc->aVertIdxs[i]];

                                pDest->aTexVertices[i] = pSrc->aTexVertices[pSrc->aTexVertIdxs[i]];
                                rdVector_Add2Acc(&pDest->aTexVertices[i], pTexVertOffset);

                                // TODO: verify if it's a bug assigning to pDest->aVertLights[i] instead of pSrc->aVertLights[pSrc->aVertIdxs[i]]
                                // Verify if not adding intensities alpha is ok.

                                pDest->aVertLights[i] = pSrc->aVertLights[pSrc->aVertIdxs[i]];
                                rdVector_Add4(&pDest->aVertLights[i], &pSrc->aVertLights[pSrc->aVertIdxs[i]], &pSrc->aVertIntensities[i]); // Fixed: Add aVertIntensities[i].alpha. OG set dest alpha to aVertLights[i].alpha
                                rdMath_ClampVector4Acc(&pDest->aVertLights[i], 0.0f, 1.0f);

                                /*if ( pSrc->aVertLights[pSrc->aVertIdxs[i]].red + pSrc->aVertIntensities[i].red < 0.0f )
                                {
                                    red = 0.0f;
                                }

                                else if ( pSrc->aVertLights[pSrc->aVertIdxs[i]].red + pSrc->aVertIntensities[i].red > 1.0f )
                                {
                                    red = 1.0f;
                                }
                                else
                                {
                                    red = pSrc->aVertLights[pSrc->aVertIdxs[i]].red + pSrc->aVertIntensities[i].red;
                                }

                                pDest->aVertLights[i].red = red;

                                if ( pSrc->aVertLights[pSrc->aVertIdxs[i]].green + pSrc->aVertIntensities[i].green < 0.0f )
                                {
                                    green = 0.0f;
                                }

                                else if ( pSrc->aVertLights[pSrc->aVertIdxs[i]].green + pSrc->aVertIntensities[i].green > 1.0f )
                                {
                                    green = 1.0f;
                                }
                                else
                                {
                                    green = pSrc->aVertLights[pSrc->aVertIdxs[i]].green + pSrc->aVertIntensities[i].green;
                                }

                                pDest->aVertLights[i].green = green;

                                if ( pSrc->aVertLights[pSrc->aVertIdxs[i]].blue + pSrc->aVertIntensities[i].blue < 0.0f )
                                {
                                    blue = 0.0f;
                                }

                                else if ( pSrc->aVertLights[pSrc->aVertIdxs[i]].blue + pSrc->aVertIntensities[i].blue > 1.0f )
                                {
                                    blue = 1.0f;
                                }
                                else
                                {
                                    blue = pSrc->aVertLights[pSrc->aVertIdxs[i]].blue + pSrc->aVertIntensities[i].blue;
                                }

                                pDest->aVertLights[i].blue = blue;

                                if ( pSrc->aVertLights[pSrc->aVertIdxs[i]].alpha < 0.0f )
                                {
                                    alpha = 0.0f;
                                }

                                else if ( pSrc->aVertLights[pSrc->aVertIdxs[i]].alpha > 1.0f )
                                {
                                    alpha = 1.0f;
                                }
                                else
                                {
                                    alpha = pSrc->aVertLights[pSrc->aVertIdxs[i]].alpha;
                                }

                                pDest->aVertLights[i].alpha = alpha;*/
                            }
                        }
                        else
                        {
                            for ( size_t i = 0; i < pSrc->numVertices; ++i )
                            {
                                pDest->aVertices[i]    = pSrc->aVertices[pSrc->aVertIdxs[i]];

                                pDest->aTexVertices[i] = pSrc->aTexVertices[pSrc->aTexVertIdxs[i]];
                                rdVector_Add2Acc(&pDest->aTexVertices[i], pTexVertOffset);

                                // TODO: verify if it's a bug assigning to pDest->aVertLights[i] instead of pSrc->aVertLights[pSrc->aVertIdxs[i]]


                                pDest->aVertLights[i] = pSrc->aVertLights[pSrc->aVertIdxs[i]];
                                rdMath_ClampVector4Acc(&pDest->aVertLights[i], 0.0f, 1.0f);
                            }
                        }
                    }
                    else
                    {
                        for ( size_t i = 0; i < pSrc->numVertices; ++i )
                        {
                            pDest->aVertices[i]    = pSrc->aVertices[pSrc->aVertIdxs[i]];

                            pDest->aTexVertices[i] = pSrc->aTexVertices[pSrc->aTexVertIdxs[i]];
                            rdVector_Add2Acc(&pDest->aTexVertices[i], pTexVertOffset);
                        }
                    }

                    if ( rdCamera_g_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
                    {
                        pDest->numVertices = rdClip_Face3T(pFrustrum, pDest->aVertices, pDest->aTexVertices, pDest->aVertLights, pSrc->numVertices);
                    }
                    else
                    {
                        pDest->numVertices = rdClip_Face3TOrtho(pFrustrum, pDest->aVertices, pDest->aTexVertices, pDest->aVertLights, pSrc->numVertices);
                    }
                }
            }

            break;

        default:
            return;
    }
}