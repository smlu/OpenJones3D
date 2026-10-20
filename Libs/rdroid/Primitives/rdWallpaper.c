#include "rdWallpaper.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Engine/rdMaterial.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Raster/rdCache.h>
#include <rdroid/RTI/symbols.h>

#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdDisplay.h>

#define RDWALLPAPER_NUMMATS        6u 
#define RDWALLPAPER_LINE_THICKNESS 6.0f

static rdVector4 rdWallpaper_extraLight = { 1.0f, 1.0f, 1.0f, 1.0f };

void rdWallpaper_InstallHooks(void)
{
    J3D_HOOKFUNC(rdWallpaper_New);
    J3D_HOOKFUNC(rdWallpaper_Free);
    J3D_HOOKFUNC(rdWallpaper_Draw);
    J3D_HOOKFUNC(rdWallpaper_NewWallLine);
    J3D_HOOKFUNC(rdWallpaper_FreeWallLine);
    J3D_HOOKFUNC(rdWallpaper_DrawWallLine);
}

void rdWallpaper_ResetGlobals(void)
{}

rdWallpaper* J3DAPI rdWallpaper_New(const char* pName)
{
    rdWallpaper* pWallpapaer = (rdWallpaper*)STDMALLOC(sizeof(rdWallpaper));
    if ( !pWallpapaer )
    {
        RDLOG_ERROR("Error allocating memory for wallpaper.\n");
        return NULL;
    }

    memset(pWallpapaer, 0, sizeof(rdWallpaper));

    STD_STRCPY(pWallpapaer->aName, pName);
    pWallpapaer->pCamera = rdCamera_New(90.0f, 0, 0.001f, 100.0f, 1.0f);

    for ( size_t i = 0; i < RDWALLPAPER_NUMMATS; ++i )
    {
        char aMatName[64];
        STD_FORMAT(aMatName, "%s_%1d.mat", pWallpapaer->aName, i + 1);
        if ( rdMaterial_LoadEntry(aMatName, &pWallpapaer->aMaterials[i]) )
        {
            // Error loading mat
            if ( pWallpapaer ) {
                rdWallpaper_Free(pWallpapaer);
            }
            return NULL;
        }
    }

    return pWallpapaer;
}

void J3DAPI rdWallpaper_Free(rdWallpaper* pWallpaper)
{
    if ( pWallpaper )
    {
        rdCamera_Free(pWallpaper->pCamera);
        for ( size_t i = 0; i < RDWALLPAPER_NUMMATS; ++i ) {
            rdMaterial_FreeEntry(&pWallpaper->aMaterials[i]);
        }
        stdMemory_Free(pWallpaper);
    }
}

