#include "std.h"
#include "stdColor.h"

#include <j3dcore/j3dhook.h>
#include <std/RTI/symbols.h>

void stdColor_InstallHooks(void)
{
    J3D_HOOKFUNC(stdColor_ColorConvertOneRow);
}

void stdColor_ResetGlobals(void)
{}

void J3DAPI stdColor_RGBtoHSV(float r, float g, float b, float* hue, float* saturation, float* value)
{
    float cmin;
    float v8;
    float cmax;
    float v12;
    float v13;
    float v14;
    float r1;
    float v16;
    float g1;
    float v18;

    r1 = r * 0.0039215689f;
    g1 = g * 0.0039215689f;
    v12 = b * 0.0039215689f;
    if ( r1 <= (double)g1 )
    {
        cmax = g * 0.0039215689f;
    }
    else
    {
        cmax = r * 0.0039215689f;
    }

    if ( cmax <= (double)v12 )
    {
        cmax = b * 0.0039215689f;
    }

    else if ( r1 <= (double)g1 )
    {
        cmax = g * 0.0039215689f;
    }
    else
    {
        cmax = r * 0.0039215689f;
    }

    if ( r1 >= (double)g1 )
    {
        v8 = g * 0.0039215689f;
    }
    else
    {
        v8 = r * 0.0039215689f;
    }

    if ( v8 >= (double)v12 )
    {
        cmin = b * 0.0039215689f;
    }

    else if ( r1 >= (double)g1 )
    {
        cmin = g * 0.0039215689f;
    }
    else
    {
        cmin = r * 0.0039215689f;
    }

    v13 = cmax - cmin;
    *value = cmax;
    if ( cmax == 0.0f )
    {
        *saturation = 0.0f;
    }
    else
    {
        *saturation = v13 / cmax;
    }

    if ( *saturation == 0.0f )
    {
        *hue = 0.0f;
    }
    else
    {
        v14 = (cmax - r1) / v13;
        v18 = (cmax - g1) / v13;
        v16 = (cmax - v12) / v13;
        if ( r1 == cmax )
        {
            *hue = v16 - v18;
        }

        else if ( g1 == cmax )
        {
            *hue = v14 + 2.0f - v16;
        }

        else if ( v12 == cmax )
        {
            *hue = v18 + 4.0f - v14;
        }

        *hue = *hue * 60.0f;
        if ( *hue < 0.0f )
        {
            *hue = *hue + 360.0f;
        }
    }
}

void J3DAPI stdColor_HSVtoRGB(float hue, float saturation, float value, float* r, float* g, float* b)
{
    unsigned int i;
    float ff;
    float v9;
    float v10;
    float v11;
    float p;
    float hh;

    if ( saturation == 0.0f )
    {
        v9 = value * 255.0f;
        *r = v9;
        *g = v9;
        *b = v9;
    }
    else
    {
        if ( hue == 360.0f )
        {
            hue = 0.0f;
        }

        hh = hue / 60.0f;
        i = (int32_t)hh;
        ff = hh - (float)i;
        p = (1.0f - saturation) * value;
        v11 = (1.0f - saturation * ff) * value;
        v10 = (1.0f - (1.0f - ff) * saturation) * value;

        switch ( i )
        {
            case 0u:
                *r = value * 255.0f;
                *g = v10 * 255.0f;
                *b = p * 255.0f;
                break;

            case 1u:
                *r = v11 * 255.0f;
                *g = value * 255.0f;
                *b = p * 255.0f;
                break;

            case 2u:
                *r = p * 255.0f;
                *g = value * 255.0f;
                *b = v10 * 255.0f;
                break;

            case 3u:
                *r = p * 255.0f;
                *g = v11 * 255.0f;
                *b = value * 255.0f;
                break;

            case 4u:
                *r = v10 * 255.0f;
                *g = p * 255.0f;
                *b = value * 255.0f;
                break;

            case 5u:
                *r = value * 255.0f;
                *g = p * 255.0f;
                *b = v11 * 255.0f;
                break;

            default:
                return;
        }
    }
}

uint32_t J3DAPI stdColor_ScaleColorComponent(uint32_t cc, int srcBPP, int deltaBPP)
{
    if ( deltaBPP <= 0 ) // Upscale
    {
        // Fixed: Fixed scaling to get correct value from lower bpp.
        //        Original was calculated only "cc >> -deltaBPP", which resulted in dimmer colors.
        // Fixed: Repeat source bits across the destination width, including 1-, 2-, and 3-bit channels.
        int destBPP     = srcBPP - deltaBPP;
        uint32_t result = 0;
        for ( int remainingBPP = destBPP; remainingBPP > 0; remainingBPP -= srcBPP )
        {
            int copyBPP = remainingBPP < srcBPP ? remainingBPP : srcBPP;
            result = (result << copyBPP) | (cc >> (srcBPP - copyBPP));
        }

        return result;
    }

    // Downscale
    return cc >> deltaBPP;
}

