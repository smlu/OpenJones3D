// Decode.c: pixel-format conversion and RLE decoding (original 0x004EE6A0-0x004EF890).
#include <smush/insane/Decode/Decode.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

#include <stddef.h>
#include <smush/insane/Decode/DecodeAsm.h>
#include <smush/insane/File/Framestream.h>

static const int decode_masks[9] = { 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF };

static uint8_t decode_buffer[2][0x1400];

static int decode_length;
static int decode_state;
static uint8_t decode_value;
static uint8_t* decode_pSrc;
static int decode_flip;

void Decode_InstallHooks(void)
{
    J3D_HOOKFUNC(decode_Rle);
    J3D_HOOKFUNC(decode_RleInit);
    J3D_HOOKFUNC(decode_RleByte);
    J3D_HOOKFUNC(decode_Run);
    J3D_HOOKFUNC(decode_MakeTable);
    J3D_HOOKFUNC(decode_Convert);
    J3D_HOOKFUNC(decode_ToRGBA);
    J3D_HOOKFUNC(decode_Begin);
}

void Decode_ResetGlobals(void)
{}

void J3DAPI decode_Rle(uint8_t* pDst, uint8_t* pSrc, int n)
{
    decode_RleInit(pSrc);
    while ( n-- ) {
        *pDst++ = decode_RleByte();
    }
    decode_RleEnd();
}

void J3DAPI decode_RleInit(uint8_t* pSrc)
{
    decode_state = 2;
    decode_pSrc = pSrc;
}

void decode_RleEnd(void)
{}

uint8_t decode_RleByte(void)
{
    uint8_t c;

    if ( decode_state == 2 )
    {
        decode_length = (*decode_pSrc >> 1) + 1;
        if ( *decode_pSrc++ & 1 )
        {
            decode_state = 1;
            decode_value = *decode_pSrc++;
        }
        else {
            decode_state = 0;
        }
    }
    switch ( decode_state )
    {
    case 1:
        c = decode_value;
        break;
    case 0:
        c = *decode_pSrc++;
        break;
    }
    if ( --decode_length == 0 ) {
        decode_state = 2;
    }
    return c;
}

void J3DAPI decode_Run(uint8_t* pDst, LecBitmapHeader* pBitmap, int n, uint8_t* pPixel)
{
    switch ( pBitmap->pixelSize )
    {
    case 1:
        framestream_CopyB(pDst, *pPixel, n);
        break;
    case 2:
        if ( pPixel[0] == pPixel[1] ) {
            framestream_CopyB(pDst, pPixel[0], n * 2);
        }
        else
        {
            while ( n-- )
            {
                *(uint16_t*)pDst = *(uint16_t*)pPixel;
                pDst += 2;
            }
        }
        break;
    case 3:
        if ( pPixel[0] == pPixel[1] && pPixel[2] == pPixel[1] ) {
            framestream_CopyB(pDst, pPixel[0], n * 3);
        }
        else
        {
            while ( n-- )
            {
                *pDst++ = pPixel[0];
                *pDst++ = pPixel[1];
                *pDst++ = pPixel[2];
            }
        }
        break;
    case 4:
        if ( pPixel[0] == pPixel[1] && pPixel[2] == pPixel[1] && pPixel[3] == pPixel[1] ) {
            framestream_CopyB(pDst, pPixel[0], n * 4);
        }
        else
        {
            while ( n-- )
            {
                *(uint32_t*)pDst = *(uint32_t*)pPixel;
                pDst += 4;
            }
        }
        break;
    }
}

int J3DAPI decode_MakeTable(LecBitmapHeader* pSrcBitmap, LecBitmapHeader* pDstBitmap, void* pTable, void* pPalette)
{
    if ( pSrcBitmap->colorMode == 0 && pSrcBitmap->pixelSize == 1 && pDstBitmap->colorMode != 0 )
    {
        if ( pSrcBitmap->indexed.pPalette ) {
            pPalette = pSrcBitmap->indexed.pPalette;
        }
        if ( pPalette )
        {
            decode_Begin(pTable, pDstBitmap, 256, 0, pPalette);
            return 0;
        }
    }
    return -1;
}

