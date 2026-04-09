#include "rdPrimit2.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Engine/rdClip.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/RTI/symbols.h>

#include <std/General/stdColor.h>
#include <std/General/stdMath.h>
#include <std/Win95/std3D.h>
#include <std/Win95/stdDisplay.h>

#ifdef J3D_OPENGL
#include "rdroid/Raster/rdCache.h"
#endif

D3DTLVERTEX rdPrimit2_aD3DVertices[2];

void rdPrimit2_InstallHooks(void)
{
    J3D_HOOKFUNC(rdPrimit2_DrawClippedLine);
    J3D_HOOKFUNC(rdPrimit2_Reset);
    J3D_HOOKFUNC(rdPrimit2_DrawClippedLine2);
    J3D_HOOKFUNC(rdPrimit2_DrawClippedCircle);
    J3D_HOOKFUNC(rdPrimit2_DrawClippedCircle2);
}

void rdPrimit2_ResetGlobals(void)
{}

// Added: Helper function to plot a pixel based on bit depth
void J3DAPI rdPrimit2_DrawPixel(tVBuffer* pVBuffer, size_t index, uint32_t pixelColor)
{
    switch ( pVBuffer->rasterInfo.colorInfo.bpp ) {
        case 16:
            ((uint16_t*)pVBuffer->pPixels)[index] = (uint16_t)pixelColor;
            break;
        case 24: // Added: Fixes drawing to 24 bpp buffer, before was 16 bpp only
        {
            uint8_t* pixelAddr = (uint8_t*)pVBuffer->pPixels + index * 3;
            pixelAddr[0] = pixelColor & 0xFF;          // Blue
            pixelAddr[1] = (pixelColor >> 8) & 0xFF;   // Green
            pixelAddr[2] = (pixelColor >> 16) & 0xFF;  // Red
            break;
        }
        case 32: // Added: Fixes drawing to 32 bpp buffer, before was 16 bpp only
            ((uint32_t*)pVBuffer->pPixels)[index] = pixelColor;
            break;
    }
}

