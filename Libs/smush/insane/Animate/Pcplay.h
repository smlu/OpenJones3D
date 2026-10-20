#ifndef SMUSH_INSANE_ANIMATE_PCPLAY_H
#define SMUSH_INSANE_ANIMATE_PCPLAY_H
#include <assert.h>
#include <j3dcore/j3d.h>
#include <smush/RTI/addresses.h>
#include <smush/insane/lec.h>
#include <smush/insane/animplay.h>
#include <smush/insane/Memory/Memory.h>
#include <smush/insane/File/Cdstream.h>
#include <smush/insane/Animate/Frame.h>

/*
 * Pcplay.c: the animation player (`ANIMPLAY=insane`, AnimPlayTable in animplay.h). It owns the player's tick and pacing timers
 * and drives Framestream.c; animplay_ops and the subscribed services are shared with Framestream.c.
 */

J3D_EXTERN_C_START

/* The services Pcplay.c subscribes to (slot roles inferred from the calls). */
typedef struct sAnimPlayDebugService
{
    void (*Log)(int a, int b);
    void* pfn04;
    void* pfn08;
    void* pfn0C;
    void* pfn10;
    void (*Show)(int a, int b, int c);
    void* pfn18;
    int  (*Query)(int a, int b);
} AnimPlayDebugService;
static_assert(sizeof(AnimPlayDebugService) == 0x20, "sizeof(AnimPlayDebugService) == 0x20");

/* The `DEBUG=insane_animplay` interface. */
typedef struct sAnimPlayDebug
{
    const char* (*Name)(int index);
    void (*SetFlags)(int flags);
    int  (*GetFlags)(void);
    int  speed;
} AnimPlayDebug;
static_assert(sizeof(AnimPlayDebug) == 0x10, "sizeof(AnimPlayDebug) == 0x10");

/* Pcplay.c's non-static state that Framestream.c reads. */
extern LecShare* animplay_pShare;
extern MemoryDebugInterface* animplay_pMemory;
extern AnimPlayDebugService* animplay_pDebug;
extern StreamerInterface* animplay_pStreamer;
extern AnimFrameTable* animplay_pFrame;
extern AnimPlayOps animplay_ops;

int J3DAPI animplay_Register(LecShare* pShare, void* pParam);
void animplay_Unregister(void);
int animplay_Startup(void);
void animplay_Shutdown(void);
int J3DAPI animplay_Play(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13);
int J3DAPI animplay_Create(AnimPlay** ppPlay, const char* pName, int rate, int ringSize, int frameSize, int flags);
int J3DAPI animplay_GetTime(AnimPlay* pPlay);
int J3DAPI animplay_Pace(AnimPlay* pPlay, void* a2, int a3, int a4, int a5, void* a6, int* a7, int* a8, int a9, int a10);
void J3DAPI animplay_Draw(AnimPlay* pPlay, void* pBitmap, int a3, int a4, int a5, void* pPalette, int a7, int a8, int a9);
void J3DAPI animplay_Destroy(AnimPlay** ppPlay);
void J3DAPI animplay_SetRate(AnimPlay* pPlay, int rate);
void J3DAPI animplay_Rewind(AnimPlay* pPlay);
int J3DAPI animplay_IsDone(AnimPlay* pPlay);
int J3DAPI animplay_Step(AnimPlay* pPlay, void* a2, int a3, int a4, int a5, void* a6, int* a7, int* a8, int a9, int a10);
void J3DAPI animplay_TickCallback(int index, int param);
void J3DAPI animplay_PaceCallback(int index, int param);
const char* J3DAPI animplay_DebugName(int index);
void J3DAPI animplay_SetDebug(int flags);
int animplay_GetDebug(void);

void Pcplay_InstallHooks(void);
void Pcplay_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_ANIMATE_PCPLAY_H
