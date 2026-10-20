// Mixer.c: the software mixer (original 0x004EC720-0x004EDEF0).
#include <smush/insane/Sound/Mixer.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/addresses.h>
#include <smush/RTI/symbols.h>

#include <math.h>
#include <stdint.h>
#include <string.h>

#include <smush/insane/Bitmap/Blit.h>
#include <smush/insane/Decode/Decode.h>

// Byte-swaps a 16-bit sample (big-endian sources)
static int mixer_Swap16(int x)
{
    return (((uint16_t)x & 0xFF) << 8) | ((uint16_t)x >> 8);
}

static int mixer_headroom = 4;
static int mixer_channels = 0;
static int16_t* mixer_pClip = NULL;
static uint8_t mixer_pan[17][17];
static int mixer_size = 0;
static int mixer_a4 = 0;
static int32_t* mixer_pBuffer = NULL;

int J3DAPI mixer_Init(MemoryDebugInterface* pMemory, int headroom, int size, int a4, int a5, int a6)
{
    J3D_UNUSED(a4);
    J3D_UNUSED(a5);
    J3D_UNUSED(a6);
    mixer_headroom = headroom;
    return mixer_Setup(pMemory, 2, size * 4, 0);
}

void J3DAPI mixer_Done(MemoryDebugInterface* pMemory)
{
    mixer_Free(pMemory);
}

int J3DAPI mixer_Begin(int n)
{
    J3D_UNUSED(n);
    return mixer_Clear();
}

void J3DAPI mixer_Feed(void* pSrc, int offset, int n, int bytes, int a5, int volume, int pan, int format)
{
    J3D_UNUSED(a5);

    int bits;
    int channels;
    int isSigned;

    switch ( format )
    {
        case 0:
            channels = 1;
            bits = 8;
            isSigned = 1;
            break;

        case 1:
            channels = 2;
            bits = 8;
            isSigned = 1;
            break;

        case 2:
            channels = 1;
            bits = 16;
            isSigned = 0;
            break;

        case 3:
            channels = 2;
            bits = 16;
            isSigned = 0;
            break;

        case 4:
            channels = 1;
            bits = 16;
            isSigned = 1;
            break;

        case 5:
            channels = 2;
            bits = 16;
            isSigned = 1;
            break;
    }

    mixer_Add(pSrc, bytes / ((bits >> 3) * channels), bits, channels, n, offset, volume, (pan + 127) / 2, isSigned, 0);
}

int J3DAPI mixer_Mix(int16_t* pDst, int n)
{
    return mixer_Output(pDst, n);
}

int J3DAPI mixer_Setup(MemoryDebugInterface* pMemory, int channels, int size, int a4)
{
    uint8_t* pPan = &mixer_pan[0][0];
    for ( int i = 0; i < 17; i++ )
    {
        for ( int j = 0; j < 17; j++ ) {
            *pPan++ = (uint8_t)((sin((j * 3.1415) * 0.03125) * i + 0.5) * 8.0);
        }
    }

    mixer_channels = channels;
    mixer_size = size * 4;
    mixer_pBuffer = pMemory->Alloc(mixer_size, INSANE_FILE, __LINE__);
    if ( !mixer_pBuffer ) {
        return -1;
    }

    mixer_a4 = a4;
    mixer_pClip = pMemory->Alloc(mixer_headroom << 16, INSANE_FILE, __LINE__);
    if ( !mixer_pClip ) {
        return -1;
    }

    int16_t* pClip = mixer_pClip;
    for ( int j = 0; j < mixer_headroom << 15; j++ ) {
        *pClip++ = (int16_t)((j * 524272.0 / (j * 7.0 + 262136.0) + 1.0) * 0.5);
    }

    return 0;
}

int J3DAPI mixer_Free(MemoryDebugInterface* pMemory)
{
    if ( mixer_pClip ) {
        pMemory->Free(mixer_pClip, INSANE_FILE, __LINE__);
    }

    mixer_pClip = NULL;
    if ( mixer_pBuffer ) {
        pMemory->Free(mixer_pBuffer, INSANE_FILE, __LINE__);
    }

    mixer_pBuffer = NULL;
    return 0;
}