void J3DAPI rdWallpaper_Draw(const rdWallpaper* pWallpaper)
{
    uint32_t height, width;
    stdDisplay_GetBackBufferSize(&width, &height);
    float swidth  = (float)width / RD_REF_WIDTH;
    float sheight = (float)height / RD_REF_HEIGHT;

    rdCamera_SetCurrent(pWallpaper->pCamera);

    for ( size_t matNum = 0; matNum < RDWALLPAPER_NUMMATS; ++matNum )
    {
        rdCacheProcEntry* pPoly = rdCache_GetProcEntry();
        if ( !pPoly )
        {
            // Error getting proc entry
            return;
        }

        pPoly->flags        = RD_FF_TEX_CLAMP_Y | RD_FF_TEX_CLAMP_X;
        pPoly->lightingMode = RD_LIGHTING_DIFFUSE;
        pPoly->numVertices  = 4;
        rdVector_Copy4(&pPoly->extraLight, &rdWallpaper_extraLight);

        for ( size_t i = 0; i < pPoly->numVertices; ++i ) {
            rdVector_Copy4(&pPoly->aVertIntensities[i], &rdWallpaper_extraLight);
        }

        LPD3DTLVERTEX pCurVert = pPoly->aVertices;
        switch ( matNum )
        {
            case 0u: // left top
                pCurVert->sx  = 0.0f;
                pCurVert->sy  = 0.0f;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5; // 0.00015259022
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 0.0f;
                pCurVert->tv  = 0.0f;
                ++pCurVert;

                pCurVert->sx  = 256.0f * swidth;
                pCurVert->sy  = 0.0f;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 1.0f;
                pCurVert->tv  = 0.0f;
                ++pCurVert;

                pCurVert->sx  = 256.0f * swidth;
                pCurVert->sy  = 256.0f * sheight;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 1.0f;
                pCurVert->tv  = 1.0f;
                ++pCurVert;

                pCurVert->sx  = 0.0f;
                pCurVert->sy  = 256.0f * sheight;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 0.0f;
                pCurVert->tv  = 1.0f;
                break;

            case 1u: // top middle
                pCurVert->sx  = 256.0f * swidth;
                pCurVert->sy  = 0.0f;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 0.0f;
                pCurVert->tv  = 0.0f;
                ++pCurVert;

                pCurVert->sx  = 512.0f * swidth;
                pCurVert->sy  = 0.0f;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 1.0f;
                pCurVert->tv  = 0.0f;
                ++pCurVert;

                pCurVert->sx  = 512.0f * swidth;
                pCurVert->sy  = 256.0f * sheight;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 1.0f;
                pCurVert->tv  = 1.0f;
                ++pCurVert;

                pCurVert->sx  = 256.0f * swidth;
                pCurVert->sy  = 256.0f * sheight;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 0.0f;
                pCurVert->tv  = 1.0f;
                break;

            case 2u: // top right
                pCurVert->sx  = 512.0f * swidth;
                pCurVert->sy  = 0.0f;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 0.0f;
                pCurVert->tv  = 0.0f;
                ++pCurVert;

                pCurVert->sx  = 640.0f * swidth;
                pCurVert->sy  = 0.0f;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 1.0f;
                pCurVert->tv  = 0.0f;
                ++pCurVert;

                pCurVert->sx  = 640.0f * swidth;
                pCurVert->sy  = 256.0f * sheight;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 1.0f;
                pCurVert->tv  = 1.0f;
                ++pCurVert;

                pCurVert->sx  = 512.0f * swidth;
                pCurVert->sy  = 256.0f * sheight;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 0.0f;
                pCurVert->tv  = 1.0f;
                break;

            case 3u:// bottom left
                pCurVert->sx  = 0.0f;
                pCurVert->sy  = 256.0f * sheight;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 0.0f;
                pCurVert->tv  = 0.0f;
                ++pCurVert;

                pCurVert->sx  = 256.0f * swidth;
                pCurVert->sy  = 256.0f * sheight;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 1.0f;
                pCurVert->tv  = 0.0f;
                ++pCurVert;

                pCurVert->sx  = 256.0f * swidth;
                pCurVert->sy  = 480.0f * sheight;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 1.0f;
                pCurVert->tv  = 0.875f;
                ++pCurVert;

                pCurVert->sx  = 0.0f;
                pCurVert->sy  = 480.0f * sheight;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 0.0f;
                pCurVert->tv  = 0.875f; // (480 - 256) / 256
                break;

            case 4u: // bottom middle
                pCurVert->sx  = 256.0f * swidth;
                pCurVert->sy  = 256.0f * sheight;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 0.0f;
                pCurVert->tv  = 0.0f;
                ++pCurVert;

                pCurVert->sx  = 512.0f * swidth;
                pCurVert->sy  = 256.0f * sheight;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 1.0f;
                pCurVert->tv  = 0.0f;
                ++pCurVert;

                pCurVert->sx  = 512.0f * swidth;
                pCurVert->sy  = 480.0f * sheight;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 1.0f;
                pCurVert->tv  = 0.875f; // (480 - 256) / 256
                ++pCurVert;

                pCurVert->sx  = 256.0f * swidth;
                pCurVert->sy  = 480.0f * sheight;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 0.0f;
                pCurVert->tv  = 0.875f; // (480 - 256) / 256
                break;

            case 5u: // bottom right
                pCurVert->sx  = 512.0f * swidth;
                pCurVert->sy  = 256.0f * sheight;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 0.0f;
                pCurVert->tv  = 0.0f;
                ++pCurVert;

                pCurVert->sx  = 640.0f * swidth;
                pCurVert->sy  = 256.0f * sheight;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 1.0f;
                pCurVert->tv  = 0.0f;
                ++pCurVert;

                pCurVert->sx  = 640.0f * swidth;
                pCurVert->sy  = 480.0f * sheight;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 1.0f;
                pCurVert->tv  = 0.875f; // (480 - 256) / 256
                ++pCurVert;

                pCurVert->sx  = 512.0f * swidth;
                pCurVert->sy  = 480.0f * sheight;
                pCurVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
                pCurVert->tu  = 0.0f;
                pCurVert->tv  = 0.875f; // (480 - 256) / 256
                break;

            default:
                break;
        }

        pPoly->pMaterial = &pWallpaper->aMaterials[matNum];
        pPoly->matCelNum = pWallpaper->aMatCelNums[matNum];
        rdCache_AddProcFace(4u);
    }
}

