// Pcplay.c: the animation player (`ANIMPLAY=insane`) (original 0x004E3810-0x004E4230).
#include <smush/insane/Animate/Pcplay.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

#include <stddef.h>
#include <smush/insane/File/Framestream.h>
#include <smush/insane/Timer/Timer.h>
#include <smush/insane/Sound/Engine.h>
#include <smush/insane/Bitmap/Bitmap.h>

static LecModule animplay_module = { animplay_Register, animplay_Unregister, animplay_Startup, animplay_Shutdown };
static AnimPlayTable animplay_interface = {
    animplay_Play, animplay_Create, animplay_GetTime, animplay_Pace, animplay_Draw, animplay_Destroy,
    framestream_IsOpen, framestream_Stall, framestream_Open, framestream_Mode
};
static AnimPlayDebug animplay_debug = { animplay_DebugName, animplay_SetDebug, animplay_GetDebug, 8 };

AnimPlayOps animplay_ops = { animplay_SetRate, animplay_Rewind, animplay_IsDone, NULL };

LecShare* animplay_pShare;
MemoryDebugInterface* animplay_pMemory;
AnimPlayDebugService* animplay_pDebug;
StreamerInterface* animplay_pStreamer;
AnimFrameTable* animplay_pFrame;

static void* animplay_pCookie;
static BitmapInterface* animplay_pBitmap;
static int animplay_bitmapSize;
static int animplay_rate;
static TimerInterface* animplay_timer;
static SoundInterface* animplay_pVoice;
static int animplay_debugMode;
static int animplay_lipFlag;

void Pcplay_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(animplay_Register);
    J3D_HOOKFUNC(animplay_Unregister);
    J3D_HOOKFUNC(animplay_Startup);
    J3D_HOOKFUNC(animplay_Shutdown);
    J3D_HOOKFUNC(animplay_Play);
    J3D_HOOKFUNC(animplay_Create);
    J3D_HOOKFUNC(animplay_GetTime);
    J3D_HOOKFUNC(animplay_Pace);
    J3D_HOOKFUNC(animplay_Draw);
    J3D_HOOKFUNC(animplay_Destroy);
    J3D_HOOKFUNC(animplay_SetRate);
    J3D_HOOKFUNC(animplay_Rewind);
    J3D_HOOKFUNC(animplay_IsDone);
    J3D_HOOKFUNC(animplay_Step);
    J3D_HOOKFUNC(animplay_TickCallback);
    J3D_HOOKFUNC(animplay_PaceCallback);
    J3D_HOOKFUNC(animplay_DebugName);
    J3D_HOOKFUNC(animplay_SetDebug);
    J3D_HOOKFUNC(animplay_GetDebug);
}

void Pcplay_ResetGlobals(void)
{
    animplay_ops.SetRate = animplay_SetRate;
    animplay_ops.Rewind  = animplay_Rewind;
    animplay_ops.IsDone  = animplay_IsDone;
    animplay_ops.pfn0C   = NULL;
    animplay_debug.speed = 8;

    animplay_pShare     = NULL;
    animplay_pMemory    = NULL;
    animplay_pDebug     = NULL;
    animplay_pStreamer  = NULL;
    animplay_pFrame     = NULL;
    animplay_pCookie    = NULL;
    animplay_pBitmap    = NULL;
    animplay_bitmapSize = 0;
    animplay_rate       = 0;
    animplay_timer      = NULL;
    animplay_pVoice     = NULL;
    animplay_debugMode  = 0;
    animplay_lipFlag    = 0;
}

int J3DAPI animplay_Register(LecShare* pShare, void* pParam)
{
    animplay_pShare  = pShare;
    animplay_pCookie = pParam;
    pShare->Publish("LECMODULE=insane_animplay", &animplay_module);
    animplay_pShare->Publish("ANIMPLAY=insane", &animplay_interface);
    animplay_pShare->Publish("DEBUG=insane_animplay", &animplay_debug);
    animplay_bitmapSize = 0x4B000;
    animplay_rate       = 1000;
    return 0;
}

void animplay_Unregister(void)
{
    animplay_pShare->Unpublish("ANIMPLAY=insane");
    animplay_pShare->Unpublish("LECMODULE=insane_animplay");
    animplay_pShare->Unpublish("DEBUG=insane_animplay");
}