void J3DAPI stdColor_ColorConvertOneRow(uint8_t* pDestRow, const ColorInfo* pDestCI, const uint8_t* pSrcRow, const ColorInfo* pSrcCI, int width, int bColorKey, LPDDCOLORKEY pColorKey)
{
    unsigned int redMask       = 0xFFFFFFFF >> (32 - (pSrcCI->redBPP & 0xFF));
    unsigned int greenMask     = 0xFFFFFFFF >> (32 - (pSrcCI->greenBPP & 0xFF));
    unsigned int blueMask      = 0xFFFFFFFF >> (32 - (pSrcCI->blueBPP & 0xFF));
    unsigned int alphaMask     = 0; // Added: Zero init
    unsigned int maxAlphaValue = 0; // Added: Zero init

    if ( pSrcCI->alphaBPP )
    {
        alphaMask = 0xFFFFFFFF >> (32 - (pSrcCI->alphaBPP & 0xFF));
        if ( (255 >> pSrcCI->alphaPosShiftRight) / 2 <= 1 )
        {
            maxAlphaValue = 1;
        }
        else
        {
            maxAlphaValue = (255 >> pSrcCI->alphaPosShiftRight) / 2;
        }
    }

    int redDelta   = pSrcCI->redBPP - pDestCI->redBPP;
    int greenDelta = pSrcCI->greenBPP - pDestCI->greenBPP;
    int blueDelta  = pSrcCI->blueBPP - pDestCI->blueBPP;
    int alphaDelta = 0;
    if ( pSrcCI->alphaBPP )
    {
        alphaDelta = pSrcCI->alphaBPP - pDestCI->alphaBPP;
    }

    for ( int i = 0; i < width; ++i )
    {
        unsigned int pixel = 0;
        switch ( pSrcCI->bpp )
        {
            case 8:
                pixel = *pSrcRow;
                break;

            case 16:
                pixel = *(uint16_t*)pSrcRow;
                break;

            case 24:
                // Fixed: Swap first and third byte to correctly extract encoded pixel in little-endian format (BGR/RGB)
                //        Original: pixel = pSrcRow[2] | (pSrcRow[1] << 8) | (*pSrcRow << 16);
                pixel = pSrcRow[0] | (pSrcRow[1] << 8) | (pSrcRow[2] << 16);
                break;

            case 32:
                pixel = *(uint32_t*)pSrcRow;
                break;

            default:
                STDLOG_FATAL("Unsupported pixel depth. Only 8, 16, 24, &32 bits per pixel supported at the moment.");
                break;
        }

        uint32_t r = redMask & (pixel >> pSrcCI->redPosShift);
        uint32_t g = greenMask & (pixel >> pSrcCI->greenPosShift);
        uint32_t b = blueMask & (pixel >> pSrcCI->bluePosShift);
        uint32_t a = 0; // Added: Zero init
        if ( pSrcCI->alphaBPP )
        {
            a = alphaMask & (pixel >> pSrcCI->alphaPosShift);
        }

        r = stdColor_ScaleColorComponent(r, pSrcCI->redBPP, redDelta);
        g = stdColor_ScaleColorComponent(g, pSrcCI->greenBPP, greenDelta);
        b = stdColor_ScaleColorComponent(b, pSrcCI->blueBPP, blueDelta);

        pixel = (b << pDestCI->bluePosShift) | (g << pDestCI->greenPosShift) | (r << pDestCI->redPosShift);
        if ( pSrcCI->alphaBPP )
        {
            J3D_UNUSED(bColorKey);
            J3D_UNUSED(pColorKey);

        #ifdef J3D_COLORKEYSUPPORTED
            if ( bColorKey )
            {
                if ( a < maxAlphaValue )
                {
                    pixel = pColorKey->dwColorSpaceLowValue;
                }
            }
            else
            #endif
            {
                a = stdColor_ScaleColorComponent(a, pSrcCI->alphaBPP, alphaDelta);
                pixel |= a << pDestCI->alphaPosShift;
            }
        }

        switch ( pDestCI->bpp )
        {
            case 8:
                *pDestRow = (pixel & 0xff);
                break;

            case 16:
                *(uint16_t*)pDestRow = (uint16_t)pixel;
                break;

            case 24:
                // Fixed: Swap first and third byte to correctly write encoded pixel in little-endian format (BGR/RGB)
                //        Original: 
                //             pDestRow[0] = (pixel >> 16) & 0xFF;
                //             pDestRow[1] = (pixel >> 8) & 0xFF;
                //             pDestRow[2] = pixel & 0xFF;
                pDestRow[0] = pixel & 0xFF;
                pDestRow[1] = (pixel >> 8) & 0xFF;
                pDestRow[2] = (pixel >> 16) & 0xFF;
                break;
            case 32:
                *(uint32_t*)pDestRow = pixel;
                break;

            default:
                STDLOG_FATAL("Unsupported pixel depth. Only 8, 16, 24, &32 bits per pixel supported at the moment.");
                break;
        }

        pSrcRow += (unsigned int)pSrcCI->bpp >> 3;
        pDestRow += (unsigned int)pDestCI->bpp >> 3;
    }
}

