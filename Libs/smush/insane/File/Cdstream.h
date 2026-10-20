#ifndef SMUSH_INSANE_FILE_CDSTREAM_H
#define SMUSH_INSANE_FILE_CDSTREAM_H
#include <assert.h>
#include <j3dcore/j3d.h>
#include <smush/RTI/addresses.h>
#include <smush/insane/lec.h>
#include <smush/insane/animplay.h>
#include <smush/insane/filio.h>
#include <smush/insane/Memory/Memory.h>

/*
 * The `STREAMER=insane_streamer` service Cdstream.c publishes (`streamer_interface`): the animation player (Pcplay.c)
 * creates and fills a stream through it, the frame stream (Framestream.c) advances it. `Advance` returns the frame's
 * address, or a negative code (-1 not ready, -2 frame too large), as an int.
 */

J3D_EXTERN_C_START

/* The file ring buffer streamer_Create allocates (0x110 bytes); AnimPlay.pStream points at one. */
struct sStream
{
    char* pBuf;                                 /* 0x00 ring buffer base */
    char* pEnd;                                 /* 0x04 base + ringSize */
    int ringSize;                               /* 0x08 (multiple of 0x800) */
    int maxFrame;                               /* 0x0C */
    int field10;                                /* 0x10 busy count: Prime raises it around its state update, Advance refuses while set */
    int active;                                 /* 0x14 */
    int field18;                                /* 0x18 */
    char* pStart;                               /* 0x1C */
    int field20;                                /* 0x20 */
    char* pRead;                                /* 0x24 */
    char* pWrite;                               /* 0x28 */
    int field2C;                                /* 0x2C */
    int hFile;                                  /* 0x30 */
    int filePos;                                /* 0x34 */
    int fileSize;                               /* 0x38 */
    char name[0xC8];                            /* 0x3C */
    int startOffset;                            /* 0x104 */
    int reopen;                                 /* 0x108 */
    int (*pfnRetry)(int code, const char* pName); /* 0x10C */
};
static_assert(sizeof(Stream) == 0x110, "sizeof(Stream) == 0x110");

typedef struct sStreamerInterface
{
    int  (*Create)(Stream** ppStream, int ringSize, int frameSize);
    void (*Destroy)(Stream** ppStream);
    int  (*SetFile)(Stream* pStream, const char* pName, int startOffset);
    int  (*IsActive)(Stream* pStream);
    void (*Fill)(Stream* pStream, int limit);
    int  (*Advance)(Stream* pStream, int n);
    void (*Reset)(Stream* pStream);
    int  (*Percent)(Stream* pStream);
} StreamerInterface;
static_assert(sizeof(StreamerInterface) == 0x20, "sizeof(StreamerInterface) == 0x20");

int J3DAPI streamer_Register(LecShare* pShare, void* pParam);
void streamer_Unregister(void);
int streamer_Startup(void);
void streamer_Shutdown(void);
int J3DAPI streamer_Create(Stream** ppStream, int ringSize, int frameSize);
void J3DAPI streamer_Destroy(Stream** ppStream);
void J3DAPI streamer_Reset(Stream* pStream);
void J3DAPI streamer_Fill(Stream* pStream, int limit);
int J3DAPI streamer_SetFile(Stream* pStream, const char* pName, int startOffset);
int J3DAPI streamer_Prime(Stream* pStream);
int J3DAPI streamer_IsActive(Stream* pStream);
int J3DAPI streamer_Advance(Stream* pStream, int n);
int J3DAPI streamer_Cancel(Stream* pStream);
int J3DAPI streamer_InRange(Stream* pStream, int n);
int J3DAPI streamer_Percent(Stream* pStream);

void Cdstream_InstallHooks(void);
void Cdstream_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_FILE_CDSTREAM_H