int animplay_Startup(void)
{
    animplay_pMemory   = (MemoryDebugInterface*)animplay_pShare->Subscribe("MEMORY.workspace", animplay_pCookie);
    animplay_pDebug    = (AnimPlayDebugService*)animplay_pShare->Subscribe("VIRTHERC.debug", animplay_pCookie);
    animplay_pStreamer = (StreamerInterface*)animplay_pShare->Subscribe("STREAMER.spooler", animplay_pCookie);
    animplay_pFrame    = (AnimFrameTable*)animplay_pShare->Subscribe("ANIMFRAME.render", animplay_pCookie);
    animplay_pBitmap   = (BitmapInterface*)animplay_pShare->Subscribe("BITMAP.buffers", animplay_pCookie);
    animplay_timer     = (TimerInterface*)animplay_pShare->Subscribe("TIMER.clocks", animplay_pCookie);
    animplay_pVoice    = (SoundInterface*)animplay_pShare->Subscribe("SOUND.lipsync", animplay_pCookie);
    if ( animplay_pMemory == NULL )
    {
        animplay_pShare->ReportError("AnimPlay Module: Cannot allocate memory services.");
        return -1;
    }

    if ( animplay_pStreamer == NULL )
    {
        animplay_pShare->ReportError("AnimPlay Module: Cannot allocate streamer services.");
        return -1;
    }

    if ( animplay_pFrame == NULL )
    {
        animplay_pShare->ReportError("AnimPlay Module: Cannot allocate animframe services.");
        return -1;
    }

    if ( animplay_pBitmap == NULL )
    {
        animplay_pShare->ReportError("AnimPlay Module: Cannot allocate bitmap services.");
        return -1;
    }

    if ( animplay_timer == NULL )
    {
        animplay_pShare->ReportError("AnimPlay Module: Cannot allocate timer services.");
        return -1;
    }

    return 0;
}

void animplay_Shutdown(void)
{
    animplay_pShare->Unsubscribe(animplay_pMemory);
    animplay_pShare->Unsubscribe(animplay_pStreamer);
    animplay_pShare->Unsubscribe(animplay_pFrame);
    animplay_pShare->Unsubscribe(animplay_pBitmap);
    animplay_pShare->Unsubscribe(animplay_timer);
    if ( animplay_pDebug )
    {
        animplay_pShare->Unsubscribe(animplay_pDebug);
    }

    if ( animplay_pVoice )
    {
        animplay_pShare->Unsubscribe(animplay_pVoice);
    }
}

int J3DAPI animplay_Play(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13)
{
    AnimPlay* pPlay;
    LecBitmapHeader* pBitmap;
    char aPalette[0x400];
    int rc = 0;

    if ( a2 == 0 )
    {
        a2 = 15;
    }

    if ( animplay_pBitmap->Create(&pBitmap, a6, a7, -1) )
    {
        animplay_pShare->ReportError("Cannot allocate render bitmap.");
        return -1;
    }

    int loops = a11;
    if ( loops == 0 )
    {
        loops = -1;
    }

    int cnt = loops - 1;
    if ( loops != 0 )
    {
        do
        {
            rc = animplay_Create(&pPlay, (const char*)(intptr_t)a1, a2, a12, a13, a3);
            if ( rc == 0 )
            {
                int r;
                do
                {
                    int v2 = 0;
                    int v1 = 0;
                    r = animplay_Pace(pPlay, pBitmap, 0, 0, 0, aPalette, &v1, &v2, a8, a9);
                    if ( r == 0 || r == 2 || r == 1 )
                    {
                        if ( a10 )
                        {
                            int bStalled = pPlay->stalled;

                            ((void (__cdecl*)(AnimPlay*, void*, void*, int, int))(intptr_t)a10)(pPlay, pBitmap, aPalette, v1, v2);
                            if ( bStalled == 0 && pPlay->stalled != 0 )
                            {
                                cnt = 0;
                                break;
                            }
                        }

                        animplay_Draw(pPlay, pBitmap, 0, a4, a5, aPalette, v1, v2, a3);
                    }
                } while ( r >= 0 && r != 4 );

                animplay_Destroy(&pPlay);
            }
        } while ( cnt-- != 0 );
    }

    animplay_pBitmap->Destroy(&pBitmap);
    return rc;
}

