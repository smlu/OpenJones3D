// Bloc16Asm.c: C translation of INSANE's MASM source Bloc16Asm.asm (blocky16 codec 2 block decoder) (original 0x004EDEF0-0x004EE02C).
#include <smush/insane/Flobject/Bloc16Asm.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/addresses.h>
#include <smush/RTI/symbols.h>

#include <string.h>

void Bloc16Asm_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(dbloc16_Interpolate);
    J3D_HOOKFUNC(dbloc16_Codec2);
}

void Bloc16Asm_ResetGlobals(void)
{}

// The block decoder's state; the original keeps it in the assembly's own data block (0x0054DC70), which dbloc16_Codec2
// fills on every call before it is read.
typedef struct sBloc16Context
{
    int prevOffset;           // previous frame - current frame, in bytes
    int deltaOffset;          // delta frame - current frame, in bytes
    int lineSize;             // bytes per line (width * 2)
    const int16_t* pMotion;   // motion vectors (pixel offsets) for opcodes 0x00-0xF4
    const uint8_t* pColors;   // the four fill colours (words) for opcodes 0xF9-0xFC
    const Bl16Small* pSmall;  // 4x4 glyphs
    const Bl16Big* pBig;      // 8x8 glyphs
    const uint16_t* pPalette; // 8-bit to 16-bit palette
} Bloc16Context;

