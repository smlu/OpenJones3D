#ifndef SMUSH_INSANE_SOUND_ENGINE_H
#define SMUSH_INSANE_SOUND_ENGINE_H
#include <assert.h>
#include <j3dcore/j3d.h>
#include <smush/RTI/addresses.h>
#include <smush/insane/lec.h>
#include <smush/insane/Sound/Dsnd.h>

/*
 * The `SOUND=insane` service Engine.c publishes (`sound_interface`): the wave decoder (D_wave.c) plays its packets through
 * it, the animation player (Pcplay.c, as `SOUND.lipsync`) pauses and resumes it, SmushPlay sets the volume.
 */

J3D_EXTERN_C_START

#define SOUND_NUMTRACKS 16
#define SOUND_NOTRACK   (-255)
#define SOUND_MIXSIZE   0x1800 /* bytes of a track's fade buffer */

typedef enum eSoundTrackState
{
    SOUND_TRACK_FREE     = 0,
    SOUND_TRACK_PLAYING  = 1,
    SOUND_TRACK_STARTING = 2,
    SOUND_TRACK_STARVED  = 3
} SoundTrackState;

/* Class of a track, in bits 6-7 of its priority; the three classes are switched by sound_Control (SOUND_TAG_MUSIC etc.) */
#define SOUND_PRIORITY_CLASSMASK 0xC0

typedef enum eSoundPriorityClass
{
    SOUND_CLASS_VOICE  = 0x00,
    SOUND_CLASS_MUSIC  = 0x40,
    SOUND_CLASS_EFFECT = 0x80
} SoundPriorityClass;

/* Tags of sound_Control and sound_Query */
typedef enum eSoundTag
{
    SOUND_TAG_VAR     = 1,
    SOUND_TAG_DUCK    = 2,
    SOUND_TAG_MUSIC   = 3,
    SOUND_TAG_EFFECTS = 4,
    SOUND_TAG_VOICE   = 5
} SoundTag;

/* Variable indices of the track script that address the engine's own values instead of a script variable */
#define SOUND_VAR_VOLUME      0xFF /* master volume */
#define SOUND_VAR_TRACKVOLUME 0xFE
#define SOUND_VAR_TRACKPAN    0xFD

/* Block types of a track's block script (first byte of a block; byte 1 is the block length - 2) */
typedef enum eSoundBlockType
{
    SOUND_BLOCK_SAMPLES     = 1,  /* sample range at the engine's rate */
    SOUND_BLOCK_JUMP        = 2,  /* jump if a variable is set */
    SOUND_BLOCK_SETVAR      = 3,
    SOUND_BLOCK_ADDVAR      = 4,
    SOUND_BLOCK_SAMPLESRATE = 6,  /* sample range with its own rate */
    SOUND_BLOCK_LOOP        = 7,
    SOUND_BLOCK_JUMPGT      = 8,  /* jump if a variable > value */
    SOUND_BLOCK_JUMPLT      = 9,
    SOUND_BLOCK_JUMPEQ      = 10,
    SOUND_BLOCK_JUMPNE      = 11
} SoundBlockType;

typedef struct sSoundInterface
{
    void (*Play)(uint8_t* pData, int id, int volume, int pan, int priority);
    void (*Stop)(int id);
    void (*Pause)(void);
    void (*Resume)(void);
    void (*StopAll)(void);
    int  (*GetVolume)(int id);
    void (*SetVolume)(int id, int volume);
    int  (*GetPan)(int id);
    void (*SetPan)(int id, int pan);
    int  (*GetMasterVolume)(void);
    void (*SetMasterVolume)(int volume);
    int  (*GetRate)(void);
    void (*SetRate)(int rate);
    int  (*GetPosition)(int id);
    int  (*Query)(int a1, int tag, int* pA, int* pB);
    int  (*Control)(int a1, int tag, int a3, int a4);
} SoundInterface;
static_assert(sizeof(SoundInterface) == 0x40, "sizeof(SoundInterface) == 0x40");