int mixer_Clear(void)
{
    memset(mixer_pBuffer, 0, mixer_size);
    return 0;
}

void J3DAPI mixer_Add(void* pSrc, uint32_t count, int bits, int channels, uint32_t n, int offset, int volume, int pan, int isSigned, int isSwapped)
{
    if ( !mixer_pBuffer || !pSrc || !count ) {
        return;
    }

    if ( channels == 1 )
    {
        uint32_t v = volume >> 3;
        if ( volume ) {
            v++;
        }

        if ( v >= 17 ) {
            v = 16;
        }

        int p = (pan >> 3) - 8;
        if ( pan > 64 ) {
            p++;
        }

        int left  = mixer_pan[v][8 - p];
        int right = mixer_pan[v][8 + p];
        if ( bits == 8 ) {
            mixer_Mono8(pSrc, count, n, offset, left, right);
        }
        else if ( !isSigned ) {
            mixer_MonoU16(pSrc, count, n, offset, left, right);
        }
        else if ( isSwapped ) {
            mixer_MonoSwap16(pSrc, count, n, offset, left, right);
        }
        else {
            mixer_Mono16(pSrc, count, n, offset, left, right);
        }
    }
    else
    {
        if ( bits == 8 ) {
            mixer_Stereo8(pSrc, count, n, offset, volume + 1);
        }
        else if ( !isSigned ) {
            mixer_StereoU16(pSrc, count, n, offset, volume + 1);
        }
        else if ( isSwapped ) {
            mixer_StereoSwap16(pSrc, count, n, offset, volume + 1);
        }
        else {
            mixer_Stereo16(pSrc, count, n, offset, volume + 1);
        }
    }
}

int J3DAPI mixer_Output(int16_t* pDst, int n)
{
    int32_t* pBuf = mixer_pBuffer;
    int16_t* pOut = pDst;

    if ( !pBuf || !pOut || !n ) {
        return -1;
    }

    if ( mixer_channels == 2 ) {
        n <<= 1;
    }

    while ( n-- )
    {
        int s = *pBuf++ >> 7;
        if ( s < -0x8000 ) {
            s = -0x8000;
        }
        else if ( s > 0x7FFF ) {
            s = 0x7FFF;
        }

        *pOut++ = (int16_t)s;
    }

    return 0;
}

void J3DAPI mixer_Mono8(uint8_t* pSrc, uint32_t count, uint32_t n, int offset, int left, int right)
{
    int32_t* pBuf = mixer_pBuffer + offset * 2;
    uint32_t step = (count << 16) / n;

    switch ( step )
    {
        case 0x8000:
            count--;
            // TODO: [BUG] The mean uses the sample two ahead (pSrc[2]) instead of the next one; a mono stream has no interleaved channel.
            while ( count-- )
            {
                *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * left;
                *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * right;
                *pBuf++ += (((pSrc[0] + pSrc[2]) << 7) ^ 0x8000) * left;
                *pBuf++ += (((pSrc[0] + pSrc[2]) << 7) ^ 0x8000) * right;
                pSrc++;
            }

            *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * left;
            *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * right;
            *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * left;
            *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * right;
            break;

        case 0x10000:
            while ( n-- )
            {
                *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * left;
                *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * right;
                pSrc++;
            }
            break;

        case 0x20000:
            while ( n-- )
            {
                *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * left;
                *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * right;
                pSrc += 2;
            }
            break;

        default:
        {
            uint32_t frac = 0;
            while ( n-- )
            {
                *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * left;
                *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * right;
                frac += step;
                pSrc += (int)frac >> 16;
                frac &= 0xFFFF;
            }
        } break;
    }
}