int J3DAPI stdColor_ColorConvertOnePixel(ColorInfo* pDestCI, int pixl, ColorInfo* pSrcCI, int bColorKey, LPDDCOLORKEY pColorKey)
{
    int cpixel = 0;
    stdColor_ColorConvertOneRow((uint8_t*)&cpixel, pDestCI, (const uint8_t*)&pixl, pSrcCI, 1, bColorKey, pColorKey);
    return cpixel;
}

uint32_t stdColor_EncodeRGB(const ColorInfo* ci, uint8_t r, uint8_t g, uint8_t b)
{
    STD_ASSERT(ci);

    // Scale color components according to bits per component
    uint32_t redScaled   = r;
    uint32_t greenScaled = g;
    uint32_t blueScaled  = b;

    // Adjust for component bit depth if needed
    if ( ci->redBPP < 8 )
    {
        redScaled = redScaled >> (8 - ci->redBPP);
    }
    if ( ci->greenBPP < 8 )
    {
        greenScaled = greenScaled >> (8 - ci->greenBPP);
    }
    if ( ci->blueBPP < 8 )
    {
        blueScaled = blueScaled >> (8 - ci->blueBPP);
    }

    // Shift components to their positions and combine
    uint32_t encoded = 0;
    if ( ci->redPosShift >= 0 )
    {
        encoded |= (redScaled << ci->redPosShift);
    }
    else
    {
        encoded |= (redScaled >> ci->redPosShiftRight);
    }

    if ( ci->greenPosShift >= 0 )
    {
        encoded |= (greenScaled << ci->greenPosShift);
    }
    else
    {
        encoded |= (greenScaled >> ci->greenPosShiftRight);
    }

    if ( ci->bluePosShift >= 0 )
    {
        encoded |= (blueScaled << ci->bluePosShift);
    }
    else
    {
        encoded |= (blueScaled >> ci->bluePosShiftRight);
    }

    return encoded;
}

uint32_t stdColor_EncodeRGBA(const ColorInfo* ci, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    STD_ASSERT(ci);

    // Start with RGB encoding
    uint32_t encoded = stdColor_EncodeRGB(ci, r, g, b);

    // Add alpha if supported by the format
    if ( ci->alphaBPP > 0 )
    {
        uint32_t alphaScaled = a;

        // Adjust for alpha bit depth if needed
        if ( ci->alphaBPP < 8 )
        {
            alphaScaled = alphaScaled >> (8 - ci->alphaBPP);
        }

        // Shift alpha to its position and combine
        if ( ci->alphaPosShift >= 0 )
        {
            encoded |= (alphaScaled << ci->alphaPosShift);
        }
        else
        {
            encoded |= (alphaScaled >> ci->alphaPosShiftRight);
        }
    }

    return encoded;
}

void stdColor_DecodeRGB(uint32_t encoded, const ColorInfo* ci, uint8_t* r, uint8_t* g, uint8_t* b)
{
    STD_ASSERT(ci && r && g && b);

    // Create masks based on bit depths
    uint32_t redMask   = ((1 << ci->redBPP) - 1);
    uint32_t greenMask = ((1 << ci->greenBPP) - 1);
    uint32_t blueMask  = ((1 << ci->blueBPP) - 1);

    // Extract components using shifts and masks
    uint32_t redVal;
    if ( ci->redPosShift >= 0 )
    {
        redVal = (encoded >> ci->redPosShift) & redMask;
    }
    else
    {
        redVal = (encoded << ci->redPosShiftRight) & redMask;
    }

    uint32_t greenVal;
    if ( ci->greenPosShift >= 0 )
    {
        greenVal = (encoded >> ci->greenPosShift) & greenMask;
    }
    else
    {
        greenVal = (encoded << ci->greenPosShiftRight) & greenMask;
    }

    uint32_t blueVal;
    if ( ci->bluePosShift >= 0 )
    {
        blueVal = (encoded >> ci->bluePosShift) & blueMask;
    }
    else
    {
        blueVal = (encoded << ci->bluePosShiftRight) & blueMask;
    }

    // Scale back to 8-bit range if needed
    if ( ci->redBPP < 8 )
    {
        // Fixed: Use the shared bit-replication path so sub-4-bit channels cannot shift by a negative count.
        redVal = stdColor_ScaleColorComponent(redVal, ci->redBPP, (int)ci->redBPP - 8);
    }

    if ( ci->greenBPP < 8 )
    {
        greenVal = stdColor_ScaleColorComponent(greenVal, ci->greenBPP, (int)ci->greenBPP - 8);
    }

    if ( ci->blueBPP < 8 )
    {
        blueVal = stdColor_ScaleColorComponent(blueVal, ci->blueBPP, (int)ci->blueBPP - 8);
    }

    // Store the results
    *r = (uint8_t)redVal;
    *g = (uint8_t)greenVal;
    *b = (uint8_t)blueVal;
}