int J3DAPI rdPrimit2_DrawLine(rdCanvas* pCanvas, int x1, int y1, int x2, int y2, uint32_t color, uint32_t pattern)
{
    // Validate canvas buffer
    if ( !pCanvas || !pCanvas->pVBuffer ) // Fixed: Make sure we have everything initialized
    {
        return 0;
    }

    // Recode color to raster color format
    color = stdColor_Recode(color, &stdColor_cfRGB888, &pCanvas->pVBuffer->rasterInfo.colorInfo);

    // Calculate line parameters
    int x = x1;
    int y = y1;

    int dx = x2 - x1;
    int dy = y2 - y1;

    // Determine direction increments
    int xIncrement = (dx <= 0) ? -1 : 1;
    int yIncrement = (dy <= 0) ? -1 : 1;

    // Adjust for Bresenham algorithm
    if ( xIncrement < 0 )
    {
        dy = y1 - y2;
    }

    if ( yIncrement > 0 )
    {
        dx = x1 - x2;
    }

    // Get bit depth and determine buffer parameters
    uint32_t bpp = pCanvas->pVBuffer->rasterInfo.colorInfo.bpp;

    // Handle 8-bit depth separately (direct pixel access)
    if ( bpp == 8 )
    {
        // Draw first pixel if needed
        if ( (pattern & 0x80000000) != 0 )
        {
            pCanvas->pVBuffer->pPixels[pCanvas->pVBuffer->rasterInfo.rowSize * y + x] = color;
        }

        // Draw the line
        int error = 0;
        uint32_t mask = 0x80000000;
        while ( x != x2 || y != y2 )
        {
            // Rotate pattern mask
            mask >>= 1;
            if ( !mask ) mask = 0x80000000;

            // Determine next pixel
            int absError = abs(dy + error);
            if ( absError >= abs(dx + error) )
            {
                y += yIncrement;
                error += dx;
            }
            else
            {
                x += xIncrement;
                error += dy;
            }

            // Draw if pattern bit is set
            if ( (mask & pattern) != 0 )
            {
                pCanvas->pVBuffer->pPixels[pCanvas->pVBuffer->rasterInfo.rowSize * y + x] = color;
            }
        }
    }
    // Handle 16/32 bpp with the rdPrimit2_DrawPixel function
    else
    {
        // Lock buffer if needed
        bool bVBLocked = false;
        if ( pCanvas->pVBuffer->type == VBUFFER_HARDWARE )
        {
            bVBLocked = true;
            stdDisplay_VBufferLock(pCanvas->pVBuffer);
        }

        // Draw first pixel if needed
        if ( (pattern & 0x80000000) != 0 )
        {
            rdPrimit2_DrawPixel(pCanvas->pVBuffer, pCanvas->pVBuffer->rasterInfo.rowWidth * y + x, color); // Fixed: This function fixes drawing to 24 bpp & 32 bpp vbuffer; was buggy and always drew as 16 bpp vbuffer
        }

        // Draw the line
        int error = 0;
        uint32_t mask = 0x80000000;
        while ( x != x2 || y != y2 )
        {
            // Rotate pattern mask
            mask >>= 1;
            if ( mask == 0 ) mask = 0x80000000;

            // Determine next pixel
            int absError = abs(dy + error);
            if ( absError >= abs(dx + error) )
            {
                y += yIncrement;
                error += dx;
            }
            else
            {
                x += xIncrement;
                error += dy;
            }

            // Draw if pattern bit is set
            if ( (mask & pattern) != 0 )
            {
                rdPrimit2_DrawPixel(pCanvas->pVBuffer, pCanvas->pVBuffer->rasterInfo.rowWidth * y + x, color); // Fixed: This function fixes drawing to 24 bpp & 32 bpp vbuffer; was buggy and always drew as 16 bpp vbuffer
            }
        }

        // Unlock buffer if we locked it
        if ( bVBLocked )
        {
            stdDisplay_VBufferUnlock(pCanvas->pVBuffer);
        }
    }

    return 1;
}

int J3DAPI rdPrimit2_DrawClippedLine(rdCanvas* pCanvas, int x1, int y1, int x2, int y2, uint32_t color, uint32_t pattern)
{
    // Clip line to canvas boundaries
    if ( !rdClip_Line2(pCanvas, &x1, &y1, &x2, &y2) )
    {
        return 0;
    }

    // Altered: Refactored to call rdPrimit2_DrawLine; was implemented same code as rdPrimit2_DrawLine
    return rdPrimit2_DrawLine(pCanvas, x1, y1, x2, y2, color, pattern);
}

void rdPrimit2_Reset(void)
{
    std3D_SetWireframeRenderState();
    memset(rdPrimit2_aD3DVertices, 0, sizeof(rdPrimit2_aD3DVertices));
}

int J3DAPI rdPrimit2_DrawClippedLine2(float x1, float y1, float x2, float y2, uint32_t color)
{
    if ( !rdClip_Line2Ex(&x1, &y1, &x2, &y2) )
    {
        return 0;
    }

    rdPrimit2_aD3DVertices[0].sx    = x1;
    rdPrimit2_aD3DVertices[0].sy    = y1;
    rdPrimit2_aD3DVertices[0].sz    = RD_FIXEDPOINT_RHW_SCALE_X3; // Fixed: Assigned sz value, should fix scaling & depth layering which fixes drawing line in some cases
    rdPrimit2_aD3DVertices[0].rhw   = RD_FIXEDPOINT_RHW_SCALE_X3; // Fixed: Assigned rhw value, should fix scaling & depth layering which fixes drawing line in some cases
    rdPrimit2_aD3DVertices[0].color = color;

    rdPrimit2_aD3DVertices[1].sx  = x2;
    rdPrimit2_aD3DVertices[1].sy  = y2;
    rdPrimit2_aD3DVertices[1].sz  = RD_FIXEDPOINT_RHW_SCALE_X3; // Fixed: Assigned sz value, should fix scaling & depth layering which fixes drawing line in some cases
    rdPrimit2_aD3DVertices[1].rhw = RD_FIXEDPOINT_RHW_SCALE_X3; // Fixed: Assigned rhw value, should fix scaling & depth layering which fixes drawing line in some cases
    rdPrimit2_aD3DVertices[1].color = color;

#ifndef J3D_OPENGL
    std3D_DrawLineStrip(rdPrimit2_aD3DVertices, 2u);
#else
    rdCache_AddLineDrawCall(rdPrimit2_aD3DVertices, 2u);
#endif
    return 1;
}