int J3DAPI decode_Convert(uint8_t* pSrc, LecBitmapHeader* pSrcBitmap, int n, void* pPalette, uint8_t* pDst, LecBitmapHeader* pDstBitmap, void* pTable)
{
    int srcRGBA;
    int dstRGBA;
    int chunk;
    int buf;

    if ( pSrcBitmap->bpp == pDstBitmap->bpp && pSrcBitmap->colorMode == pDstBitmap->colorMode
        && ((pSrcBitmap->colorMode != 1 && pSrcBitmap->colorMode != 2)
            || (pSrcBitmap->format.redPosShift == pDstBitmap->format.redPosShift
                && pSrcBitmap->format.greenPosShift == pDstBitmap->format.greenPosShift
                && pSrcBitmap->format.bluePosShift == pDstBitmap->format.bluePosShift)) )
    {
        framestream_Copy(pDst, pSrc, n * pDstBitmap->pixelSize);
        return 0;
    }

    if ( pTable )
    {
        if ( pSrcBitmap->colorMode == 0 && pSrcBitmap->pixelSize == 1 )
        {
            decode_Lut8(pDst, pSrc, n, pDstBitmap->pixelSize, pTable);
            return 0;
        }
        if ( pSrcBitmap->pixelSize == 2 && pDstBitmap->colorMode != 0 )
        {
            decode_Lut16(pDst, pSrc, n, pDstBitmap->pixelSize, pTable);
            return 0;
        }
    }
    srcRGBA = pSrcBitmap->colorMode == 2 && pSrcBitmap->pixelSize == 4 && pSrcBitmap->format.redPosShift == 0
        && pSrcBitmap->format.greenPosShift == 8 && pSrcBitmap->format.bluePosShift == 16;
    dstRGBA = pDstBitmap->colorMode == 2 && pDstBitmap->pixelSize == 4 && pDstBitmap->format.redPosShift == 0
        && pDstBitmap->format.greenPosShift == 8 && pDstBitmap->format.bluePosShift == 16;
    for ( chunk = n < 0x500 ? n : 0x500; chunk > 0; chunk = n < 0x500 ? n : 0x500 )
    {
        if ( srcRGBA )
        {
            if ( decode_Begin(pDst, pDstBitmap, chunk, pPalette, pSrc) ) {
                return -1;
            }
        }
        else if ( dstRGBA )
        {
            if ( decode_ToRGBA(pSrc, pSrcBitmap, chunk, pPalette, pDst) ) {
                return -1;
            }
        }
        else
        {
            buf = ++decode_flip & 1;
            if ( decode_ToRGBA(pSrc, pSrcBitmap, chunk, pPalette, decode_buffer[buf]) ) {
                return -1;
            }
            if ( decode_Begin(pDst, pDstBitmap, chunk, pPalette, decode_buffer[buf]) ) {
                return -1;
            }
        }
        pSrc += pSrcBitmap->pixelSize * chunk;
        n -= chunk;
        pDst += pDstBitmap->pixelSize * chunk;
    }
    return 0;
}