rdWallLine* J3DAPI rdWallpaper_NewWallLine(float startX, float startY, float endX, float endY, const rdVector4* pColor)
{
    rdWallLine* pWallLine;

    pWallLine = (rdWallLine*)STDMALLOC(sizeof(rdWallLine));
    if ( !pWallLine )
    {
        RDLOG_ERROR("Error allocating memory for WallLine.\n");
        return NULL; // Fixed: The original logged the error and then dereferenced the NULL pointer.
    }
    memset(pWallLine, 0, sizeof(rdWallLine));

    pWallLine->pMaterial = (rdMaterial*)STDMALLOC(sizeof(rdMaterial));
    if ( !pWallLine->pMaterial )
    {
        stdMemory_Free(pWallLine);
        return NULL;
    }
    memset(pWallLine->pMaterial, 0, sizeof(rdMaterial));

    if ( rdMaterial_LoadEntry("mat\\gen_4red.mat", pWallLine->pMaterial) )
    {
        if ( pWallLine->pMaterial )
        {
            rdMaterial_FreeEntry(pWallLine->pMaterial);
            stdMemory_Free(pWallLine->pMaterial);
            pWallLine->pMaterial = NULL;
        }

        stdMemory_Free(pWallLine);
        return NULL;
    }

    pWallLine->startX = startX;
    pWallLine->startY = startY;
    pWallLine->deltaX = endX - startX;
    pWallLine->deltaY = endY - startY;
    rdVector_Copy4(&pWallLine->color, pColor);
    return pWallLine;
}

void J3DAPI rdWallpaper_FreeWallLine(rdWallLine* pWallLine)
{
    if ( pWallLine )
    {
        if ( pWallLine->pMaterial )
        {
            rdMaterial_FreeEntry(pWallLine->pMaterial);
            stdMemory_Free(pWallLine->pMaterial);
            pWallLine->pMaterial = NULL;
        }

        stdMemory_Free(pWallLine);
    }
}