/* A track: the ring buffer a chunk is copied into and its playback state. The timer thread writes `state`. */
typedef struct sSoundTrack
{
    uint8_t* pBuf;                  /* 0x00 */
    uint8_t* pMix;                  /* 0x04: SOUND_MIXSIZE-byte fade buffer */
    uint8_t* pBlock;                /* 0x08: first block */
    uint8_t* pData;                 /* 0x0C: sample data */
    int size;                       /* 0x10 */
    int8_t volume;                  /* 0x14 */
    int8_t pan;                     /* 0x15 */
    volatile uint8_t state;         /* 0x16: SoundTrackState; set by the timer thread */
    uint8_t format;                 /* 0x17: SoundFormat */
    int priority;                   /* 0x18 */
    int id;                         /* 0x1C */
    int age;                        /* 0x20 */
    int ringSize;                   /* 0x24 */
    int written;                    /* 0x28 */
    int position;                   /* 0x2C */
    int total;                      /* 0x30 */
} SoundTrack;
static_assert(sizeof(SoundTrack) == 0x34, "sizeof(SoundTrack) == 0x34");

/* The callback's per-track playback cursor (local to sound_Callback; packed, unaligned fields). */
#pragma pack(push, 1)
typedef struct sSndChannel
{
    uint8_t* pBlock;                /* 0x00 */
    uint8_t* pData;                 /* 0x04 */
    int ringSize;                   /* 0x08 */
    int pos;                        /* 0x0C */
    int avail;                      /* 0x10 */
    int rate;                       /* 0x14 */
    uint8_t format;                 /* 0x18 */
    int prevState;                  /* 0x19 */
    int fadeRate;                   /* 0x1D */
    int fadeVolume;                 /* 0x21 */
    int fadeLeft;                   /* 0x25 */
    uint8_t fadeFormat;             /* 0x29 */
    int ramp;                       /* 0x2A */
    int loopStart;                  /* 0x2E */
    int loopLeft;                   /* 0x32 */
} SndChannel;
#pragma pack(pop)
static_assert(sizeof(SndChannel) == 0x36, "sizeof(SndChannel) == 0x36");

int J3DAPI sound_Register(LecShare* pShare, void* pParam);
void sound_Unregister(void);
int sound_Startup(void);
void sound_Shutdown(void);
int J3DAPI sound_Open(int rate, int size, ...);
int sound_Close(void);
int J3DAPI sound_AddTrack(int size, int minSize);
void J3DAPI sound_Play(uint8_t* pData, int id, int volume, int pan, int priority);
void J3DAPI sound_Stop(int id);
void sound_Pause(void);
void sound_Resume(void);
void sound_StopAll(void);
void J3DAPI sound_SetVolume(int id, int volume);
int J3DAPI sound_GetVolume(int id);
void J3DAPI sound_SetPan(int id, int pan);
int J3DAPI sound_GetPan(int id);
void J3DAPI sound_SetMasterVolume(int volume);
int sound_GetMasterVolume(void);
void J3DAPI sound_SetRate(int rate);
int sound_GetRate(void);
int J3DAPI sound_GetPosition(int id);
int J3DAPI sound_Control(int a1, int tag, int a3, int a4);
int J3DAPI sound_Query(int a1, int tag, int* pA, int* pB);
uint8_t J3DAPI sound_GetVar(uint8_t index);
void J3DAPI sound_SetVar(uint8_t index, uint8_t value);
void J3DAPI sound_StartVoc(uint8_t* pData, int len, int id, int volume, int pan, int priority);
void J3DAPI sound_StartStream(uint8_t* pData, int len, int id, int volume, int pan, int priority, int stream, int part, int parts, int a10);
void sound_SyncTimer(void);
void J3DAPI sound_Callback(int n, SoundFeed pfnFeed);
void J3DAPI sound_Advance(double ticks);
void J3DAPI sound_Copy(void* pDst, const void* pSrc, int n);
void J3DAPI sound_Fill(void* pDst, int value, int n);
int sound_Null(void); // Folded onto 0x004E74A0 (shared with other modules); never hook (see RTI/addresses.h)

void Engine_InstallHooks(void);
void Engine_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_SOUND_ENGINE_H
