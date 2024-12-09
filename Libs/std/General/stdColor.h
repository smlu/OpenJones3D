#ifndef STD_STDCOLOR_H
#define STD_STDCOLOR_H
#include "std.h"
#include <j3dcore/j3d.h>
#include <std/types.h>
#include <std/RTI/addresses.h>

J3D_EXTERN_C_START

// TODO: Add stdColor_GammaCorrect


static const ColorInfo stdColor_cfRGB555 =
{
    .colorMode          = STDCOLOR_RGB,
    .bpp                = 16,
    .redBPP             = 5,
    .greenBPP           = 5,
    .blueBPP            = 5,
    .redPosShift        = 10,
    .greenPosShift      = 5,
    .bluePosShift       = 0,
    .redPosShiftRight   = 3,
    .greenPosShiftRight = 3,
    .bluePosShiftRight  = 3,
    .alphaBPP           = 0,
    .alphaPosShift      = 0,
    .alphaPosShiftRight = 0
};

static const ColorInfo stdColor_cfRGB565 =
{
    .colorMode          = STDCOLOR_RGB,
    .bpp                = 16,
    .redBPP             = 5,
    .greenBPP           = 6,
    .blueBPP            = 5,
    .redPosShift        = 11,
    .greenPosShift      = 5,
    .bluePosShift       = 0,
    .redPosShiftRight   = 3,
    .greenPosShiftRight = 2,
    .bluePosShiftRight  = 3,
    .alphaBPP           = 0,
    .alphaPosShift      = 0,
    .alphaPosShiftRight = 0
};

static const ColorInfo stdColor_cfBGR888 = {
    .colorMode          = STDCOLOR_RGB,
    .bpp                = 24,
    .redBPP             = 8,
    .greenBPP           = 8,
    .blueBPP            = 8,
    .redPosShift        = 0,
    .greenPosShift      = 8,
    .bluePosShift       = 16,
    .redPosShiftRight   = 0,
    .greenPosShiftRight = 0,
    .bluePosShiftRight  = 0,
    .alphaBPP           = 0,
    .alphaPosShift      = 0,
    .alphaPosShiftRight = 0
};

static const ColorInfo stdColor_cfRGB888 =
{
    .colorMode          = STDCOLOR_RGB,
    .bpp                = 24,
    .redBPP             = 8,
    .greenBPP           = 8,
    .blueBPP            = 8,
    .redPosShift        = 16,
    .greenPosShift      = 8,
    .bluePosShift       = 0,
    .redPosShiftRight   = 0,
    .greenPosShiftRight = 0,
    .bluePosShiftRight  = 0,
    .alphaBPP           = 0,
    .alphaPosShift      = 0,
    .alphaPosShiftRight = 0
};


static const ColorInfo stdColor_cfBGR8888 = {
    .colorMode          = STDCOLOR_RGB,
    .bpp                = 32,
    .redBPP             = 8,
    .greenBPP           = 8,
    .blueBPP            = 8,
    .redPosShift        = 0,
    .greenPosShift      = 8,
    .bluePosShift       = 16,
    .redPosShiftRight   = 0,
    .greenPosShiftRight = 0,
    .bluePosShiftRight  = 0,
    .alphaBPP           = 0,
    .alphaPosShift      = 0,
    .alphaPosShiftRight = 0
};

static const ColorInfo stdColor_cfRGB8888 =
{
    .colorMode          = STDCOLOR_RGB,
    .bpp                = 32,
    .redBPP             = 8,
    .greenBPP           = 8,
    .blueBPP            = 8,
    .redPosShift        = 16,
    .greenPosShift      = 8,
    .bluePosShift       = 0,
    .redPosShiftRight   = 0,
    .greenPosShiftRight = 0,
    .bluePosShiftRight  = 0,
    .alphaBPP           = 0,
    .alphaPosShift      = 0,
    .alphaPosShiftRight = 0
};

static const ColorInfo stdColor_cfABGR8888 =
{
    .colorMode          = STDCOLOR_RGBA,
    .bpp                = 32,
    .redBPP             = 8,
    .greenBPP           = 8,
    .blueBPP            = 8,
    .redPosShift        = 0,
    .greenPosShift      = 8,
    .bluePosShift       = 16,
    .redPosShiftRight   = 0,
    .greenPosShiftRight = 0,
    .bluePosShiftRight  = 0,
    .alphaBPP           = 8,
    .alphaPosShift      = 24,
    .alphaPosShiftRight = 0
};

static const ColorInfo stdColor_cfARGB8888 =
{
    .colorMode          = STDCOLOR_RGBA,
    .bpp                = 32,
    .redBPP             = 8,
    .greenBPP           = 8,
    .blueBPP            = 8,
    .redPosShift        = 16,
    .greenPosShift      = 8,
    .bluePosShift       = 0,
    .redPosShiftRight   = 0,
    .greenPosShiftRight = 0,
    .bluePosShiftRight  = 0,
    .alphaBPP           = 8,
    .alphaPosShift      = 24,
    .alphaPosShiftRight = 0
};

static const ColorInfo stdColor_cfRGBA8888 =
{
    .colorMode          = STDCOLOR_RGBA,
    .bpp                = 32,
    .redBPP             = 8,
    .greenBPP           = 8,
    .blueBPP            = 8,
    .redPosShift        = 24,
    .greenPosShift      = 16,
    .bluePosShift       = 8,
    .redPosShiftRight   = 0,
    .greenPosShiftRight = 0,
    .bluePosShiftRight  = 0,
    .alphaBPP           = 8,
    .alphaPosShift      = 0,
    .alphaPosShiftRight = 0
};

void J3DAPI stdColor_RGBtoHSV(float r, float g, float b, float* hue, float* saturation, float* value); // Added
void J3DAPI stdColor_HSVtoRGB(float hue, float saturation, float value, float* r, float* g, float* b); // Added
void J3DAPI stdColor_ColorConvertOneRow(uint8_t* pDestRow, const ColorInfo* pDestInfo, const uint8_t* pSrcRow, const ColorInfo* pSrcInfo, int width, int bColorKey, LPDDCOLORKEY pColorKey);
int J3DAPI stdColor_ColorConvertOnePixel(ColorInfo* pDestInfo, int pixl, ColorInfo* pSrcInfo, int bColorKey, LPDDCOLORKEY pColorKey); // Added

// Added
inline void stdColor_CalcColorBits(uint32_t bitmask, uint32_t* cbpp, int32_t* posShift, int32_t* posShiftRight)
{
    if ( !cbpp || !posShift || !posShiftRight ) {
        return;
    }

    int bits = 0;
    uint32_t curBitmask = bitmask;
    for ( ; (curBitmask & 1) == 0; curBitmask >>= 1 ) {
        ++bits;
    }
    *posShift = bits;
    *posShiftRight = stdCalcBitPos(255 / (bitmask >> bits));

    bits = 0;
    while ( (curBitmask & 1) != 0 )
    {
        ++bits;
        curBitmask >>= 1;
    }
    *cbpp = bits;
}

// Helper hooking functions
void stdColor_InstallHooks(void);
void stdColor_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // STD_STDCOLOR_H