void J3DAPI mixer_Stereo8(uint8_t* pSrc, uint32_t count, uint32_t n, int offset, int volume)
{
    int32_t* pBuf = mixer_pBuffer + offset * 2;
    uint32_t step = (count << 16) / n;

    switch ( step )
    {
        case 0x8000:
            count--;
            while ( count-- )
            {
                *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * volume;
                *pBuf++ += ((pSrc[1] ^ 0x80) << 8) * volume;
                *pBuf++ += (((pSrc[2] + pSrc[0]) << 7) ^ 0x8000) * volume;
                pSrc++;
                *pBuf++ += (((pSrc[2] + pSrc[0]) << 7) ^ 0x8000) * volume;
                pSrc++;
            }

            *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * volume;
            *pBuf++ += ((pSrc[1] ^ 0x80) << 8) * volume;
            *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * volume;
            *pBuf++ += ((pSrc[1] ^ 0x80) << 8) * volume;
            break;

        case 0x10000:
            n <<= 1;
            while ( n-- ) {
                *pBuf++ += ((*pSrc++ ^ 0x80) << 8) * volume;
            }
            break;

        case 0x20000:
            while ( n-- )
            {
                *pBuf++ += ((*pSrc++ ^ 0x80) << 8) * volume;
                *pBuf++ += ((*pSrc++ ^ 0x80) << 8) * volume;
                pSrc += 2;
            }
            break;

        default:
        {
            uint32_t frac = 0;
            step <<= 1;
            while ( n-- )
            {
                *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * volume;
                *pBuf++ += ((pSrc[1] ^ 0x80) << 8) * volume;
                frac += step;
                pSrc += (int)frac >> 16;
                frac &= 0xFFFF;
            }
        } break;
    }
}

void J3DAPI mixer_Mono16(int16_t* pSrc, uint32_t count, uint32_t n, int offset, int left, int right)
{
    int32_t* pBuf = mixer_pBuffer + offset * 2;
    uint32_t step = (count << 16) / n;

    switch ( step )
    {
        case 0x8000:
            count--;
            // TODO: [BUG] The mean uses the sample two ahead (pSrc[2]) instead of the next one; a mono stream has no interleaved channel.
            while ( count-- )
            {
                *pBuf++ += pSrc[0] * left;
                *pBuf++ += pSrc[0] * right;
                *pBuf++ += ((pSrc[0] + pSrc[2]) >> 1) * left;
                *pBuf++ += ((pSrc[0] + pSrc[2]) >> 1) * right;
                pSrc++;
            }

            *pBuf++ += pSrc[0] * left;
            *pBuf++ += pSrc[0] * right;
            *pBuf++ += pSrc[0] * left;
            *pBuf++ += pSrc[0] * right;
            break;

        case 0x10000:
            while ( n-- )
            {
                *pBuf++ += pSrc[0] * left;
                *pBuf++ += pSrc[0] * right;
                pSrc++;
            }
            break;

        case 0x20000:
            while ( n-- )
            {
                *pBuf++ += pSrc[0] * left;
                *pBuf++ += pSrc[0] * right;
                pSrc += 2;
            }
            break;

        default:
        {
            uint32_t frac = 0;
            while ( n-- )
            {
                *pBuf++ += pSrc[0] * left;
                *pBuf++ += pSrc[0] * right;
                frac += step;
                pSrc += (int)frac >> 16;
                frac &= 0xFFFF;
            }
        } break;
    }
}

void J3DAPI mixer_Stereo16(int16_t* pSrc, uint32_t count, uint32_t n, int offset, int volume)
{
    int32_t* pBuf = mixer_pBuffer + offset * 2;
    uint32_t step = (count << 16) / n;

    switch ( step )
    {
        case 0x8000:
            count--;
            while ( count-- )
            {
                *pBuf++ += pSrc[0] * volume;
                *pBuf++ += pSrc[1] * volume;
                *pBuf++ += ((pSrc[2] + pSrc[0]) >> 1) * volume;
                pSrc++;
                *pBuf++ += ((pSrc[2] + pSrc[0]) >> 1) * volume;
                pSrc++;
            }

            *pBuf++ += pSrc[0] * volume;
            *pBuf++ += pSrc[1] * volume;
            *pBuf++ += pSrc[0] * volume;
            *pBuf++ += pSrc[1] * volume;
            break;

        case 0x10000:
            n <<= 1;
            while ( n-- ) {
                *pBuf++ += *pSrc++ * volume;
            }
            break;

        case 0x20000:
            while ( n-- )
            {
                *pBuf++ += *pSrc++ * volume;
                *pBuf++ += *pSrc++ * volume;
                pSrc += 2;
            }
            break;

        default:
        {
            uint32_t frac = 0;
            step <<= 1;
            while ( n-- )
            {
                *pBuf++ += pSrc[0] * volume;
                *pBuf++ += pSrc[1] * volume;
                frac += step;
                pSrc += (int)frac >> 16;
                frac &= 0xFFFF;
            }
        } break;
    }
}