int J3DAPI decode_ToRGBA(uint8_t* pSrc, LecBitmapHeader* pBitmap, int n, void* pPalette, uint8_t* pDst)
{
    uint32_t pix;
    uint8_t* pIn;
    uint8_t* pOut;

    if ( pBitmap->colorMode == 1 && pBitmap->pixelSize == 2 )
    {
        if ( pBitmap->format.redPosShift == 10 && pBitmap->format.greenPosShift == 5 && pBitmap->format.bluePosShift == 0 )
        {
            decode_555toRGBA(pDst, pSrc, n);
            return 0;
        }
        if ( pBitmap->format.redPosShift == 0 && pBitmap->format.greenPosShift == 5 && pBitmap->format.bluePosShift == 11 )
        {
            decode_565BtoRGBA(pDst, pSrc, n);
            return 0;
        }
        if ( pBitmap->format.redPosShift == 11 && pBitmap->format.greenPosShift == 5 && pBitmap->format.bluePosShift == 0 )
        {
            decode_565toRGBA(pDst, pSrc, n);
            return 0;
        }
    }
    if ( pBitmap->colorMode == 0 )
    {
        if ( pBitmap->indexed.pPalette ) {
            pPalette = pBitmap->indexed.pPalette;
        }
        if ( !pPalette ) {
            return -1;
        }
        decode_Lut8(pDst, pSrc, n, 4, pPalette);
        return 0;
    }
    if ( pBitmap->colorMode == 1 )
    {
        uint8_t rMask;
        uint8_t gMask;
        uint8_t bMask;
        int8_t rShift;
        int8_t gShift;
        int8_t bShift;
        int8_t rShiftR;
        int8_t gShiftR;
        int8_t bShiftR;

        pOut = pDst;
        rMask = (uint8_t)decode_masks[pBitmap->format.redBPP];
        gMask = (uint8_t)decode_masks[pBitmap->format.greenBPP];
        bMask = (uint8_t)decode_masks[pBitmap->format.blueBPP];
        rShift = (int8_t)pBitmap->format.redPosShift;
        gShift = (int8_t)pBitmap->format.greenPosShift;
        bShift = (int8_t)pBitmap->format.bluePosShift;
        rShiftR = (int8_t)pBitmap->format.redPosShiftRight;
        gShiftR = (int8_t)pBitmap->format.greenPosShiftRight;
        bShiftR = (int8_t)pBitmap->format.bluePosShiftRight;
        if ( pBitmap->pixelSize == 2 )
        {
            for ( pIn = pSrc; n != 0; n-- )
            {
                pix = *(uint16_t*)pIn;
                pIn += 2;
                *pOut++ = (uint8_t)((pix >> rShift) << rShiftR) & rMask;
                *pOut++ = (uint8_t)((pix >> gShift) << gShiftR) & gMask;
                *pOut++ = (uint8_t)((pix >> bShift) << bShiftR) & bMask;
                *pOut++ = 0xFF;
            }
            return 0;
        }
        if ( pBitmap->pixelSize == 3 )
        {
            for ( pIn = pSrc; n != 0; n-- )
            {
                pix = *(uint32_t*)pIn & 0xFFFFFF;
                pIn += 3;
                *pOut++ = (uint8_t)((pix >> rShift) << rShiftR) & rMask;
                *pOut++ = (uint8_t)((pix >> gShift) << gShiftR) & gMask;
                *pOut++ = (uint8_t)((pix >> bShift) << bShiftR) & bMask;
                *pOut++ = 0xFF;
            }
            return 0;
        }
        if ( pBitmap->pixelSize == 4 )
        {
            for ( pIn = pSrc; n != 0; n-- )
            {
                pix = *(uint32_t*)pIn;
                pIn += 4;
                *pOut++ = (uint8_t)((pix >> rShift) << rShiftR) & rMask;
                *pOut++ = (uint8_t)((pix >> gShift) << gShiftR) & gMask;
                *pOut++ = (uint8_t)((pix >> bShift) << bShiftR) & bMask;
                *pOut++ = 0xFF;
            }
            return 0;
        }
        return -1;
    }
    if ( pBitmap->colorMode == 2 )
    {
        uint8_t rMask;
        uint8_t gMask;
        uint8_t bMask;
        uint8_t aMask;
        int8_t rShift;
        int8_t gShift;
        int8_t bShift;
        int8_t aShift;
        int8_t rShiftR;
        int8_t gShiftR;
        int8_t bShiftR;
        int8_t aShiftR;

        pOut = pDst;
        rMask = (uint8_t)decode_masks[pBitmap->format.redBPP];
        gMask = (uint8_t)decode_masks[pBitmap->format.greenBPP];
        bMask = (uint8_t)decode_masks[pBitmap->format.blueBPP];
        aMask = (uint8_t)decode_masks[pBitmap->format.alphaBPP];
        rShift = (int8_t)pBitmap->format.redPosShift;
        gShift = (int8_t)pBitmap->format.greenPosShift;
        bShift = (int8_t)pBitmap->format.bluePosShift;
        aShift = (int8_t)pBitmap->format.alphaPosShift;
        rShiftR = (int8_t)pBitmap->format.redPosShiftRight;
        gShiftR = (int8_t)pBitmap->format.greenPosShiftRight;
        bShiftR = (int8_t)pBitmap->format.bluePosShiftRight;
        aShiftR = (int8_t)pBitmap->format.alphaPosShiftRight;
        if ( pBitmap->pixelSize == 2 )
        {
            for ( pIn = pSrc; n != 0; n-- )
            {
                pix = *(uint16_t*)pIn;
                pIn += 2;
                *pOut++ = (uint8_t)((pix >> rShift) << rShiftR) & rMask;
                *pOut++ = (uint8_t)((pix >> gShift) << gShiftR) & gMask;
                *pOut++ = (uint8_t)((pix >> bShift) << bShiftR) & bMask;
                *pOut++ = (uint8_t)((pix >> aShift) << aShiftR) & aMask;
            }
            return 0;
        }
        if ( pBitmap->pixelSize == 3 )
        {
            for ( pIn = pSrc; n != 0; n-- )
            {
                pix = *(uint32_t*)pIn & 0xFFFFFF;
                pIn += 3;
                *pOut++ = (uint8_t)((pix >> rShift) << rShiftR) & rMask;
                *pOut++ = (uint8_t)((pix >> gShift) << gShiftR) & gMask;
                *pOut++ = (uint8_t)((pix >> bShift) << bShiftR) & bMask;
                *pOut++ = (uint8_t)((pix >> aShift) << aShiftR) & aMask;
            }
            return 0;
        }
        if ( pBitmap->pixelSize == 4 )
        {
            for ( pIn = pSrc; n != 0; n-- )
            {
                pix = *(uint32_t*)pIn;
                pIn += 4;
                *pOut++ = (uint8_t)((pix >> rShift) << rShiftR) & rMask;
                *pOut++ = (uint8_t)((pix >> gShift) << gShiftR) & gMask;
                *pOut++ = (uint8_t)((pix >> bShift) << bShiftR) & bMask;
                *pOut++ = (uint8_t)((pix >> aShift) << aShiftR) & aMask;
            }
            return 0;
        }
        return -1;
    }
    return -1;
}

