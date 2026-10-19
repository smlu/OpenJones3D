#ifndef SMUSH_INSANE_ANIMPLAY_H
#define SMUSH_INSANE_ANIMPLAY_H
#include <assert.h>
#include <j3dcore/j3d.h>

/*
 * The 0x70-byte animation object (Pcplay.c allocates it, Framestream.c drives its chunk state machine at 0x0C-0x5C).
 * Member names are inferred from the accesses.
 */

J3D_EXTERN_C_START

typedef struct sStream Stream; /* Cdstream.h */

typedef struct sAnimPlay
{
    int hTick;              /* 0x00 timer index of the tick counter */
    int hPace;              /* 0x04 timer index of the frame pacer */
    uint32_t flags;         /* 0x08 */
    int outFlags;           /* 0x0C flagsIn copied by the header chunk */
    int done;               /* 0x10 */
    int ticks;              /* 0x14 */
    int lead;               /* 0x18 */
    int interval;           /* 0x1C */
    int interval2;          /* 0x20 */
    void* pCur;             /* 0x24 current chunk data */
    int frame;              /* 0x28 */
    int frameCount;         /* 0x2C */
    int chunkSize;          /* 0x30 */
    uint32_t tag;           /* 0x34 chunk state (FourCC) */
    int open;               /* 0x38 */
    int flagsIn;            /* 0x3C */
    int hold;               /* 0x40 */
    int paused;             /* 0x44 */
    int seekTo;             /* 0x48 */
    int seekBase;           /* 0x4C */
    int seekLen;            /* 0x50 */
    int stalled;            /* 0x54 */
    int mode;               /* 0x58 */
    int f5C;
    Stream* pStream;        /* 0x60 */
    int lipFlags;           /* 0x64 */
    int state;              /* 0x68 */
    int playing;            /* 0x6C */
} AnimPlay;
static_assert(sizeof(AnimPlay) == 0x70, "sizeof(AnimPlay) == 0x70");

/* The `ANIMPLAY=insane` table Pcplay.c publishes (`animplay_interface`); SmushPlay subscribes to it as `ANIMPLAY.app`. */
typedef struct sAnimPlayTable
{
    int  (*Play)(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13);
    int  (*Create)(AnimPlay** ppPlay, const char* pName, int rate, int ringSize, int frameSize, int flags);
    int  (*GetTime)(AnimPlay* pPlay);
    int  (*Pace)(AnimPlay* pPlay, void* a2, int a3, int a4, int a5, void* a6, int* a7, int* a8, int a9, int a10);
    void (*Draw)(AnimPlay* pPlay, void* pBitmap, int a3, int a4, int a5, void* pPalette, int a7, int a8, int a9);
    void (*Destroy)(AnimPlay** ppPlay);
    int  (*IsOpen)(AnimPlay* pPlay);
    void (*Stall)(AnimPlay* pPlay);
    int  (*Open)(AnimPlay* pPlay, const char* pName, int flags, int seekLen, int seekTo, int hold);
    int  (*Mode)(AnimPlay* pPlay, int op);
} AnimPlayTable;
static_assert(sizeof(AnimPlayTable) == 0x28, "sizeof(AnimPlayTable) == 0x28");

/* The callbacks the frame stream (Framestream.c) makes back into the player (`animplay_ops`, original .data 0x0054C200). */
typedef struct sAnimPlayOps
{
    void (*SetRate)(AnimPlay* pPlay, int rate);
    void (*Rewind)(AnimPlay* pPlay);
    int  (*IsDone)(AnimPlay* pPlay);
    void* pfn0C;
} AnimPlayOps;
static_assert(sizeof(AnimPlayOps) == 0x10, "sizeof(AnimPlayOps) == 0x10");

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_ANIMPLAY_H
