// Framestream.c: the chunked frame stream the animation player drives (original 0x004EBDE0-0x004EC560).
#include <smush/insane/File/Framestream.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/addresses.h>
#include <smush/RTI/symbols.h>

#include <stddef.h>
#include <smush/insane/File/Cdstream.h>
#include <smush/insane/Animate/Frame.h>
#include <smush/insane/Animate/Pcplay.h>
#include <smush/insane/Bitmap/Blit.h>
#include <smush/insane/Bitmap/MemCopy.h>
#include <smush/insane/Bitmap/MemFill.h>

static int framestream_lastFrame;

void Framestream_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(framestream_Reset);
    J3D_HOOKFUNC(framestream_Tick);
    J3D_HOOKFUNC(framestream_Fetch);
    J3D_HOOKFUNC(framestream_Continue);
    J3D_HOOKFUNC(framestream_Present);
    J3D_HOOKFUNC(framestream_Header);
    J3D_HOOKFUNC(framestream_IsOpen);
    J3D_HOOKFUNC(framestream_Stall);
    J3D_HOOKFUNC(framestream_Open);
    J3D_HOOKFUNC(framestream_Seek);
    J3D_HOOKFUNC(framestream_Mode);
    J3D_HOOKFUNC(framestream_Copy);
    J3D_HOOKFUNC(framestream_CopyB);
}

void Framestream_ResetGlobals(void)
{
    framestream_lastFrame = 0;
}

int J3DAPI framestream_Reset(FrameStream* pFrames)
{
    animplay_pStreamer->Reset(pFrames->pStream);
    pFrames->outFlags   = 0;
    pFrames->frame      = 0;
    pFrames->tag        = FRAMESTREAM_TAG_NULL;
    pFrames->frameCount = 1;
    pFrames->open       = 0;
    pFrames->stalled    = 0;
    pFrames->mode       = 0;
    pFrames->hold       = 0;
    pFrames->paused     = 0;
    return 0;
}

void J3DAPI framestream_Close(FrameStream* pFrames)
{
    J3D_UNUSED(pFrames);
}

