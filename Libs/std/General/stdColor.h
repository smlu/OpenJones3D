#ifndef STD_STDCOLOR_H
#define STD_STDCOLOR_H
#include "std.h"
#include <j3dcore/j3d.h>
#include <std/types.h>
#include <std/RTI/addresses.h>

J3D_EXTERN_C_START

typedef uint32_t tStdColor;

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

static const ColorInfo stdColor_cfARGB4444 =
{
    .colorMode          = STDCOLOR_RGBA,
    .bpp                = 16,
    .redBPP             = 4,
    .greenBPP           = 4,
    .blueBPP            = 4,
    .redPosShift        = 8,
    .greenPosShift      = 4,
    .bluePosShift       = 0,
    .redPosShiftRight   = 4,
    .greenPosShiftRight = 4,
    .bluePosShiftRight  = 4,
    .alphaBPP           = 4,
    .alphaPosShift      = 12,
    .alphaPosShiftRight = 4
};

static const ColorInfo stdColor_cfARGB5551 =
{
    .colorMode          = STDCOLOR_RGBA,
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
    .alphaBPP           = 1,
    .alphaPosShift      = 15,
    .alphaPosShiftRight = 7
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


/**
 * Encodes RGB values into the standard RGB(BE) format with alpha set to 0xFF (fully opaque)
 *
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @return Encoded color in stdColor_cfARGB8888 format with alpha set to 0xFF
 */
#define STD_RGB(r, g, b) \
    (0xFF000000 | ((tStdColor)(r) << 16) | ((tStdColor)(g) << 8) | (tStdColor)(b))

/**
 * Encodes RGBA values into the standard RGBA(BE) format (0xAARRGGBB)
 *
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @param a Alpha component (0-255)
 * @return Encoded color in stdColor_cfARGB8888 format
 */
#define STD_RGBA(r, g, b, a) \
    (((tStdColor)(a) << 24) | ((tStdColor)(r) << 16) | ((tStdColor)(g) << 8) | (tStdColor)(b))

// Macros to extract individual color components (Alpha, Red, Green, Blue)
// from a encoded color integer generated by STD_RGB or STD_RGBA (format: 0xAARRGGBB)
#define STD_GETALPHA(color) (((color) >> 24) & 0xFF)  // Extracts the alpha component
#define STD_GETRED(color)   (((color) >> 16) & 0xFF)  // Extracts the red component
#define STD_GETGREEN(color) (((color) >> 8)  & 0xFF)  // Extracts the green component
#define STD_GETBLUE(color)  ((color) & 0xFF)          // Extracts the blue component

// TODO: Add stdColor_GammaCorrect

void J3DAPI stdColor_RGBtoHSV(float r, float g, float b, float* hue, float* saturation, float* value); // Added
void J3DAPI stdColor_HSVtoRGB(float hue, float saturation, float value, float* r, float* g, float* b); // Added
void J3DAPI stdColor_ColorConvertOneRow(uint8_t* pDestRow, const ColorInfo* pDestCI, const uint8_t* pSrcRow, const ColorInfo* pSrcCI, int width, int bColorKey, LPDDCOLORKEY pColorKey);
int J3DAPI stdColor_ColorConvertOnePixel(ColorInfo* pDestInfo, int pixl, ColorInfo* pSrcInfo, int bColorKey, LPDDCOLORKEY pColorKey); // Added

/**
 * Encodes RGB color components into a single uint32_t value based on the color format.
 *
 * @param ci Color information structure defining the encoding format
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @return Encoded color value as uint32_t
 */
uint32_t stdColor_EncodeRGB(const ColorInfo* ci, uint8_t r, uint8_t g, uint8_t b); // Added

/**
 * Encodes RGBA color components into a single uint32_t value based on the color format.
 *
 * @param ci Color information structure defining the encoding format
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @param a Alpha component (0-255)
 * @return Encoded color value as uint32_t
 */
uint32_t stdColor_EncodeRGBA(const ColorInfo* ci, uint8_t r, uint8_t g, uint8_t b, uint8_t a); // Added

/**
 * Decodes an encoded color value into RGB components based on the color format.
 *
 * @param encoded The encoded color value
 * @param ci Color information structure defining the decoding format
 * @param r Pointer to store the red component (0-255)
 * @param g Pointer to store the green component (0-255)
 * @param b Pointer to store the blue component (0-255)
 */
void stdColor_DecodeRGB(uint32_t encoded, const ColorInfo* ci, uint8_t* r, uint8_t* g, uint8_t* b); // Added

/**
 * Decodes an encoded color value into RGBA components based on the color format.
 *
 * @param encoded The encoded color value
 * @param ci Color information structure defining the decoding format
 * @param r Pointer to store the red component (0-255)
 * @param g Pointer to store the green component (0-255)
 * @param b Pointer to store the blue component (0-255)
 * @param a Pointer to store the alpha component (0-255)
 */
void stdColor_DecodeRGBA(uint32_t encoded, const ColorInfo* ci, uint8_t* r, uint8_t* g, uint8_t* b, uint8_t* a); // Added

/**
 * Recodes a color value from one color format to another.
 *
 * @param sourceEncoded The source encoded color value
 * @param sourceCI Color information structure defining the source format
 * @param targetCI Color information structure defining the target format
 * @return Color value encoded in the target format
 */
uint32_t stdColor_Recode(uint32_t encoded, const ColorInfo* pSrcCI, const ColorInfo* pDestCI); // Added

// Added
inline void stdColor_CalcColorBits(uint32_t bitmask, uint32_t* cbpp, int32_t* posShift, int32_t* posShiftRight)
{
    if ( !cbpp || !posShift || !posShiftRight )
    {
        return;
    }

    // Fixed: An empty mask has no first set bit and would otherwise loop forever.
    if ( !bitmask )
    {
        *cbpp          = 0;
        *posShift      = 0;
        *posShiftRight = 0;
        return;
    }

    int bits = 0;
    uint32_t curBitmask = bitmask;
    for ( ; (curBitmask & 1) == 0; curBitmask >>= 1 )
    {
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