void stdColor_DecodeRGBA(uint32_t encoded, const ColorInfo* ci, uint8_t* r, uint8_t* g, uint8_t* b, uint8_t* a)
{
    STD_ASSERT(ci && r && g && b && a);

    // First decode the RGB components
    stdColor_DecodeRGB(encoded, ci, r, g, b);

    // Then handle alpha if the format supports it
    if ( ci->alphaBPP > 0 )
    {
        uint32_t alphaMask = ((1 << ci->alphaBPP) - 1);
        uint32_t alphaVal;

        // Extract alpha component
        if ( ci->alphaPosShift >= 0 )
        {
            alphaVal = (encoded >> ci->alphaPosShift) & alphaMask;
        }
        else
        {
            alphaVal = (encoded << ci->alphaPosShiftRight) & alphaMask;
        }

        // Scale back to 8-bit range if needed
        if ( ci->alphaBPP < 8 )
        {
            // Fixed: Expand low-bit alpha through the same defined bit-replication path as RGB.
            alphaVal = stdColor_ScaleColorComponent(alphaVal, ci->alphaBPP, (int)ci->alphaBPP - 8);
        }

        *a = (uint8_t)alphaVal;
    }
    else
    {
// If format doesn't support alpha, set to fully opaque
        *a = 255;
    }
}

uint32_t stdColor_Recode(uint32_t encoded, const ColorInfo* pSrcCI, const ColorInfo* pDestCI)
{
    unsigned int redMask       = 0xFFFFFFFF >> (32 - (pSrcCI->redBPP & 0xFF));
    unsigned int greenMask     = 0xFFFFFFFF >> (32 - (pSrcCI->greenBPP & 0xFF));
    unsigned int blueMask      = 0xFFFFFFFF >> (32 - (pSrcCI->blueBPP & 0xFF));
    unsigned int alphaMask     = 0;
    unsigned int maxAlphaValue = 0;

    if ( pSrcCI->alphaBPP )
    {
        alphaMask = 0xFFFFFFFF >> (32 - (pSrcCI->alphaBPP & 0xFF));
        if ( (255 >> pSrcCI->alphaPosShiftRight) / 2 <= 1 )
        {
            maxAlphaValue = 1;
        }
        else
        {
            maxAlphaValue = (255 >> pSrcCI->alphaPosShiftRight) / 2;
        }
    }

    int redDelta   = pSrcCI->redBPP - pDestCI->redBPP;
    int greenDelta = pSrcCI->greenBPP - pDestCI->greenBPP;
    int blueDelta  = pSrcCI->blueBPP - pDestCI->blueBPP;
    int alphaDelta = 0;
    if ( pSrcCI->alphaBPP )
    {
        alphaDelta = pSrcCI->alphaBPP - pDestCI->alphaBPP;
    }

    // Decode
    uint32_t r = redMask & (encoded >> pSrcCI->redPosShift);
    uint32_t g = greenMask & (encoded >> pSrcCI->greenPosShift);
    uint32_t b = blueMask & (encoded >> pSrcCI->bluePosShift);
    uint32_t a = 0;
    if ( pSrcCI->alphaBPP )
    {
        a = alphaMask & (encoded >> pSrcCI->alphaPosShift);
    }

    // Encode
    r = stdColor_ScaleColorComponent(r, pSrcCI->redBPP, redDelta);
    g = stdColor_ScaleColorComponent(g, pSrcCI->greenBPP, greenDelta);
    b = stdColor_ScaleColorComponent(b, pSrcCI->blueBPP, blueDelta);

    encoded = (b << pDestCI->bluePosShift) | (g << pDestCI->greenPosShift) | (r << pDestCI->redPosShift);
    if ( pSrcCI->alphaBPP )
    {
        a = stdColor_ScaleColorComponent(a, pSrcCI->alphaBPP, alphaDelta);
        encoded |= a << pDestCI->alphaPosShift;
    }

    return encoded;
}
