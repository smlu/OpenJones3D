// Framestream.c: the chunked frame stream the animation player drives (original 0x004EBDE0-0x004EC560).
#include <smush/insane/File/Framestream.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

#include <stddef.h>
#include <smush/insane/File/Cdstream.h>
#include <smush/insane/Animate/Frame.h>
#include <smush/insane/Animate/Pcplay.h>
#include <smush/insane/Bitmap/Blit.h>
#include <smush/insane/Bitmap/MemCopy.h>
#include <smush/insane/Bitmap/MemFill.h>

static int framestream_lastFrame; // The last frame number framestream_Continue accepted (only written)

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
    pFrames->mode       = FRAMESTREAM_MODE_OFF;
    pFrames->hold       = 0;
    pFrames->paused     = 0;
    return 0;
}

void J3DAPI framestream_Close(FrameStream* pFrames)
{
    J3D_UNUSED(pFrames);
}

int J3DAPI framestream_Tick(FrameStream* pFrames, void* pBitmap, int a3, int a4, int a5, void* pPalette, int* pOutA, int* pOutB, int flags, int a10, int a11)
{
    if ( pFrames->stalled )
    {
        pFrames->stalled = 0;
        return FRAMESTREAM_STATE_STALLED;
    }

    for ( ;; )
    {
        switch ( pFrames->tag )
        {
            case FRAMESTREAM_TAG_FLHD:
                framestream_Fetch(pFrames, pFrames->chunkSize);
                if ( (intptr_t)pFrames->pCur == STREAMER_NOTREADY )
                {
                    return FRAMESTREAM_STATE_WAIT;
                }

                if ( (intptr_t)pFrames->pCur == STREAMER_BADSIZE )
                {
                    return FRAMESTREAM_STATE_ERROR;
                }

                animplay_pFrame->Parse(pFrames->pCur, pFrames->chunkSize);
                pFrames->tag = FRAMESTREAM_TAG_NULL;
                continue;

            case FRAMESTREAM_TAG_ANNO:
                framestream_Fetch(pFrames, pFrames->chunkSize);
                if ( (intptr_t)pFrames->pCur == STREAMER_NOTREADY )
                {
                    return FRAMESTREAM_STATE_WAIT;
                }

                if ( (intptr_t)pFrames->pCur == STREAMER_BADSIZE )
                {
                    return FRAMESTREAM_STATE_ERROR;
                }

                pFrames->tag = FRAMESTREAM_TAG_NULL;
                continue;

            case FRAMESTREAM_TAG_FRME:
                framestream_Fetch(pFrames, pFrames->chunkSize);
                if ( (intptr_t)pFrames->pCur == STREAMER_NOTREADY )
                {
                    return FRAMESTREAM_STATE_WAIT;
                }

                if ( (intptr_t)pFrames->pCur == STREAMER_BADSIZE )
                {
                    return FRAMESTREAM_STATE_ERROR;
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

                framestream_Present(pFrames, pBitmap, a3, a4, a5, pPalette, pOutA, pOutB, flags, a10, a11);
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
                else if ( pFrames->mode == FRAMESTREAM_MODE_SWAP )
                {
                    pFrames->tag = FRAMESTREAM_TAG_SWAP;
                }
                else if ( pFrames->mode == FRAMESTREAM_MODE_PENDING )
                {
                    pFrames->mode = FRAMESTREAM_MODE_SWAP;
                }

                if ( pFrames->paused )
                {
                    return FRAMESTREAM_STATE_PAUSED;
                }

                if ( pFrames->frame < FRAMESTREAM_EARLYFRAMES )
                {
                    return FRAMESTREAM_STATE_EARLY;
                }

                return (pFrames->frame >= pFrames->frameCount - 1) ? FRAMESTREAM_STATE_END : FRAMESTREAM_STATE_FRAME;

            case FRAMESTREAM_TAG_SHDR:
                framestream_Fetch(pFrames, pFrames->chunkSize);
                if ( (intptr_t)pFrames->pCur == STREAMER_NOTREADY )
                {
                    return FRAMESTREAM_STATE_WAIT;
                }

                if ( (intptr_t)pFrames->pCur == STREAMER_BADSIZE )
                {
                    return FRAMESTREAM_STATE_ERROR;
                }

                pFrames->tag = FRAMESTREAM_TAG_NULL;
                if ( pFrames->pCur != NULL )
                {
                    framestream_Header(pFrames, (const FrameHeader*)pFrames->pCur, pPalette, pOutB);
                }

                continue;

            case FRAMESTREAM_TAG_NULL:
                if ( pFrames->hold )
                {
                    pFrames->hold   = 0;
                    pFrames->paused = 1;
                }

                framestream_Fetch(pFrames, FRAMESTREAM_CHUNKHDR_SIZE);
                if ( (intptr_t)pFrames->pCur == STREAMER_NOTREADY )
                {
                    return FRAMESTREAM_STATE_WAIT;
                }

                if ( (intptr_t)pFrames->pCur == STREAMER_BADSIZE )
                {
                    return FRAMESTREAM_STATE_ERROR;
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
                    framestream_Present(pFrames, pBitmap, a3, a4, a5, pPalette, pOutA, pOutB, flags, a10, a11);
                }

                if ( animplay_pStreamer->IsActive(pFrames->pStream) || (pFrames->hold && animplay_pFrame->Fetch(-8)) )
                {
                    pFrames->tag = FRAMESTREAM_TAG_NULL;
                }

                return FRAMESTREAM_STATE_END;

            case FRAMESTREAM_TAG_WAIT:
                if ( animplay_pStreamer->IsActive(pFrames->pStream) || (pFrames->hold && animplay_pFrame->Fetch(-8)) )
                {
                    pFrames->tag = FRAMESTREAM_TAG_NULL;
                }

                return FRAMESTREAM_STATE_WAIT;

            case FRAMESTREAM_TAG_SWAP:
                framestream_Present(pFrames, pBitmap, a3, a4, a5, pPalette, pOutA, pOutB, flags, a10, a11);
                if ( pFrames->mode != FRAMESTREAM_MODE_SWAP )
                {
                    if ( pFrames->mode == FRAMESTREAM_MODE_PENDING )
                    {
                        pFrames->mode = FRAMESTREAM_MODE_SWAP;
                    }

                    pFrames->tag = FRAMESTREAM_TAG_NULL;
                }

                return FRAMESTREAM_STATE_SWAP;

            default:
                animplay_pShare->ReportError("Read invalid frame header %c%c%c%c -- CD may be dirty or damaged.",
                    pFrames->tag >> 24, pFrames->tag >> 16, pFrames->tag >> 8, pFrames->tag);
                return FRAMESTREAM_STATE_ERROR;
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
        if ( (intptr_t)pFrames->pCur != STREAMER_NOTREADY && pFrames->pCur == NULL )
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

int J3DAPI framestream_Present(FrameStream* pFrames, void* pBitmap, int a3, int a4, int a5, void* pPalette, int* pOutA, int* pOutB, int flags, int a10, int a11)
{
    if ( !((flags | pFrames->outFlags) & FRAMESTREAM_FLAG_NOFILL) )
    {
        blit_Fill(pBitmap, 0);
    }

    uint32_t result = animplay_pFrame->Decode(pBitmap, a3, a4, a5, pPalette, pOutA, pOutB, flags | pFrames->outFlags, a10, a11, pFrames->pCur, pFrames->chunkSize);
    if ( result & FRAMESTREAM_DECODE_STALL )
    {
        framestream_Stall(pFrames);
    }

    if ( result & FRAMESTREAM_DECODE_REWIND )
    {
        animplay_ops.Rewind(pFrames);
    }

    return 0;
}

void J3DAPI framestream_Header(FrameStream* pFrames, const FrameHeader* pHeader, void* pPalette, int* pFlags)
{
    pFrames->outFlags = pFrames->flagsIn;
    if ( !(pFrames->flagsIn & FRAMESTREAM_FLAG_NOPALETTE) )
    {
        framestream_Copy(pPalette, pHeader->palette, sizeof(pHeader->palette));
        *pFlags |= FRAMESTREAM_PALETTE_CHANGED;
    }

    pFrames->frameCount = pHeader->count;
    if ( pHeader->rate && !(pFrames->outFlags & FRAMESTREAM_FLAG_KEEPRATE) )
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
    if ( animplay_pStreamer->SetFile(pFrames->pStream, pName, FRAMESTREAM_CHUNKHDR_SIZE) )
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
    if ( op == FRAMESTREAM_OP_TOGGLE )
    {
        pFrames->mode = (pFrames->mode == FRAMESTREAM_MODE_OFF);
    }
    else if ( op == FRAMESTREAM_OP_ARM )
    {
        if ( pFrames->mode )
        {
            pFrames->mode = FRAMESTREAM_MODE_PENDING;
        }
    }
    else if ( op != FRAMESTREAM_OP_QUERY )
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