void J3DAPI rdPrimit2_DrawClippedCircle(rdCanvas* pCanvas, int x, int y, float radius, float step, uint32_t color, uint32_t pattern)
{
    float angle;
    int v8;
    int y2;
    int x2;
    float cosval;
    int y1;
    int x1;
    float sinval;

    v8 = (int32_t)(radius + 0.5f);
    if ( v8 + x >= pCanvas->rect.left && x - v8 <= pCanvas->rect.right && v8 + y >= pCanvas->rect.top && y - v8 <= pCanvas->rect.bottom )
    {
        stdMath_SinCos(0.0f, &sinval, &cosval);
        x1 = x + (int32_t)(radius * cosval + 0.5f);
        y1 = y + (int32_t)(radius * sinval + 0.5f);
        for ( angle = step + 0.0f; angle <= 360.0f; angle = angle + step )
        {
            stdMath_SinCos(angle, &sinval, &cosval);
            x2 = x + (int32_t)(radius * cosval + 0.5f);
            y2 = y + (int32_t)(radius * sinval + 0.5f);

            rdPrimit2_DrawClippedLine(pCanvas, x1, y1, x2, y2, color, pattern);

            x1 = x2;
            y1 = y2;
        }
    }
}

void J3DAPI rdPrimit2_DrawClippedCircle2(float centerX, float centerY, float radius, float degrees, uint32_t color)
{
    float cosval, sinval;
    stdMath_SinCos(0.0f, &sinval, &cosval);

    float x1 = radius * cosval + centerX;
    float y1 = radius * sinval + centerY;

    for ( float angle = degrees + 0.0f; angle <= 360.0f; angle = angle + degrees )
    {
        stdMath_SinCos(angle, &sinval, &cosval);

        float x2 = radius * cosval + centerX;
        float y2 = radius * sinval + centerY;
        rdPrimit2_DrawClippedLine2(x1, y1, x2, y2, color);

        x1 = x2;
        y1 = y2;
    }
}

int J3DAPI rdPrimit2_DrawRectangle(rdCanvas* pCanvas, int x1, int y1, int x2, int y2, uint32_t color, uint32_t pattern)
{
    rdPrimit2_DrawClippedLine(pCanvas, x1, y1, x2, y1, color, pattern);
    rdPrimit2_DrawClippedLine(pCanvas, x2, y1, x2, y2, color, pattern);
    rdPrimit2_DrawClippedLine(pCanvas, x1, y1, x1, y2, color, pattern);
    return rdPrimit2_DrawClippedLine(pCanvas, x1, y2, x2, y2, color, pattern);
}

int J3DAPI rdPrimit2_DrawTriangle(rdCanvas* pCanvas, int x1, int y1, int x2, int y2, int a6, int a7, uint32_t color, uint32_t pattern)
{
    rdPrimit2_DrawClippedLine(pCanvas, x1, y1, x2, y2, color, pattern);
    rdPrimit2_DrawClippedLine(pCanvas, x2, y2, a6, a7, color, pattern);
    return rdPrimit2_DrawClippedLine(pCanvas, a6, a7, x1, y1, color, pattern);
}