void J3DAPI mixer_MonoU16(uint16_t* pSrc, uint32_t count, uint32_t n, int offset, int left, int right)
{
    int32_t* pBuf = mixer_pBuffer + offset * 2;
    uint32_t step = (count << 16) / n;
    int16_t s, t;

    switch ( step )
    {
        case 0x8000:
            count--;
            // TODO: [BUG] The mean uses the sample two ahead (pSrc[2]) instead of the next one; a mono stream has no interleaved channel.
            while ( count-- )
            {
                s = (int16_t)(pSrc[0] ^ 0x8000);
                t = (int16_t)(pSrc[2] ^ 0x8000);
                *pBuf++ += s * left;
                *pBuf++ += s * right;
                *pBuf++ += ((t + s) >> 1) * left;
                *pBuf++ += ((t + s) >> 1) * right;
                pSrc++;
            }

            s = (int16_t)(pSrc[0] ^ 0x8000);
            *pBuf++ += s * left;
            *pBuf++ += s * right;
            *pBuf++ += s * left;
            *pBuf++ += s * right;
            break;

        case 0x10000:
            while ( n-- )
            {
                s = (int16_t)*pSrc++;
                s = (int16_t)(s ^ 0x8000);
                *pBuf++ += s * left;
                *pBuf++ += s * right;
            }
            break;

        case 0x20000:
            while ( n-- )
            {
                s = (int16_t)(pSrc[0] ^ 0x8000);
                *pBuf++ += s * left;
                *pBuf++ += s * right;
                pSrc += 2;
            }
            break;

        default:
        {
            uint32_t frac = 0;
            while ( n-- )
            {
                s = (int16_t)(pSrc[0] ^ 0x8000);
                *pBuf++ += s * left;
                *pBuf++ += s * right;
                frac += step;
                pSrc += (int)frac >> 16;
                frac &= 0xFFFF;
            }
        } break;
    }
}

void J3DAPI mixer_StereoU16(uint16_t* pSrc, uint32_t count, uint32_t n, int offset, int volume)
{
    int32_t* pBuf = mixer_pBuffer + offset * 2;
    uint32_t step = (count << 16) / n;
    int16_t s0, s1, s2, s3;

    switch ( step )
    {
        case 0x8000:
            count--;
            while ( count-- )
            {
                s0 = (int16_t)pSrc[0];
                s1 = (int16_t)pSrc[1];
                s2 = (int16_t)pSrc[2];
                s3 = (int16_t)pSrc[3];
                s0 = (int16_t)(s0 ^ 0x8000);
                s1 = (int16_t)(s1 ^ 0x8000);
                s2 = (int16_t)(s2 ^ 0x8000);
                s3 = (int16_t)(s3 ^ 0x8000);
                *pBuf++ += s0 * volume;
                *pBuf++ += s1 * volume;
                *pBuf++ += ((s2 + s0) >> 1) * volume;
                *pBuf++ += ((s3 + s1) >> 1) * volume;
                pSrc += 2;
            }

            s0 = (int16_t)pSrc[0];
            s0 = (int16_t)(s0 ^ 0x8000);
            s1 = (int16_t)pSrc[1];
            s1 = (int16_t)(s1 ^ 0x8000);
            *pBuf++ += s0 * volume;
            *pBuf++ += s1 * volume;
            *pBuf++ += s0 * volume;
            *pBuf++ += s1 * volume;
            break;

        case 0x10000:
            n <<= 1;
            while ( n-- )
            {
                s0 = (int16_t)*pSrc++;
                s0 = (int16_t)(s0 ^ 0x8000);
                *pBuf++ += s0 * volume;
            }
            break;

        case 0x20000:
            while ( n-- )
            {
                s0 = (int16_t)*pSrc++;
                s1 = (int16_t)*pSrc;
                s0 = (int16_t)(s0 ^ 0x8000);
                s1 = (int16_t)(s1 ^ 0x8000);
                *pBuf++ += s0 * volume;
                *pBuf++ += s1 * volume;
                pSrc += 3;
            }
            break;

        default:
        {
            uint32_t frac = 0;
            step <<= 1;
            while ( n-- )
            {
                s0 = (int16_t)pSrc[0];
                s1 = (int16_t)pSrc[1];
                s0 = (int16_t)(s0 ^ 0x8000);
                s1 = (int16_t)(s1 ^ 0x8000);
                *pBuf++ += s0 * volume;
                *pBuf++ += s1 * volume;
                frac += step;
                pSrc += (int)frac >> 16;
                frac &= 0xFFFF;
            }
        } break;
    }
}

