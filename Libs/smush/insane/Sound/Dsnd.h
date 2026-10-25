#ifndef SMUSH_INSANE_SOUND_DSND_H
#define SMUSH_INSANE_SOUND_DSND_H
#include <assert.h>
#include <j3dcore/j3d.h>
#include <smush/RTI/addresses.h>
#include <smush/insane/lec.h>

/*
 * The `SNDDRIVER=insane_dsnd` interface Dsnd.c publishes (`dsnd_interface`) and the sound engine (Engine.c) subscribes to.
 * The driver's timer callback calls the engine's fill function once per block with the mixer's per-track feed
 * (`mixer_Feed`). Only the DirectSound object the game hands in (dsnd_Configure(5, pDSound)) is used; DirectSoundCreate and
 * a primary buffer only when none was given.
 */

J3D_EXTERN_C_START

#define DSND_BLOCKSIZE   0x1800 /* bytes per mixed block (0x600 stereo 16-bit frames) */
#define DSND_BLOCKFRAMES 0x600  /* stereo 16-bit frames per mixed block */
#define DSND_TICKRATE    1193180 /* ticks per second of the timer's clock */

/* Sample format of a track and of a feed (the engine's frame sizes are 1, 2, 2, 4, 2, 4 bytes) */
typedef enum eSoundFormat
{
    SOUND_FORMAT_MONO8     = 0,
    SOUND_FORMAT_STEREO8   = 1,
    SOUND_FORMAT_MONO16U   = 2, /* unsigned 16-bit */
    SOUND_FORMAT_STEREO16U = 3,
    SOUND_FORMAT_MONO16    = 4,
    SOUND_FORMAT_STEREO16  = 5,
    SOUND_FORMAT_COUNT
} SoundFormat;

/* Tags of dsnd_Configure */
typedef enum eDsndConfig
{
    DSND_CONFIG_END    = 0,
    DSND_CONFIG_1      = 1, /* Stored, never read */
    DSND_CONFIG_2      = 2, /* Stored, never read */
    DSND_CONFIG_3      = 3, /* Stored, never read */
    DSND_CONFIG_4      = 4, /* Stored, never read */
    DSND_CONFIG_DSOUND = 5  /* The game's DirectSound object */
} DsndConfig;

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