int J3DAPI framestream_Tick(FrameStream* pFrames, void* a2, int a3, int a4, int a5, void* a6, int* a7, int* a8, int a9, int a10, int a11)
{
    if ( pFrames->stalled )
    {
        pFrames->stalled = 0;
        return -1;
    }

    for ( ;; )
    {
        switch ( pFrames->tag )
        {
            case FRAMESTREAM_TAG_FLHD:
                framestream_Fetch(pFrames, pFrames->chunkSize);
                if ( (intptr_t)pFrames->pCur == -1 )
                {
                    return 5;
                }

                if ( (intptr_t)pFrames->pCur == -2 )
                {
                    return -2;
                }

                animplay_pFrame->Parse(pFrames->pCur, pFrames->chunkSize);
                pFrames->tag = FRAMESTREAM_TAG_NULL;
                continue;

            case FRAMESTREAM_TAG_ANNO:
                framestream_Fetch(pFrames, pFrames->chunkSize);
                if ( (intptr_t)pFrames->pCur == -1 )
                {
                    return 5;
                }

                if ( (intptr_t)pFrames->pCur == -2 )
                {
                    return -2;
                }

                pFrames->tag = FRAMESTREAM_TAG_NULL;
                continue;

            case FRAMESTREAM_TAG_FRME:
                framestream_Fetch(pFrames, pFrames->chunkSize);
                if ( (intptr_t)pFrames->pCur == -1 )
                {
                    return 5;
                }

                if ( (intptr_t)pFrames->pCur == -2 )
                {
                    return -2;
                }

                pFrames->tag = FRAMESTREAM_TAG_NULL;
                if ( pFrames->pCur == NULL )
                {
                    continue;
                }

                pFrames->frame++;
                if ( !pFrames->paused && framestream_Continue(pFrames) )
                {
                    if ( pFrames->frame >= pFrames->frameCount - 1 )
                    {
                        if ( animplay_pStreamer->IsActive(pFrames->pStream) != 0 || pFrames->hold != 0 )
                        {
                            pFrames->tag = FRAMESTREAM_TAG_NULL;
                        }
                        else
                        {
                            pFrames->tag = FRAMESTREAM_TAG_END;
                        }
                    }

                    continue;
                }

                framestream_Present(pFrames, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
                if ( pFrames->frame >= pFrames->frameCount - 1 && !pFrames->paused )
                {
                    if ( animplay_pStreamer->IsActive(pFrames->pStream) == 0 && pFrames->hold == 0 )
                    {
                        pFrames->tag = FRAMESTREAM_TAG_END;
                    }
                    else
                    {
                        pFrames->tag = FRAMESTREAM_TAG_NULL;
                    }
                }
                else if ( pFrames->mode == 1 )
                {
                    pFrames->tag = FRAMESTREAM_TAG_SWAP;
                }
                else if ( pFrames->mode == 3 )
                {
                    pFrames->mode = 1;
                }

                if ( pFrames->paused )
                {
                    return 2;
                }

                if ( pFrames->frame < 10 )
                {
                    return 1;
                }

                return (pFrames->frame >= pFrames->frameCount - 1) ? 4 : 0;

            case FRAMESTREAM_TAG_SHDR:
                framestream_Fetch(pFrames, pFrames->chunkSize);
                if ( (intptr_t)pFrames->pCur == -1 )
                {
                    return 5;
                }

                if ( (intptr_t)pFrames->pCur == -2 )
                {
                    return -2;
                }

                pFrames->tag = FRAMESTREAM_TAG_NULL;
                if ( pFrames->pCur != NULL )
                {
                    framestream_Header(pFrames, (const FrameHeader*)pFrames->pCur, a6, a8);
                }

                continue;

            case FRAMESTREAM_TAG_NULL:
                if ( pFrames->hold )
                {
                    pFrames->hold   = 0;
                    pFrames->paused = 1;
                }

                framestream_Fetch(pFrames, 8);
                if ( (intptr_t)pFrames->pCur == -1 )
                {
                    return 5;
                }

                if ( (intptr_t)pFrames->pCur == -2 )
                {
                    return -2;
                }

                if ( pFrames->pCur != NULL )
                {
                    const uint8_t* pBytes = (const uint8_t*)pFrames->pCur;

                    pFrames->tag       = (pBytes[0] << 24) | (pBytes[1] << 16) | (pBytes[2] << 8) | pBytes[3];
                    pFrames->chunkSize = (((pBytes[4] << 24) | (pBytes[5] << 16) | (pBytes[6] << 8) | pBytes[7]) + 1) & ~1;
                }

                continue;

            case FRAMESTREAM_TAG_END:
                pFrames->frame++;
                if ( framestream_Continue(pFrames) == 0 )
                {
                    framestream_Present(pFrames, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
                }

                if ( animplay_pStreamer->IsActive(pFrames->pStream) || (pFrames->hold && animplay_pFrame->Fetch(-8)) )
                {
                    pFrames->tag = FRAMESTREAM_TAG_NULL;
                }

                return 4;

            case FRAMESTREAM_TAG_WAIT:
                if ( animplay_pStreamer->IsActive(pFrames->pStream) || (pFrames->hold && animplay_pFrame->Fetch(-8)) )
                {
                    pFrames->tag = FRAMESTREAM_TAG_NULL;
                }

                return 5;

            case FRAMESTREAM_TAG_SWAP:
                framestream_Present(pFrames, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
                if ( pFrames->mode != 1 )
                {
                    if ( pFrames->mode == 3 )
                    {
                        pFrames->mode = 1;
                    }

                    pFrames->tag = FRAMESTREAM_TAG_NULL;
                }

                return 3;

            default:
                animplay_pShare->ReportError("Read invalid frame header %c%c%c%c -- CD may be dirty or damaged.",
                    pFrames->tag >> 24, pFrames->tag >> 16, pFrames->tag >> 8, pFrames->tag);
                return -2;
        }
    }
}

void J3DAPI framestream_Fetch(FrameStream* pFrames, int size)
{
    if ( pFrames->paused )
    {
        pFrames->pCur = animplay_pFrame->Fetch(size);
        animplay_pStreamer->Advance(pFrames->pStream, -1);
        if ( pFrames->pCur == NULL )
        {
            pFrames->paused   = 0;
            pFrames->seekLen  = 0;
            pFrames->seekBase = 0;
            pFrames->tag      = FRAMESTREAM_TAG_WAIT;
        }
    }
    else
    {
        pFrames->pCur = (void*)(intptr_t)animplay_pStreamer->Advance(pFrames->pStream, size);
        if ( (intptr_t)pFrames->pCur != -1 && pFrames->pCur == NULL )
        {
            pFrames->frame = -1;
            pFrames->open  = 0;
        }
    }
}

int J3DAPI framestream_Continue(FrameStream* pFrames)
{
    if ( !pFrames->paused )
    {
        if ( pFrames->open )
        {
            if ( pFrames->seekLen >= 0 && pFrames->frame > pFrames->seekBase + pFrames->seekLen )
            {
                return 1;
            }
        }
        else if ( pFrames->seekTo >= 0 )
        {
            if ( pFrames->frame < pFrames->seekTo )
            {
                return 1;
            }

            pFrames->seekTo = -1;
        }

        framestream_lastFrame = pFrames->frame;
    }

    return 0;
}

int J3DAPI framestream_Present(FrameStream* pFrames, void* pBitmap, int a3, int a4, int a5, void* a6, int* a7, int* a8, int a9, int a10, int a11)
{
    if ( !((a9 | pFrames->outFlags) & 4) )
    {
        blit_Fill(pBitmap, 0);
    }

    uint32_t result = animplay_pFrame->Decode(pBitmap, a3, a4, a5, a6, a7, a8, a9 | pFrames->outFlags, a10, a11, pFrames->pCur, pFrames->chunkSize);
    if ( result & 1 )
    {
        framestream_Stall(pFrames);
    }

    if ( result & 2 )
    {
        animplay_ops.Rewind(pFrames);
    }

    return 0;
}

void J3DAPI framestream_Header(FrameStream* pFrames, const FrameHeader* pHeader, void* pPalette, int* pFlags)
{
    pFrames->outFlags = pFrames->flagsIn;
    if ( !(pFrames->flagsIn & 0x2000) )
    {
        framestream_Copy(pPalette, pHeader->palette, 0x400);
        *pFlags |= 3;
    }

    pFrames->frameCount = pHeader->count;
    if ( pHeader->rate && !(pFrames->outFlags & 8) )
    {
        animplay_ops.SetRate(pFrames, 1000000 / pHeader->rate);
        animplay_ops.Rewind(pFrames);
    }
}

int J3DAPI framestream_IsOpen(FrameStream* pFrames)
{
    return pFrames->open;
}

void J3DAPI framestream_Stall(FrameStream* pFrames)
{
    animplay_pFrame->Reset();
    pFrames->stalled = 1;
}

int J3DAPI framestream_Open(FrameStream* pFrames, const char* pName, int flags, int seekLen, int seekTo, int hold)
{
    pFrames->hold    = hold;
    pFrames->flagsIn = flags;
    if ( animplay_pStreamer->SetFile(pFrames->pStream, pName, 8) )
    {
        return -1;
    }

    pFrames->open = 1;
    framestream_Seek(pFrames, seekLen, seekTo);
    return 0;
}

void J3DAPI framestream_Seek(FrameStream* pFrames, int length, int to)
{
    pFrames->seekBase = pFrames->frame;
    pFrames->seekTo   = to;
    pFrames->seekLen  = length;
}

int J3DAPI framestream_Mode(FrameStream* pFrames, int op)
{
    if ( op == 2 )
    {
        pFrames->mode = (pFrames->mode == 0);
    }
    else if ( op == 4 )
    {
        if ( pFrames->mode )
        {
            pFrames->mode = 3;
        }
    }
    else if ( op != 5 )
    {
        pFrames->mode = op;
    }

    return pFrames->mode;
}

void J3DAPI framestream_Copy(void* pDst, const void* pSrc, int size)
{
    blit_MemCopy(pDst, pSrc, size);
}

void J3DAPI framestream_CopyB(void* pDst, uint8_t value, int size)
{
    blit_MemCopyB(pDst, value, size);
}