int J3DAPI decode_Begin(uint8_t* pDst, LecBitmapHeader* pBitmap, int n, void* pPalette, uint8_t* pSrc)
{
    if ( pBitmap->colorMode == 1 && pBitmap->pixelSize == 2 )
    {
        if ( pBitmap->format.redPosShift == 10 && pBitmap->format.greenPosShift == 5 && pBitmap->format.bluePosShift == 0 )
        {
            decode_RGBAto555(pDst, pSrc, n);
            return 0;
        }
        if ( pBitmap->format.redPosShift == 0 && pBitmap->format.greenPosShift == 5 && pBitmap->format.bluePosShift == 11 )
        {
            decode_RGBAto565B(pDst, pSrc, n);
            return 0;
        }
        if ( pBitmap->format.redPosShift == 11 && pBitmap->format.greenPosShift == 5 && pBitmap->format.bluePosShift == 0 )
        {
            decode_RGBAto565(pDst, pSrc, n);
            return 0;
        }
    }
    if ( pBitmap->colorMode == 1 )
    {
        uint8_t* pIn;
        uint8_t* pOut;
        uint32_t pix;
        uint8_t rMask;
        uint8_t gMask;
        uint8_t bMask;
        int8_t rShift;
        int8_t gShift;
        int8_t bShift;
        int8_t rShiftR;
        int8_t gShiftR;
        int8_t bShiftR;

        rMask = (uint8_t)decode_masks[pBitmap->format.redBPP];
        gMask = (uint8_t)decode_masks[pBitmap->format.greenBPP];
        bMask = (uint8_t)decode_masks[pBitmap->format.blueBPP];
        rShift = (int8_t)pBitmap->format.redPosShift;
        gShift = (int8_t)pBitmap->format.greenPosShift;
        bShift = (int8_t)pBitmap->format.bluePosShift;
        rShiftR = (int8_t)pBitmap->format.redPosShiftRight;
        gShiftR = (int8_t)pBitmap->format.greenPosShiftRight;
        bShiftR = (int8_t)pBitmap->format.bluePosShiftRight;
        if ( pBitmap->pixelSize == 1 )
        {
            pOut = pDst;
            for ( pIn = pSrc; n != 0; n-- )
            {
                *pOut++ = (((pIn[2] & bMask) << bShift) >> bShiftR) | (((pIn[0] & rMask) << rShift) >> rShiftR) | (((pIn[1] & gMask) << gShift) >> gShiftR);
                pIn += 4;
            }
        }
        else if ( pBitmap->pixelSize == 2 )
        {
            pOut = pDst;
            for ( pIn = pSrc; n != 0; n-- )
            {
                *(uint16_t*)pOut = (((pIn[2] & bMask) << bShift) >> bShiftR) | (((pIn[0] & rMask) << rShift) >> rShiftR) | (((pIn[1] & gMask) << gShift) >> gShiftR);
                pOut += 2;
                pIn += 4;
            }
        }
        else if ( pBitmap->pixelSize == 3 )
        {
            pOut = pDst;
            for ( pIn = pSrc; n != 0; n-- )
            {
                pix = (((uint32_t)(pIn[0] & rMask) << rShift) >> rShiftR) | (((uint32_t)(pIn[2] & bMask) << bShift) >> bShiftR) | (((uint32_t)(pIn[1] & gMask) << gShift) >> gShiftR);
                *pOut++ = ((uint8_t*)&pix)[0];
                *pOut++ = ((uint8_t*)&pix)[1];
                *pOut++ = ((uint8_t*)&pix)[2];
                pIn += 4;
            }
        }
        else if ( pBitmap->pixelSize == 4 )
        {
            pOut = pDst;
            for ( pIn = pSrc; n != 0; n-- )
            {
                *(uint32_t*)pOut = (((uint32_t)(pIn[0] & rMask) << rShift) >> rShiftR) | (((uint32_t)(pIn[2] & bMask) << bShift) >> bShiftR) | (((uint32_t)(pIn[1] & gMask) << gShift) >> gShiftR);
                pOut += 4;
                pIn += 4;
            }
        }
        else {
            return -1;
        }
        return 0;
    }
    if ( pBitmap->colorMode == 2 )
    {
        uint8_t* pIn;
        uint8_t* pOut;
        uint32_t pix;
        uint8_t rMask;
        uint8_t gMask;
        uint8_t bMask;
        uint8_t aMask;
        int8_t rShift;
        int8_t gShift;
        int8_t bShift;
        int8_t aShift;
        int8_t rShiftR;
        int8_t gShiftR;
        int8_t bShiftR;
        int8_t aShiftR;

        rMask = (uint8_t)decode_masks[pBitmap->format.redBPP];
        gMask = (uint8_t)decode_masks[pBitmap->format.greenBPP];
        bMask = (uint8_t)decode_masks[pBitmap->format.blueBPP];
        aMask = (uint8_t)decode_masks[pBitmap->format.alphaBPP];
        rShift = (int8_t)pBitmap->format.redPosShift;
        gShift = (int8_t)pBitmap->format.greenPosShift;
        bShift = (int8_t)pBitmap->format.bluePosShift;
        aShift = (int8_t)pBitmap->format.alphaPosShift;
        rShiftR = (int8_t)pBitmap->format.redPosShiftRight;
        gShiftR = (int8_t)pBitmap->format.greenPosShiftRight;
        bShiftR = (int8_t)pBitmap->format.bluePosShiftRight;
        aShiftR = (int8_t)pBitmap->format.alphaPosShiftRight;
        if ( pBitmap->pixelSize == 2 )
        {
            pOut = pDst;
            for ( pIn = pSrc; n != 0; n-- )
            {
                *(uint16_t*)pOut = (((pIn[2] & bMask) << bShift) >> bShiftR) | (((pIn[0] & rMask) << rShift) >> rShiftR) | (((pIn[1] & gMask) << gShift) >> gShiftR) | (((pIn[3] & aMask) << aShift) >> aShiftR);
                pOut += 2;
                pIn += 4;
            }
        }
        else if ( pBitmap->pixelSize == 3 )
        {
            pOut = pDst;
            for ( pIn = pSrc; n != 0; n-- )
            {
                pix = (((uint32_t)(pIn[0] & rMask) << rShift) >> rShiftR) | (((uint32_t)(pIn[2] & bMask) << bShift) >> bShiftR) | (((uint32_t)(pIn[1] & gMask) << gShift) >> gShiftR) | (((uint32_t)(pIn[3] & aMask) << aShift) >> aShiftR);
                *pOut++ = ((uint8_t*)&pix)[0];
                *pOut++ = ((uint8_t*)&pix)[1];
                *pOut++ = ((uint8_t*)&pix)[2];
                pIn += 4;
            }
        }
        else if ( pBitmap->pixelSize == 4 )
        {
            pOut = pDst;
            for ( pIn = pSrc; n != 0; n-- )
            {
                *(uint32_t*)pOut = (((uint32_t)(pIn[0] & rMask) << rShift) >> rShiftR) | (((uint32_t)(pIn[2] & bMask) << bShift) >> bShiftR) | (((uint32_t)(pIn[1] & gMask) << gShift) >> gShiftR) | (((uint32_t)(pIn[3] & aMask) << aShift) >> aShiftR);
                pOut += 4;
                pIn += 4;
            }
        }
        else {
            return -1;
        }
        return 0;
    }
    return -1;
}