void J3DAPI mixer_MonoSwap16(int16_t* pSrc, uint32_t count, uint32_t n, int offset, int left, int right)
{
    int32_t* pBuf = mixer_pBuffer + offset * 2;
    uint32_t step = (count << 16) / n;

    switch ( step )
    {
        case 0x8000:
            count--;
            // TODO: [BUG] The mean uses the sample two ahead (pSrc[2]) instead of the next one; a mono stream has no interleaved channel.
            while ( count-- )
            {
                *pBuf++ += mixer_Swap16(pSrc[0]) * left;
                *pBuf++ += mixer_Swap16(pSrc[0]) * right;

                int16_t s = (int16_t)mixer_Swap16((pSrc[2] + pSrc[0]) >> 1);
                *pBuf++ += s * left;
                *pBuf++ += s * right;
                pSrc++;
            }

            int16_t s = (int16_t)mixer_Swap16(*pSrc);
            *pBuf++ += s * left;
            *pBuf++ += s * right;
            *pBuf++ += s * left;
            *pBuf++ += s * right;
            break;

        case 0x10000:
            while ( n-- )
            {
                *pBuf++ += mixer_Swap16(*pSrc) * left;
                *pBuf++ += mixer_Swap16(*pSrc) * right;
                pSrc++;
            }
            break;

        case 0x20000:
            while ( n-- )
            {
                *pBuf++ += mixer_Swap16(*pSrc) * left;
                *pBuf++ += mixer_Swap16(*pSrc) * right;
                pSrc += 2;
            }
            break;

        default:
        {
            uint32_t frac = 0;
            while ( n-- )
            {
                *pBuf++ += mixer_Swap16(*pSrc) * left;
                *pBuf++ += mixer_Swap16(*pSrc) * right;
                frac += step;
                pSrc += (int)frac >> 16;
                frac &= 0xFFFF;
            }
        } break;
    }
}

void J3DAPI mixer_StereoSwap16(int16_t* pSrc, uint32_t count, uint32_t n, int offset, int volume)
{
    int32_t* pBuf = mixer_pBuffer + offset * 2;
    uint32_t step = (count << 16) / n;

    switch ( step )
    {
        case 0x8000:
            count--;
            while ( count-- )
            {
                *pBuf++ += mixer_Swap16(pSrc[0]) * volume;
                *pBuf++ += mixer_Swap16(pSrc[1]) * volume;
                *pBuf++ += mixer_Swap16((pSrc[2] + pSrc[0]) >> 1) * volume;
                pSrc++;
                *pBuf++ += mixer_Swap16((pSrc[2] + pSrc[0]) >> 1) * volume;
                pSrc++;
            }

            *pBuf++ += mixer_Swap16(pSrc[0]) * volume;
            *pBuf++ += mixer_Swap16(pSrc[1]) * volume;
            *pBuf++ += mixer_Swap16(pSrc[0]) * volume;
            *pBuf++ += mixer_Swap16(pSrc[1]) * volume;
            break;

        case 0x10000:
            n <<= 1;
            while ( n-- ) {
                // TODO: [BUG] pSrc is never advanced: every output sample is the first input sample (unused path, mixer_Feed never swaps).
                *pBuf++ += mixer_Swap16(*pSrc) * volume;
            }
            break;

        case 0x20000:
            while ( n-- )
            {
                *pBuf++ += mixer_Swap16(*pSrc) * volume;
                pSrc++;
                *pBuf++ += mixer_Swap16(*pSrc) * volume;
                pSrc += 3;
            }
            break;

        default:
        {
            uint32_t frac = 0;
            step <<= 1;
            while ( n-- )
            {
                *pBuf++ += mixer_Swap16(pSrc[0]) * volume;
                *pBuf++ += mixer_Swap16(pSrc[1]) * volume;
                frac += step;
                pSrc += (int)frac >> 16;
                frac &= 0xFFFF;
            }
        } break;
    }
}

