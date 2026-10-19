#ifndef SMUSH_INSANE_FILE_FRAMESTREAM_H
#define SMUSH_INSANE_FILE_FRAMESTREAM_H
#include <j3dcore/j3d.h>
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
int J3DAPI framestream_Tick(FrameStream* pFrames, void* a2, int a3, int a4, int a5, void* a6, int* a7, int* a8, int a9, int a10, int a11);
void J3DAPI framestream_Fetch(FrameStream* pFrames, int size);
int J3DAPI framestream_Continue(FrameStream* pFrames);
int J3DAPI framestream_Present(FrameStream* pFrames, void* pBitmap, int a3, int a4, int a5, void* a6, int* a7, int* a8, int a9, int a10, int a11);
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
