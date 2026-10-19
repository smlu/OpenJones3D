#ifndef SMUSH_INSANE_SOUND_DSND_H
#define SMUSH_INSANE_SOUND_DSND_H
#include <assert.h>
#include <j3dcore/j3d.h>
#include <smush/insane/lec.h>

/*
 * The `SNDDRIVER=insane_dsnd` interface Dsnd.c publishes (`dsnd_interface`) and the sound engine (Engine.c) subscribes to.
 * The driver's timer callback calls the engine's fill function once per block with the mixer's per-track feed
 * (`mixer_Feed`). Only the DirectSound object the game hands in (dsnd_Configure(5, pDSound)) is used; DirectSoundCreate and
 * a primary buffer only when none was given.
 */

J3D_EXTERN_C_START

#define DSND_BLOCKSIZE 0x1800 /* bytes per mixed block (0x600 stereo 16-bit frames) */

typedef void (*SoundFeed)(void* pSrc, int offset, int n, int bytes, int a5, int volume, int pan, int format);

typedef struct sSndDriver
{
    int  (*Start)(int rate, int tracks, void (*pfnFill)(int n, SoundFeed pfnFeed));
    void (*Stop)(void);
    void (*SetVolume)(int volume);
    int  (*GetVolume)(void);
    int  (*SetRate)(int rate);  /* Dsnd's table holds sound_Null, an empty `return 0` */
    int  (*GetRate)(void);
} SndDriver;
static_assert(sizeof(SndDriver) == 0x18, "sizeof(SndDriver) == 0x18");

int J3DAPI dsnd_Register(LecShare* pShare, void* pParam);
int J3DAPI dsnd_Configure(int tag, ...);
void dsnd_Unregister(void);
int dsnd_Startup(void);
void dsnd_Shutdown(void);
int J3DAPI dsnd_Start(int rate, int a2, void (*pfnFill)(int, SoundFeed));
void dsnd_Stop(void);
void J3DAPI dsnd_SetVolume(int volume);
int dsnd_GetVolume(void);
int dsnd_GetRate(void);
void J3DAPI dsnd_Callback(int index, int param);
int dsnd_Init(void);
void J3DAPI dsnd_Print(const char* pFormat, ...); // Folded onto 0x0046D9E0; no address of its own, never hook
void J3DAPI dsnd_OpenSecondary(int blocks);
void dsnd_Release(void);
int dsnd_GetPlayPos(void);

void Dsnd_InstallHooks(void);
void Dsnd_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_SOUND_DSND_H