void J3DAPI mixer_Blit(LecBitmapHeader* pDst, MixerRect* pClip, int x, int y, LecBitmapHeader* pSrc, void* pPalette, void* pTable)
{
    MixerRect rect;
    if ( !pClip )
    {
        pClip = &rect;
        rect.x = 0;
        rect.y = 0;
        rect.width = pDst->width;
        rect.height = pDst->height;
    }

    int skipX = 0;
    int skipY = 0;
    int width = pSrc->width;
    int height = pSrc->height;

    int d = pClip->x - x;
    if ( d > 0 )
    {
        skipX = d;
        width -= d;
        x += d;
    }

    d = pClip->y - y;
    if ( d > 0 )
    {
        skipY = d;
        height -= d;
        y += d;
    }

    if ( x + width > pClip->x + pClip->width ) {
        width = pClip->x + pClip->width - x;
    }

    if ( y + height > pClip->y + pClip->height ) {
        height = pClip->y + pClip->height - y;
    }

    if ( width <= 0 || height <= 0 ) {
        return;
    }

    uint8_t* pOut = (uint8_t*)pDst->pPixels + pDst->pitch * y + pDst->pixelSize * x;
    uint8_t* pIn  = (uint8_t*)pSrc->pPixels + pSrc->pitch * skipY + pSrc->pixelSize * skipX;
    if ( pSrc->bpp == pDst->bpp && pSrc->colorMode == pDst->colorMode
        && ((pSrc->colorMode != 1 && pSrc->colorMode != 2)
            || (pSrc->format.redPosShift == pDst->format.redPosShift
                && pSrc->format.greenPosShift == pDst->format.greenPosShift
                && pSrc->format.bluePosShift == pDst->format.bluePosShift)) )
    {
        blit_Wrapper(pOut, pIn, 0, 0, 0, 0, pDst->pixelSize * width, height, pDst->pitch, pSrc->pitch);
        return;
    }

    uint8_t aTable[1024];
    if ( !pTable && pSrc->pixelSize == 1 )
    {
        decode_MakeTable(pSrc, pDst, aTable, pPalette);
        pTable = aTable;
    }

    int n = height;
    while ( n-- )
    {
        if ( decode_Convert(pIn, pSrc, width, pPalette, pOut, pDst, pTable) ) {
            break;
        }

        pOut += pDst->pitch;
        pIn += pSrc->pitch;
    }
}

void Mixer_InstallHooks(void)
{
    J3D_HOOKFUNC(mixer_Init);
    J3D_HOOKFUNC(mixer_Done);
    J3D_HOOKFUNC(mixer_Begin);
    J3D_HOOKFUNC(mixer_Feed);
    J3D_HOOKFUNC(mixer_Mix);
    J3D_HOOKFUNC(mixer_Setup);
    J3D_HOOKFUNC(mixer_Free);
    J3D_HOOKFUNC(mixer_Clear);
    J3D_HOOKFUNC(mixer_Add);
    J3D_HOOKFUNC(mixer_Output);
    J3D_HOOKFUNC(mixer_Mono8);
    J3D_HOOKFUNC(mixer_Stereo8);
    J3D_HOOKFUNC(mixer_Mono16);
    J3D_HOOKFUNC(mixer_Stereo16);
    J3D_HOOKFUNC(mixer_MonoU16);
    J3D_HOOKFUNC(mixer_StereoU16);
    J3D_HOOKFUNC(mixer_MonoSwap16);
    J3D_HOOKFUNC(mixer_StereoSwap16);
    J3D_HOOKFUNC(mixer_Blit);
}

void Mixer_ResetGlobals(void)
{}
