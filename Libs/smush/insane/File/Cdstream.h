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

#define STREAMER_SECTOR_SIZE 0x800 /* the ring and file reads are sector (CD) aligned */
#define STREAMER_NAME_SIZE   0xC8

/* streamer_Advance returns the frame's address, or one of these codes (cast to a pointer by the caller). */
#define STREAMER_NOTREADY (-1) /* no data available yet */
#define STREAMER_BADSIZE  (-2) /* the requested frame is larger than the stream's frame size */

/* streamer_Prime results (the others: 0 the file was (re)opened). */
#define STREAMER_PRIME_IDLE   (-1) /* nothing to open */
#define STREAMER_PRIME_FAILED (-2) /* the file could not be opened and the retry callback gave up */

/* The values the stream's pfnRetry callback is called with. */
#define STREAMER_RETRY_READ 0 /* a read failed (no file name) */
#define STREAMER_RETRY_OPEN 1 /* the file could not be opened (disc missing) */

/* The file ring buffer streamer_Create allocates (0x110 bytes); AnimPlay.pStream points at one. */
struct sStream
{
    char* pBuf;                                 /* 0x00 ring buffer base */
    char* pEnd;                                 /* 0x04 base + ringSize */
    int ringSize;                               /* 0x08 (multiple of STREAMER_SECTOR_SIZE) */
    int maxFrame;                               /* 0x0C */
    int busy;                                   /* 0x10 busy count: Prime raises it around its state update, Advance refuses while set */
    int active;                                 /* 0x14 */
    int ready;                                  /* 0x18 set by Fill once data was read for the active start offset */
    char* pStart;                               /* 0x1C */
    int skip;                                   /* 0x20 bytes of the first sector before the start offset, skipped by the first Advance */
    char* pRead;                                /* 0x24 */
    char* pWrite;                               /* 0x28 */
    int pending;                                /* 0x2C bytes handed out by the last Advance (the write position moves by this on the next one) */
    int hFile;                                  /* 0x30 */
    int filePos;                                /* 0x34 */
    int fileSize;                               /* 0x38 */
    char name[STREAMER_NAME_SIZE];             /* 0x3C */
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