void J3DAPI rdWallpaper_DrawWallLine(const rdWallLine* pLine, float progress)
{
    rdCacheProcEntry* pPoly = rdCache_GetAlphaProcEntry();
    if ( !pPoly )
    {
        // Error getting proc entry
        return;
    }

    pPoly->flags        = RD_FF_TEX_TRANSLUCENT;
    pPoly->lightingMode = RD_LIGHTING_DIFFUSE;
    pPoly->numVertices  = 4;
    rdVector_Copy4(&pPoly->extraLight, &pLine->color);

    for ( size_t i = 0; i < pPoly->numVertices; ++i ) {
        rdVector_Copy4(&pPoly->aVertIntensities[i], &pLine->color);
    }

    uint32_t width, height;
    stdDisplay_GetBackBufferSize(&width, &height);

    float swidth    = (float)width / RD_REF_WIDTH;
    float sheight   = (float)height / RD_REF_HEIGHT;
    float thickness = RDWALLPAPER_LINE_THICKNESS * swidth;

    // Calculate end point based on progress
    float endY = 0.0f, endX = ceilf(pLine->deltaX * progress / 100.0f);

    // Changed: Added handing of case for when pLine->deltaY == 0, i.e.: horizontal line
    float deltaRatio = 0.0; // Horizontal line case
    if ( pLine->deltaY != 0.0f )
    {
        // Non-horizontal line calculations
        double lineAspect = pLine->deltaY / pLine->deltaX;
        double startY = pLine->startY - lineAspect * pLine->startX;

        if ( endX == 0.0f ) {
            endY = pLine->deltaY * progress / 100.0f;
        }
        else {
            endY = (float)((pLine->startX + endX) * lineAspect + startY - pLine->startY);
        }

        deltaRatio = pLine->deltaX / pLine->deltaY;
    }

    double offsetY = pLine->startY - deltaRatio * pLine->startX;
    double coeff1  = 2.0 * -pLine->startX;
    double coeff2  = 2.0 * deltaRatio * (offsetY - pLine->startY);

    double a = pow(deltaRatio, 2.0) + 1.0f;
    double b = coeff1 + coeff2;
    double c = pow(pLine->startX, 2.0) + pow(offsetY - pLine->startY, 2.0) - pow(thickness, 2.0);

    double discriminant = sqrt(b * b - 4.0 * a * c);  // Calculate discriminant
    if ( discriminant >= 0.0 ) // 1 or 2 real roots
    {
        double denominator = 2.0 * a;

        // First root
        double root = (-b + discriminant) / denominator;
        double test = a * root * root + b * root + c;
        if ( test >= 0.001 )
        {
            // Second root
            root = (-b - discriminant) / denominator;
            test = a * root * root + b * root + c;
            if ( test >= 0.001 )
            {
                // We couldn't find valid root so we can't draw a line
                return;
            }
        }

        // Final position calculations based on the valid root
        // Changed: Added handing of case for when pLine->deltaY == 0, i.e.: horizontal line
        float deltaX = 0.0f, deltaY = thickness;
        if ( pLine->deltaY != 0.0f )
        {
            deltaX = (float)root;
            deltaY = deltaRatio * deltaX + (float)offsetY;
            deltaX -= pLine->startX;
            deltaY -= pLine->startY;
        }

        // Set vertices positions for drawing the line
        LPD3DTLVERTEX pCurVert = pPoly->aVertices;

        // top left
        pCurVert->sx  = pLine->startX * swidth + deltaX;
        pCurVert->sy  = pLine->startY * sheight - deltaY;
        pCurVert->sz  = 0.0f;
        pCurVert->rhw = 0.00001f;
        pCurVert->tu  = 1.0f;
        pCurVert->tv  = 0.0f;
        ++pCurVert;

        // top right
        pCurVert->sx  = (pLine->startX + endX) * swidth + deltaX;
        pCurVert->sy  = (pLine->startY + endY) * sheight - deltaY;
        pCurVert->sz  = 0.0f;
        pCurVert->rhw = 0.00001f;
        pCurVert->tu  = 1.0f;
        pCurVert->tv  = 1.0f;
        ++pCurVert;

        // bottom right
        pCurVert->sx  = (pLine->startX + endX) * swidth - deltaX;
        pCurVert->sy  = (pLine->startY + endY) * sheight + deltaY;
        pCurVert->sz  = 0.0f;
        pCurVert->rhw = 0.00001f;
        pCurVert->tu  = 0.0f;
        pCurVert->tv  = 1.0f;
        ++pCurVert;

        // bottom left
        pCurVert->sx  = pLine->startX * swidth - deltaX;
        pCurVert->sy  = pLine->startY * sheight + deltaY;
        pCurVert->sz  = 0.0f;
        pCurVert->rhw = 0.00001f;
        pCurVert->tu  = 0.0f;
        pCurVert->tv  = 0.0f;

        pPoly->pMaterial = pLine->pMaterial;
        pPoly->matCelNum = 0;
        rdCache_AddAlphaProcFace(4u);
    }
}
