// MixerAsm.c: C translation of INSANE's MASM source MixerAsm.asm (mixer buffer procedures) (original 0x004EC560-0x004EC71F).
#include <smush/insane/Sound/MixerAsm.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/addresses.h>
#include <smush/RTI/symbols.h>

#include <string.h>

void MixerAsm_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(mixer_MemCopy);
    J3D_HOOKFUNC(mixer_MemFill);
    J3D_HOOKFUNC(mixer_Lut16);
    J3D_HOOKFUNC(mixer_Lut16Add);
    J3D_HOOKFUNC(mixer_Fill16);
    J3D_HOOKFUNC(mixer_Lut8);
    J3D_HOOKFUNC(mixer_Lut8Add);
    J3D_HOOKFUNC(mixer_FillB);
}

void MixerAsm_ResetGlobals(void)
{}

void J3DAPI mixer_MemCopy(void* pDst, const void* pSrc, int n)
{
    // Forward, element by element like the original's string instructions (overlapping buffers included):
    // whole dwords when n is a multiple of 4, otherwise n >> 1 words and the odd byte.
    uint8_t* pOut      = (uint8_t*)pDst;
    const uint8_t* pIn = (const uint8_t*)pSrc;
    uint32_t count     = (uint32_t)n;
    if ( count == 0 )
    {
        return;
    }

    if ( (count & 3) == 0 )
    {
        for ( uint32_t i = count >> 2; i != 0; --i )
        {
            uint32_t value;
            memcpy(&value, pIn, sizeof(value));
            memcpy(pOut, &value, sizeof(value));
            pIn  += sizeof(value);
            pOut += sizeof(value);
        }
        return;
    }

    for ( uint32_t i = count >> 1; i != 0; --i )
    {
        uint16_t value;
        memcpy(&value, pIn, sizeof(value));
        memcpy(pOut, &value, sizeof(value));
        pIn  += sizeof(value);
        pOut += sizeof(value);
    }

    if ( count & 1 )
    {
        *pOut = *pIn;
    }
}

void J3DAPI mixer_MemFill(void* pDst, int value, int n)
{
    // The original replicates the low byte of value into a dword and stores n bytes in 20000-byte runs, then words and
    // the odd byte, or dwords. Every byte gets the same value, so the result is a plain fill of n bytes.
    memset(pDst, (uint8_t)value, (uint32_t)n);
}

void J3DAPI mixer_Lut16(uint16_t* pDst, const uint8_t* pSrc, int n, const uint16_t* pTables, int tableIndex)
{
    // Tables are 256 words (512 bytes) apart. The loop instruction runs at least once: n == 0 runs 2^32 times.
    const uint16_t* pTable = (const uint16_t*)((const uint8_t*)pTables + ((uint32_t)tableIndex << 9));
    uint32_t count         = (uint32_t)n;
    do
    {
        *pDst++ = pTable[*pSrc++];
    } while ( --count != 0 );
}

void J3DAPI mixer_Lut16Add(uint16_t* pDst, const uint8_t* pSrc, int n, const uint16_t* pTables, int tableIndex)
{
    const uint16_t* pTable = (const uint16_t*)((const uint8_t*)pTables + ((uint32_t)tableIndex << 9));
    uint32_t count         = (uint32_t)n;
    do
    {
        uint8_t sample = *pSrc++;
        *pDst          = (uint16_t)(pTable[sample] + *pDst);
        ++pDst;
    } while ( --count != 0 );
}

void J3DAPI mixer_Fill16(uint16_t* pDst, int value, int n)
{
    for ( uint32_t count = (uint32_t)n; count != 0; --count )
    {
        *pDst++ = (uint16_t)value;
    }
}

void J3DAPI mixer_Lut8(uint8_t* pDst, const uint8_t* pSrc, int n, const uint8_t* pTables, int tableIndex)
{
    // Tables are 256 bytes apart. The loop instruction runs at least once: n == 0 runs 2^32 times.
    const uint8_t* pTable = pTables + ((uint32_t)tableIndex << 8);
    uint32_t count        = (uint32_t)n;
    do
    {
        *pDst++ = pTable[*pSrc++];
    } while ( --count != 0 );
}

void J3DAPI mixer_Lut8Add(uint8_t* pDst, const uint8_t* pSrc, int n, const uint8_t* pTables, int tableIndex)
{
    const uint8_t* pTable = pTables + ((uint32_t)tableIndex << 8);
    uint32_t count        = (uint32_t)n;
    do
    {
        uint8_t sample = *pSrc++;
        *pDst          = (uint8_t)(pTable[sample] + *pDst);
        ++pDst;
    } while ( --count != 0 );
}

void J3DAPI mixer_FillB(void* pDst, int value, int n)
{
    // TODO: [BUG] The odd-size test is inverted: a multiple of 4 is stored as n >> 1 words and the (zero) odd byte, any other
    // size as n >> 2 dwords, so the n & 3 trailing bytes are never written.
    memset(pDst, (uint8_t)value, (uint32_t)n & ~3u);
}