int J3DAPI animplay_Create(AnimPlay** ppPlay, const char* pName, int rate, int ringSize, int frameSize, int flags)
{
    AnimPlay* pPlay = animplay_pMemory->Alloc(sizeof(AnimPlay), INSANE_FILE, __LINE__);
    if ( pPlay )
    {
        if ( animplay_pStreamer->Create(&pPlay->pStream, ringSize, frameSize) == 0 )
        {
            pPlay->interval  = 1193180 / rate;
            pPlay->flags     = flags;
            pPlay->lead      = 0;
            pPlay->lipFlags  = 0;
            pPlay->state     = 0;
            pPlay->playing   = 0;
            pPlay->interval2 = pPlay->interval;
            *ppPlay          = pPlay;
            if ( framestream_Reset(pPlay) == 0 )
            {
                if ( framestream_Open(*ppPlay, pName, flags, -1, -1, 0) == 0 )
                {
                    pPlay->hTick = animplay_timer->Alloc(animplay_TickCallback, 1, 2, (int)(intptr_t)pPlay);
                    if ( pPlay->hTick )
                    {
                        pPlay->hPace = animplay_timer->Alloc(animplay_PaceCallback, pPlay->interval, 8, (int)(intptr_t)pPlay);
                        if ( pPlay->hPace )
                        {
                            pPlay->done = 0;
                            return 0;
                        }

                        animplay_timer->Free(pPlay->hTick);
                    }
                }

                framestream_Close(*ppPlay);
            }

            animplay_pStreamer->Destroy(&pPlay->pStream);
        }

        animplay_pMemory->Free(pPlay, INSANE_FILE, __LINE__);
    }

    *ppPlay = NULL;
    return -1;
}

int J3DAPI animplay_GetTime(AnimPlay* pPlay)
{
    return pPlay->interval2 * 1000 / 1193180;
}

int J3DAPI animplay_Pace(AnimPlay* pPlay, void* a2, int a3, int a4, int a5, void* a6, int* a7, int* a8, int a9, int a10)
{
    if ( pPlay->done == 0 )
    {
        int percent = animplay_pStreamer->Percent(pPlay->pStream);
        if ( percent > 60 && animplay_debug.speed > 4 )
        {
            animplay_debug.speed--;
        }
        else if ( percent < 60 && animplay_debug.speed < 12 )
        {
            animplay_debug.speed++;
        }

        animplay_pStreamer->Fill(pPlay->pStream, animplay_debug.speed << 11);
        return animplay_Step(pPlay, a2, a3, a4, a5, a6, a7, a8, a9, a10);
    }

    return -1;
}

void J3DAPI animplay_Draw(AnimPlay* pPlay, void* pBitmap, int a3, int a4, int a5, void* pPalette, int a7, int a8, int a9)
{
    J3D_UNUSED(pPlay);
    animplay_pFrame->Draw(pBitmap, a3, a4, a5, pPalette, a7, a8, a9);
}

void J3DAPI animplay_Destroy(AnimPlay** ppPlay)
{
    AnimPlay* pPlay = *ppPlay;

    animplay_timer->Free(pPlay->hTick);
    animplay_timer->Free(pPlay->hPace);
    if ( animplay_lipFlag && animplay_pVoice )
    {
        animplay_pVoice->Resume();
    }

    framestream_Close(pPlay);
    animplay_pStreamer->Destroy(&pPlay->pStream);
    if ( *ppPlay )
    {
        animplay_pMemory->Free(*ppPlay, INSANE_FILE, __LINE__);
    }

    *ppPlay = NULL;
    animplay_pFrame->Reset();
}

void J3DAPI animplay_SetRate(AnimPlay* pPlay, int rate)
{
    int interval = 1193180 / rate;

    pPlay->interval  = interval;
    pPlay->interval2 = interval;
    animplay_timer->SetInterval(pPlay->hTick, interval);
    animplay_timer->SetInterval(pPlay->hPace, pPlay->interval);
}

void J3DAPI animplay_Rewind(AnimPlay* pPlay)
{
    pPlay->lead = 0;
    animplay_timer->SetInterval(pPlay->hTick, pPlay->interval2);
}

