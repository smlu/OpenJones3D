// DecodeAsm.c: C translation of INSANE's MASM source DecodeAsm.asm (the pixel converters) (original 0x004F0590-0x004F09BA).
#include <smush/insane/Decode/DecodeAsm.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/addresses.h>
#include <smush/RTI/symbols.h>

#include <string.h>

void DecodeAsm_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(decode_Lut8);
    J3D_HOOKFUNC(decode_Lut16);
    J3D_HOOKFUNC(decode_RGBAto555);
    J3D_HOOKFUNC(decode_555toRGBA);
    J3D_HOOKFUNC(decode_RGBAto565);
    J3D_HOOKFUNC(decode_565toRGBA);
    J3D_HOOKFUNC(decode_RGBAto565B);
    J3D_HOOKFUNC(decode_565BtoRGBA);
}

void DecodeAsm_ResetGlobals(void)
{}

// Pixel buffers need not be aligned
static uint16_t decode_Load16(const uint8_t* p)
{
    uint16_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static uint32_t decode_Load32(const uint8_t* p)
{
    uint32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static void decode_Store16(uint8_t* p, uint16_t value)
{
    memcpy(p, &value, sizeof(value));
}

static void decode_Store32(uint8_t* p, uint32_t value)
{
    memcpy(p, &value, sizeof(value));
}

void J3DAPI decode_Lut8(void* pDst, const void* pSrc, int n, int dstSize, const void* pTable)
{
    // Only the low byte of dstSize selects the destination pixel size; other sizes write nothing.
    // Every loop runs at least once (dec/jg at its end).
    uint8_t* pOut        = (uint8_t*)pDst;
    const uint8_t* pIn   = (const uint8_t*)pSrc;
    const uint8_t* pLut  = (const uint8_t*)pTable;
    int count            = n;
    switch ( (uint8_t)dstSize )
    {
        case 1:
            do
            {
                *pOut++ = pLut[*pIn++];
            } while ( --count > 0 );
            break;

        case 2:
        {
            // An odd first pixel, then pairs; n >> 1 is unsigned and the pair loop runs at least once,
            // so n = 0 converts 2 pixels and n = 1 converts 3.
            int numPairs = (int)((uint32_t)n >> 1);
            if ( n & 1 )
            {
                decode_Store16(pOut, decode_Load16(pLut + *pIn * 2));
                ++pIn;
                pOut += 2;
            }

            do
            {
                uint8_t first  = pIn[0];
                uint8_t second = pIn[1];
                decode_Store32(pOut, ((uint32_t)decode_Load16(pLut + second * 2) << 16) | decode_Load16(pLut + first * 2));
                pIn  += 2;
                pOut += 4;
            } while ( --numPairs > 0 );
            break;
        }

        case 3:
        {
            // Each pixel stores a whole dword, 3 bytes apart, so the last one writes one byte past the n * 3 output.
            // TODO: [BUG] The tripled index is computed in a register whose bits 8-31 are not cleared between pixels, so
            // the previous pixel's tripled index (above its low byte) leaks into the next index. Only sources whose tripled
            // values stay below 0x100 (bytes below 86) index the table as intended.
            uint32_t index = 0;
            do
            {
                index = ((index & 0xFFFFFF00u) | *pIn++) * 3;
                decode_Store32(pOut, decode_Load32(pLut + index));
                pOut += 3;
            } while ( --count > 0 );
            break;
        }

        case 4:
            do
            {
                decode_Store32(pOut, decode_Load32(pLut + *pIn++ * 4));
                pOut += 4;
            } while ( --count > 0 );
            break;

        default:
            break;
    }
}

void J3DAPI decode_Lut16(void* pDst, const void* pSrc, int n, int dstSize, const void* pTable)
{
    // As decode_Lut8 with 16-bit source pixels (65536-entry tables).
    uint8_t* pOut        = (uint8_t*)pDst;
    const uint8_t* pIn   = (const uint8_t*)pSrc;
    const uint8_t* pLut  = (const uint8_t*)pTable;
    int count            = n;
    switch ( (uint8_t)dstSize )
    {
        case 1:
            do
            {
                *pOut++ = pLut[decode_Load16(pIn)];
                pIn += 2;
            } while ( --count > 0 );
            break;

        case 2:
        {
            int numPairs = (int)((uint32_t)n >> 1);
            if ( n & 1 )
            {
                decode_Store16(pOut, decode_Load16(pLut + decode_Load16(pIn) * 2));
                pIn  += 2;
                pOut += 2;
            }

            do
            {
                uint32_t pair = decode_Load32(pIn);
                decode_Store32(pOut, ((uint32_t)decode_Load16(pLut + (pair >> 16) * 2) << 16) | decode_Load16(pLut + (pair & 0xFFFFu) * 2));
                pIn  += 4;
                pOut += 4;
            } while ( --numPairs > 0 );
            break;
        }

        case 3:
        {
            // TODO: [BUG] As in decode_Lut8, the bits of the previous tripled index above its low word leak into the next
            // index; only sources below 0x5556 index the table as intended.
            uint32_t index = 0;
            do
            {
                index = ((index & 0xFFFF0000u) | decode_Load16(pIn)) * 3;
                decode_Store32(pOut, decode_Load32(pLut + index));
                pIn  += 2;
                pOut += 3;
            } while ( --count > 0 );
            break;
        }

        case 4:
            do
            {
                decode_Store32(pOut, decode_Load32(pLut + decode_Load16(pIn) * 4));
                pIn  += 2;
                pOut += 4;
            } while ( --count > 0 );
            break;

        default:
            break;
    }
}

// RGBA pixels are R, G, B, A bytes in memory (R in the low byte of the little-endian dword).

static uint16_t decode_PackRGBA555(uint32_t color) // 0RRRRRGGGGGBBBBB
{
    return (uint16_t)(((color & 0xF8u) << 7) | ((color >> 6) & 0x03E0u) | ((color >> 19) & 0x001Fu));
}

static uint16_t decode_PackRGBA565(uint32_t color) // RRRRRGGGGGGBBBBB
{
    return (uint16_t)(((color & 0xF8u) << 8) | ((color >> 5) & 0x07E0u) | ((color >> 19) & 0x001Fu));
}

static uint16_t decode_PackRGBA565B(uint32_t color) // BBBBBGGGGGGRRRRR
{
    return (uint16_t)(((color >> 8) & 0xF800u) | ((color >> 5) & 0x07E0u) | ((color >> 3) & 0x001Fu));
}

// 555 and 565 expand with the low bits of every channel set (and alpha 0xFF); 565B expands with zero low bits and alpha 0.

static uint32_t decode_Unpack555(uint16_t pixel)
{
    uint32_t r = ((pixel >> 7) & 0xF8u) | 0x07u;
    uint32_t g = ((pixel >> 2) & 0xF8u) | 0x07u;
    uint32_t b = ((pixel << 3) & 0xF8u) | 0x07u;
    return 0xFF000000u | (b << 16) | (g << 8) | r;
}

static uint32_t decode_Unpack565(uint16_t pixel)
{
    uint32_t r = ((pixel >> 8) & 0xF8u) | 0x07u;
    uint32_t g = ((pixel >> 3) & 0xFCu) | 0x03u;
    uint32_t b = ((pixel << 3) & 0xF8u) | 0x07u;
    return 0xFF000000u | (b << 16) | (g << 8) | r;
}

static uint32_t decode_Unpack565B(uint16_t pixel)
{
    return ((uint32_t)(pixel & 0x001Fu) << 3) | ((uint32_t)(pixel & 0x07E0u) << 5) | ((uint32_t)(pixel & 0xF800u) << 8);
}

// The six converters handle an odd first pixel, then pairs. n >> 1 is unsigned and the pair loop runs at least once
// (dec/jg at its end), so n = 0 converts 2 pixels and n = 1 converts 3. Both pixels of a pair are read before either is
// written.

static void decode_PackPixels(uint8_t* pOut, const uint8_t* pIn, int n, uint16_t (*pfPack)(uint32_t))
{
    int numPairs = (int)((uint32_t)n >> 1);
    if ( n & 1 )
    {
        decode_Store16(pOut, pfPack(decode_Load32(pIn)));
        pIn  += 4;
        pOut += 2;
    }

    do
    {
        uint32_t second = decode_Load32(pIn + 4);
        uint32_t first  = decode_Load32(pIn);
        decode_Store32(pOut, ((uint32_t)pfPack(second) << 16) | pfPack(first));
        pIn  += 8;
        pOut += 4;
    } while ( --numPairs > 0 );
}

static void decode_UnpackPixels(uint8_t* pOut, const uint8_t* pIn, int n, uint32_t (*pfUnpack)(uint16_t))
{
    int numPairs = (int)((uint32_t)n >> 1);
    if ( n & 1 )
    {
        decode_Store32(pOut, pfUnpack(decode_Load16(pIn)));
        pIn  += 2;
        pOut += 4;
    }

    do
    {
        uint16_t second = decode_Load16(pIn + 2);
        uint16_t first  = decode_Load16(pIn);
        decode_Store32(pOut + 4, pfUnpack(second));
        decode_Store32(pOut, pfUnpack(first));
        pIn  += 4;
        pOut += 8;
    } while ( --numPairs > 0 );
}

void J3DAPI decode_RGBAto555(void* pDst, const void* pSrc, int n)
{
    decode_PackPixels((uint8_t*)pDst, (const uint8_t*)pSrc, n, decode_PackRGBA555);
}

void J3DAPI decode_555toRGBA(void* pDst, const void* pSrc, int n)
{
    decode_UnpackPixels((uint8_t*)pDst, (const uint8_t*)pSrc, n, decode_Unpack555);
}

void J3DAPI decode_RGBAto565(void* pDst, const void* pSrc, int n)
{
    decode_PackPixels((uint8_t*)pDst, (const uint8_t*)pSrc, n, decode_PackRGBA565);
}

void J3DAPI decode_565toRGBA(void* pDst, const void* pSrc, int n)
{
    decode_UnpackPixels((uint8_t*)pDst, (const uint8_t*)pSrc, n, decode_Unpack565);
}

void J3DAPI decode_RGBAto565B(void* pDst, const void* pSrc, int n)
{
    decode_PackPixels((uint8_t*)pDst, (const uint8_t*)pSrc, n, decode_PackRGBA565B);
}

void J3DAPI decode_565BtoRGBA(void* pDst, const void* pSrc, int n)
{
    decode_UnpackPixels((uint8_t*)pDst, (const uint8_t*)pSrc, n, decode_Unpack565B);
}