// Pixels are 16-bit and the blocks are copied as dwords, but nothing guarantees dword alignment
static uint16_t dbloc16_Load16(const uint8_t* p)
{
    uint16_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static uint32_t dbloc16_Load32(const uint8_t* p)
{
    uint32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static void dbloc16_Store16(uint8_t* p, uint16_t value)
{
    memcpy(p, &value, sizeof(value));
}

static void dbloc16_Store32(uint8_t* p, uint32_t value)
{
    memcpy(p, &value, sizeof(value));
}

// Copies a size x size block (8 or 4) from pBlock + offset. The original interleaves the top and bottom halves: for each
// row pair (row, row + size / 2) it reads the bottom dword, then the top one, and stores them in that order. Keep that
// order, the source may overlap the block (offset within the current frame).
static void dbloc16_CopyBlock(uint8_t* pBlock, int offset, int lineSize, int size)
{
    uint8_t* pTop    = pBlock;
    uint8_t* pBottom = pBlock + lineSize * (size / 2);
    for ( int row = 0; row < size / 2; ++row )
    {
        for ( int x = 0; x < size * 2; x += 4 )
        {
            uint32_t bottom = dbloc16_Load32(pBottom + x + offset);
            uint32_t top    = dbloc16_Load32(pTop + x + offset);
            dbloc16_Store32(pBottom + x, bottom);
            dbloc16_Store32(pTop + x, top);
        }

        pTop += lineSize;
        pBottom += lineSize;
    }
}

static void dbloc16_FillBlock(uint8_t* pBlock, uint16_t color, int lineSize, int size)
{
    uint32_t pair = ((uint32_t)color << 16) | color;
    for ( int row = 0; row < size; ++row )
    {
        for ( int x = 0; x < size * 2; x += 4 )
        {
            dbloc16_Store32(pBlock + x, pair);
        }

        pBlock += lineSize;
    }
}

// Draws a two-colour glyph: the low word of colors at the n1 pixel offsets off1, the high word at the n2 offsets off2.
// Offsets are unsigned pixel offsets from the block (movzx). Each loop runs at least once (dec/jg), so a count of 0 still
// draws the first offset.
static void dbloc16_DrawGlyph(uint8_t* pBlock, const int16_t* pOff1, uint8_t n1, const int16_t* pOff2, uint8_t n2, uint32_t colors)
{
    int count = n1;
    do
    {
        dbloc16_Store16(pBlock + (uint16_t)*pOff1++ * 2, (uint16_t)colors);
    } while ( --count > 0 );

    count = n2;
    do
    {
        dbloc16_Store16(pBlock + (uint16_t)*pOff2++ * 2, (uint16_t)(colors >> 16));
    } while ( --count > 0 );
}

// The motion offset of a copy opcode (0x00-0xF4: from the motion table, 0xF5: a 16-bit vector in the stream), in bytes
// from the block and including the previous frame's offset. Advances pSrc past the opcode.
static int dbloc16_MotionOffset(const Bloc16Context* pCtx, const uint8_t** ppSrc)
{
    const uint8_t* pSrc = *ppSrc;
    int offset;
    if ( *pSrc == 0xF5 )
    {
        offset = (int16_t)dbloc16_Load16(pSrc + 1) * 2;
        *ppSrc = pSrc + 3;
    }
    else
    {
        offset = pCtx->pMotion[*pSrc] * 2;
        *ppSrc = pSrc + 1;
    }

    return offset + pCtx->prevOffset;
}

// The fill colour of opcodes 0xF9-0xFE, advancing pSrc past the opcode
static uint16_t dbloc16_FillColor(const Bloc16Context* pCtx, const uint8_t** ppSrc)
{
    const uint8_t* pSrc = *ppSrc;
    uint8_t op          = *pSrc;
    if ( op == 0xFE ) // literal colour
    {
        *ppSrc = pSrc + 3;
        return dbloc16_Load16(pSrc + 1);
    }

    if ( op == 0xFD ) // palette colour
    {
        *ppSrc = pSrc + 2;
        return pCtx->pPalette[pSrc[1]];
    }

    // 0xF9-0xFC: one of the four colours of the frame's parameters
    *ppSrc = pSrc + 1;
    return dbloc16_Load16(pCtx->pColors + (op - 0xF9) * 2);
}

// The glyph opcodes 0xF7 (two palette colours) and 0xF8 (two literal colours): returns the glyph index and the colours
// (first colour in the low word), advancing pSrc past the opcode
static uint8_t dbloc16_GlyphColors(const Bloc16Context* pCtx, const uint8_t** ppSrc, uint32_t* pColors)
{
    const uint8_t* pSrc = *ppSrc;
    if ( *pSrc == 0xF8 )
    {
        *pColors = dbloc16_Load32(pSrc + 2);
        *ppSrc   = pSrc + 6;
    }
    else
    {
        *pColors = ((uint32_t)pCtx->pPalette[pSrc[3]] << 16) | pCtx->pPalette[pSrc[2]];
        *ppSrc   = pSrc + 4;
    }

    return pSrc[1];
}

// Original dbloc16_Block2 (0x004EE58C): register interface esi = opcode stream (returned advanced), edi = block, ebp = line size
static const uint8_t* dbloc16_Block2(const Bloc16Context* pCtx, const uint8_t* pSrc, uint8_t* pBlock)
{
    int lineSize = pCtx->lineSize;
    uint8_t op   = *pSrc;
    if ( op <= 0xF5 )
    {
        int offset = dbloc16_MotionOffset(pCtx, &pSrc);
        dbloc16_Store32(pBlock, dbloc16_Load32(pBlock + offset));
        dbloc16_Store32(pBlock + lineSize, dbloc16_Load32(pBlock + lineSize + offset));
        return pSrc;
    }

    switch ( op )
    {
        case 0xFF:
        case 0xF8: // four literal pixels
            dbloc16_Store32(pBlock, dbloc16_Load32(pSrc + 1));
            dbloc16_Store32(pBlock + lineSize, dbloc16_Load32(pSrc + 5));
            return pSrc + 9;

        case 0xF7: // four palette pixels
            dbloc16_Store16(pBlock, pCtx->pPalette[pSrc[1]]);
            dbloc16_Store16(pBlock + 2, pCtx->pPalette[pSrc[2]]);
            dbloc16_Store16(pBlock + lineSize, pCtx->pPalette[pSrc[3]]);
            dbloc16_Store16(pBlock + lineSize + 2, pCtx->pPalette[pSrc[4]]);
            return pSrc + 5;

        case 0xF6: // copy from the delta frame
            dbloc16_Store32(pBlock, dbloc16_Load32(pBlock + pCtx->deltaOffset));
            dbloc16_Store32(pBlock + lineSize, dbloc16_Load32(pBlock + lineSize + pCtx->deltaOffset));
            return pSrc + 1;

        default: // 0xF9-0xFE
            dbloc16_FillBlock(pBlock, dbloc16_FillColor(pCtx, &pSrc), lineSize, 2);
            return pSrc;
    }
}

// Original dbloc16_Block4 (0x004EE3C4): register interface as dbloc16_Block2
static const uint8_t* dbloc16_Block4(const Bloc16Context* pCtx, const uint8_t* pSrc, uint8_t* pBlock)
{
    int lineSize = pCtx->lineSize;
    uint8_t op   = *pSrc;
    if ( op <= 0xF5 )
    {
        int offset = dbloc16_MotionOffset(pCtx, &pSrc);
        dbloc16_CopyBlock(pBlock, offset, lineSize, 4);
        return pSrc;
    }

    switch ( op )
    {
        case 0xFF: // four 2x2 blocks
            pSrc = dbloc16_Block2(pCtx, pSrc + 1, pBlock);
            pSrc = dbloc16_Block2(pCtx, pSrc, pBlock + 4);
            pSrc = dbloc16_Block2(pCtx, pSrc, pBlock + lineSize * 2);
            return dbloc16_Block2(pCtx, pSrc, pBlock + lineSize * 2 + 4);

        case 0xF7:
        case 0xF8:
        {
            uint32_t colors;
            const Bl16Small* pGlyph = &pCtx->pSmall[dbloc16_GlyphColors(pCtx, &pSrc, &colors)];
            dbloc16_DrawGlyph(pBlock, pGlyph->off1, pGlyph->n1, pGlyph->off2, pGlyph->n2, colors);
            return pSrc;
        }

        case 0xF6: // copy from the delta frame
            dbloc16_CopyBlock(pBlock, pCtx->deltaOffset, lineSize, 4);
            return pSrc + 1;

        default: // 0xF9-0xFE
            dbloc16_FillBlock(pBlock, dbloc16_FillColor(pCtx, &pSrc), lineSize, 4);
            return pSrc;
    }
}

// Original dbloc16_Block8 (0x004EE02C): register interface as dbloc16_Block2
static const uint8_t* dbloc16_Block8(const Bloc16Context* pCtx, const uint8_t* pSrc, uint8_t* pBlock)
{
    int lineSize = pCtx->lineSize;
    uint8_t op   = *pSrc;
    if ( op <= 0xF5 )
    {
        int offset = dbloc16_MotionOffset(pCtx, &pSrc);
        dbloc16_CopyBlock(pBlock, offset, lineSize, 8);
        return pSrc;
    }

    switch ( op )
    {
        case 0xFF: // four 4x4 blocks
            pSrc = dbloc16_Block4(pCtx, pSrc + 1, pBlock);
            pSrc = dbloc16_Block4(pCtx, pSrc, pBlock + 8);
            pSrc = dbloc16_Block4(pCtx, pSrc, pBlock + lineSize * 4);
            return dbloc16_Block4(pCtx, pSrc, pBlock + lineSize * 4 + 8);

        case 0xF7:
        case 0xF8:
        {
            uint32_t colors;
            const Bl16Big* pGlyph = &pCtx->pBig[dbloc16_GlyphColors(pCtx, &pSrc, &colors)];
            dbloc16_DrawGlyph(pBlock, pGlyph->off1, pGlyph->n1, pGlyph->off2, pGlyph->n2, colors);
            return pSrc;
        }

        case 0xF6: // copy from the delta frame
            dbloc16_CopyBlock(pBlock, pCtx->deltaOffset, lineSize, 8);
            return pSrc + 1;

        default: // 0xF9-0xFE
            dbloc16_FillBlock(pBlock, dbloc16_FillColor(pCtx, &pSrc), lineSize, 8);
            return pSrc;
    }
}

void J3DAPI dbloc16_Interpolate(uint8_t* pDst, const uint8_t* pSrc, int stride, int rows, const uint8_t* pTable)
{
    // Pass 1: every source row goes to an odd destination row (1, 3, ...), doubled horizontally: the first pixel twice,
    // then for every next pixel the pair-table entry [previous << 8 | pixel] followed by the pixel.
    // TODO: [BUG] The pair index is built in a register whose upper word still holds the upper word of rows, so the
    // table is indexed from pTable + (rows & 0xFFFF0000) in this pass (the second pass clears it first).
    const uint8_t* pPairs = pTable + ((uint32_t)rows & 0xFFFF0000u);
    uint8_t* pOut         = pDst + stride;
    int y                 = rows;
    do
    {
        uint8_t pixel = *pSrc++;
        pOut[0]       = pixel;
        pOut[1]       = pixel;
        pOut += 2;

        int x = stride - 2;
        do
        {
            uint8_t prev = pixel;
            pixel        = *pSrc++;
            pOut[0]      = pPairs[(prev << 8) | pixel];
            pOut[1]      = pixel;
            pOut += 2;
            x -= 2;
        } while ( x > 0 );

        pOut += stride;
        y -= 2;
    } while ( y > 0 );

    // Row 0 is a copy of row 1 (rep movsb)
    for ( uint32_t i = 0; i < (uint32_t)stride; ++i )
    {
        pDst[i] = pDst[stride + i];
    }

    // Pass 2: every even row from 2 on is the pair-table entry [below << 8 | above] of its neighbours
    pOut = pDst + 2 * stride;
    y    = rows - 2;
    do
    {
        int x = stride;
        do
        {
            *pOut = pTable[(pOut[stride] << 8) | pOut[-stride]];
            ++pOut;
        } while ( --x > 0 );

        pOut += stride;
        y -= 2;
    } while ( y > 0 );
}

void J3DAPI dbloc16_Codec2(uint16_t* pDst, uint8_t* pSrc, int offset1, int offset2, int width, int16_t* pMotion, uint8_t* pParams, int height, int nextLine, Bl16Small* pSmall, Bl16Big* pBig, uint16_t* pPalette)
{
    J3D_UNUSED(nextLine); // Not read by the assembly

    Bloc16Context ctx;
    ctx.prevOffset  = (int)((uint32_t)offset1 * 2);
    ctx.deltaOffset = (int)((uint32_t)offset2 * 2);
    ctx.lineSize    = (int)((uint32_t)width * 2);
    ctx.pMotion     = pMotion;
    ctx.pColors     = pParams;
    ctx.pSmall      = pSmall;
    ctx.pBig        = pBig;
    ctx.pPalette    = pPalette;

    // The block counts are bytes: 0 (a size of 0 or 2041-2048) runs 256 blocks, and sizes above 2048 wrap.
    // Each row of blocks advances by its blocks (16 bytes each) plus 7 lines, which is 8 lines only when width is a multiple of 8.
    uint8_t numRows    = (uint8_t)(((uint32_t)height + 7) >> 3);
    uint8_t numCols    = (uint8_t)(((uint32_t)width + 7) >> 3);
    uint32_t rowSkip   = (uint32_t)ctx.lineSize * 7;
    uint8_t* pBlock    = (uint8_t*)pDst;
    const uint8_t* pIn = pSrc;
    do
    {
        uint8_t col = numCols;
        do
        {
            pIn = dbloc16_Block8(&ctx, pIn, pBlock);
            pBlock += 16;
        } while ( --col != 0 );

        pBlock += rowSkip;
    } while ( --numRows != 0 );
}
