// BlitAsm.c: C translation of INSANE's MASM source BlitAsm.asm (blit_Masm) (original 0x004F2220-0x004F22B7).
#include <smush/insane/Bitmap/BlitAsm.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/addresses.h>
#include <smush/RTI/symbols.h>

#include <string.h>

void BlitAsm_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(blit_Masm);
}

void BlitAsm_ResetGlobals(void)
{}

// The row offset y * pitch is computed with a 16-bit multiply (mul bx / mul dx): the low words of y and pitch are multiplied
// to a 32-bit product, and the upper word of y is kept in the result (OR'd in, because mul writes only ax and dx).
static uint32_t blit_RowOffset(int y, int pitch)
{
    uint32_t product = (uint32_t)(uint16_t)y * (uint16_t)pitch;
    return ((uint32_t)y & 0xFFFF0000u) | product;
}

void J3DAPI blit_Masm(void* pSrc, void* pDst, int srcX, int srcY, int dstX, int dstY, int width, int height, int srcPitch, int dstPitch)
{
    const uint8_t* pIn = (const uint8_t*)pSrc + srcX + blit_RowOffset(srcY, srcPitch);
    uint8_t* pOut      = (uint8_t*)pDst + dstX + blit_RowOffset(dstY, dstPitch);

    // Each row copies width >> 2 dwords (rep movsd), then the width & 3 odd bytes (rep movsb), forward and element by element.
    // Pitches advance the full 32-bit value. The row loop runs at least once (dec/jg at its end).
    uint32_t numDwords = (uint32_t)width >> 2;
    uint32_t numBytes  = (uint32_t)width & 3;
    int rows = height;
    do
    {
        const uint8_t* pInRow = pIn;
        uint8_t* pOutRow      = pOut;
        for ( uint32_t i = 0; i < numDwords; ++i )
        {
            uint32_t value;
            memcpy(&value, pInRow, sizeof(value));
            memcpy(pOutRow, &value, sizeof(value));
            pInRow  += sizeof(value);
            pOutRow += sizeof(value);
        }

        for ( uint32_t i = 0; i < numBytes; ++i )
        {
            *pOutRow++ = *pInRow++;
        }

        pIn  += srcPitch;
        pOut += dstPitch;
    } while ( --rows > 0 );
}