int J3DAPI animplay_IsDone(AnimPlay* pPlay)
{
    if ( pPlay->state == 5 || pPlay->ticks )
    {
        return 1;
    }

    return 0;
}

int J3DAPI animplay_Step(AnimPlay* pPlay, void* a2, int a3, int a4, int a5, void* a6, int* a7, int* a8, int a9, int a10)
{
    if ( pPlay->state == 5 )
    {
        pPlay->state = framestream_Tick(pPlay, a2, a3, a4, a5, a6, a7, a8, pPlay->flags, a9, a10);
        if ( pPlay->state < 0 )
        {
            pPlay->done = 1;
        }

        return pPlay->state;
    }

    if ( pPlay->ticks )
    {
        animplay_pStreamer->Percent(pPlay->pStream);
        pPlay->ticks = 0;
        pPlay->lead--;
        pPlay->interval2 = pPlay->interval - pPlay->lead * 79545;
        if ( pPlay->interval2 < pPlay->interval * 3 / 4 )
        {
            pPlay->interval2 = pPlay->interval * 3 / 4;
        }

        if ( pPlay->interval2 > pPlay->interval * 4 / 3 )
        {
            pPlay->interval2 = pPlay->interval * 4 / 3;
        }

        if ( pPlay->lead >= 3 )
        {
            if ( !(pPlay->lipFlags & 0x80) )
            {
                pPlay->lipFlags |= 0x80;
                pPlay->lipFlags++;
            }
        }
        else if ( pPlay->lead <= 0 )
        {
            if ( (pPlay->lipFlags & 0x80) && (pPlay->lipFlags & 0x7F) < 5 )
            {
                pPlay->lipFlags &= ~0x80;
            }
        }

        animplay_timer->SetInterval(pPlay->hTick, pPlay->interval2);
        uint32_t flags = ((pPlay->lipFlags & 0x80) && !(pPlay->flags & 2)) ? (pPlay->flags | 0x8000) : pPlay->flags;
        pPlay->state   = framestream_Tick(pPlay, a2, a3, a4, a5, a6, a7, a8, flags, a9, a10);
        if ( animplay_pDebug && (animplay_debugMode & 0xC0000000) )
        {
            animplay_pDebug->Show(0, 10, animplay_pDebug->Query(4, 32));
            animplay_pDebug->Log(0, 10);
        }

        if ( pPlay->state < 0 )
        {
            pPlay->done = 1;
        }

        if ( pPlay->state == 1 )
        {
            pPlay->playing  = 1;
            pPlay->lipFlags = 0;
        }

        return pPlay->state;
    }

    return 6;
}

void J3DAPI animplay_TickCallback(int index, int param)
{
    J3D_UNUSED(index);
    ((AnimPlay*)(intptr_t)param)->ticks++;
}

void J3DAPI animplay_PaceCallback(int index, int param)
{
    J3D_UNUSED(index);
    AnimPlay* pPlay = (AnimPlay*)(intptr_t)param;
    if ( pPlay->playing )
    {
        if ( (pPlay->flags & 1) && animplay_lipFlag == 0 )
        {
            if ( pPlay->lead >= 7 )
            {
                animplay_lipFlag = 1;
                if ( animplay_pVoice )
                {
                    animplay_pVoice->Pause();
                }
            }
        }
        else if ( animplay_lipFlag )
        {
            if ( pPlay->lead < 0 )
            {
                animplay_lipFlag = 0;
                if ( animplay_pVoice )
                {
                    animplay_pVoice->Resume();
                }
            }
        }

        if ( pPlay->lead < 20 && animplay_lipFlag == 0 )
        {
            pPlay->lead++;
        }
    }
}

const char* J3DAPI animplay_DebugName(int index)
{
    switch ( index )
    {
        case 0:
            return "Insane AnimPlay";

        case 1:
            return "Insane AnimPlay Debug Interface";

        case 2:
            return "This is the debugging interface for the Insane Animation module.";
    }

    return "";
}

void J3DAPI animplay_SetDebug(int flags)
{
    animplay_debugMode = flags;
}

int animplay_GetDebug(void)
{
    return animplay_debugMode;
}
