#ifndef SMUSH_INSANE_FILE_FRAMESTREAM_H
#define SMUSH_INSANE_FILE_FRAMESTREAM_H
#include <j3dcore/j3d.h>
#include <smush/RTI/addresses.h>
#include <smush/insane/animplay.h>

/*
 * The chunked frame stream (Framestream.c) the animation player (Pcplay.c) drives, and its two thin wrappers around the
 * MASM block copy and byte fill (MemCopy.asm, MemFill.asm) that the other modules call.
 */

J3D_EXTERN_C_START

typedef AnimPlay FrameStream;

/* The frame-stream chunk tags (FourCC) AnimPlay.tag takes. */
#define FRAMESTREAM_TAG_NULL 'null'
#define FRAMESTREAM_TAG_ANNO 'ANNO'
#define FRAMESTREAM_TAG_FRME 'FRME'
#define FRAMESTREAM_TAG_FLHD 'FLHD'
#define FRAMESTREAM_TAG_SHDR 'SHDR'
#define FRAMESTREAM_TAG_END  'end '
#define FRAMESTREAM_TAG_SWAP 'paws'
#define FRAMESTREAM_TAG_WAIT 'wait'

/* The values framestream_Tick returns (AnimPlay.state). */
typedef enum
{
    FRAMESTREAM_STATE_ERROR   = -2, /* stream error or invalid chunk */
    FRAMESTREAM_STATE_STALLED = -1, /* the previous frame stalled the decoder */
    FRAMESTREAM_STATE_FRAME   = 0,  /* a frame was presented */
    FRAMESTREAM_STATE_EARLY   = 1,  /* a frame was presented, still within the first FRAMESTREAM_EARLYFRAMES frames */
    FRAMESTREAM_STATE_PAUSED  = 2,  /* a frame was presented while paused */
    FRAMESTREAM_STATE_SWAP    = 3,  /* the pending frame was presented (swap chunk) */
    FRAMESTREAM_STATE_END     = 4,  /* the last frame was presented */
    FRAMESTREAM_STATE_WAIT    = 5   /* no data available yet */
} FrameStreamState;

#define FRAMESTREAM_EARLYFRAMES 10
#define FRAMESTREAM_CHUNKHDR_SIZE 8 /* chunk tag and size, both big-endian */

/* AnimPlay.flagsIn / outFlags and the flags framestream_Tick takes. */
#define FRAMESTREAM_FLAG_NOFILL    0x04  /* don't clear the bitmap before decoding */
#define FRAMESTREAM_FLAG_KEEPRATE  0x08  /* ignore the frame rate of the file header */
#define FRAMESTREAM_FLAG_NOPALETTE 0x2000 /* ignore the palette of the file header */

#define FRAMESTREAM_PALETTE_CHANGED 3 /* bits set in the palette flags when the header's palette is copied */

/* framestream_Mode operations and the modes they set. */
#define FRAMESTREAM_OP_TOGGLE 2
#define FRAMESTREAM_OP_ARM    4
#define FRAMESTREAM_OP_QUERY  5

#define FRAMESTREAM_MODE_OFF     0
#define FRAMESTREAM_MODE_SWAP    1 /* present on the swap chunk */
#define FRAMESTREAM_MODE_PENDING 3 /* becomes FRAMESTREAM_MODE_SWAP after the next present */

/* framestream_Present: bits of the frame decoder's result. */
#define FRAMESTREAM_DECODE_STALL  1
#define FRAMESTREAM_DECODE_REWIND 2

/* The animation file header chunk (FLHD), read in place from the stream. Serialized: packed. */
#pragma pack(push, 1)
typedef struct sFrameHeader
{
    int16_t version;        /* 0x00 */
    int16_t count;          /* 0x02 */
    char pad04[10];
    int rate;               /* 0x0E */
    char pad12[4];
    char palette[0x400];    /* 0x16 */
} FrameHeader;
#pragma pack(pop)
static_assert(sizeof(FrameHeader) == 0x416, "sizeof(FrameHeader) == 0x416");

int J3DAPI framestream_Reset(FrameStream* pFrames);
void J3DAPI framestream_Close(FrameStream* pFrames); // Folded onto 0x004EE700; no address of its own, never hook
int J3DAPI framestream_Tick(FrameStream* pFrames, void* pBitmap, int a3, int a4, int a5, void* pPalette, int* pOutA, int* pOutB, int flags, int a10, int a11);
void J3DAPI framestream_Fetch(FrameStream* pFrames, int size);
int J3DAPI framestream_Continue(FrameStream* pFrames);
int J3DAPI framestream_Present(FrameStream* pFrames, void* pBitmap, int a3, int a4, int a5, void* pPalette, int* pOutA, int* pOutB, int flags, int a10, int a11);
void J3DAPI framestream_Header(FrameStream* pFrames, const FrameHeader* pHeader, void* pPalette, int* pFlags);
int J3DAPI framestream_IsOpen(FrameStream* pFrames);
void J3DAPI framestream_Stall(FrameStream* pFrames);
int J3DAPI framestream_Open(FrameStream* pFrames, const char* pName, int flags, int seekLen, int seekTo, int hold);
void J3DAPI framestream_Seek(FrameStream* pFrames, int length, int to);
int J3DAPI framestream_Mode(FrameStream* pFrames, int op);
void J3DAPI framestream_Copy(void* pDst, const void* pSrc, int size);
void J3DAPI framestream_CopyB(void* pDst, uint8_t value, int size);

void Framestream_InstallHooks(void);
void Framestream_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_FILE_FRAMESTREAM_H
