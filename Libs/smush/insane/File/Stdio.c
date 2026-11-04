// Stdio.c: the `FILIO=insane_stdio` module (C stdio) (original 0x004EA020-0x004EA0F0).
#include <smush/insane/File/Stdio.h>
#include <j3dcore/j3d.h>

#include <limits.h>
#include <stddef.h>
#include <std/General/std.h>
#include <std/General/stdUtil.h>

static LecModule stdio_module = { stdio_Register, stdio_Unregister, stdio_Startup, stdio_Shutdown };
static FilioInterface stdio_filio = { stdio_open, stdio_close, stdio_read, stdio_write, stdio_tell, stdio_seek, stdio_flush };

static void* stdio_pParam;
static LecShare* stdio_pShare;

void Stdio_ResetGlobals(void)
{
    stdio_pParam = NULL;
    stdio_pShare = NULL;
}

int J3DAPI stdio_Register(LecShare* pShare, void* pParam)
{
    stdio_pShare = pShare;
    stdio_pParam = pParam;
    pShare->Publish("LECMODULE=insane_stdio", &stdio_module);
    stdio_pShare->Publish("FILIO=insane_stdio", &stdio_filio);
    return 0;
}

void stdio_Unregister(void)
{
    stdio_pShare->Unpublish("FILIO=insane_stdio");
    stdio_pShare->Unpublish("LECMODULE=insane_stdio");
}

int stdio_Startup(void)
{
    return 0;
}

void stdio_Shutdown(void)
{}

// Altered: The original calls the C runtime's stdio directly (fopen, after remove() for a 'w' mode). The wrappers below use the
// engine's stdFile* functions (std.c) instead. The original removes the file before opening it for 'w', stdFileOpen does not.
// stdFileOpen also differs in the share mode (fopen_s uses _SH_SECURE, the original's fopen _SH_DENYNO) and in the handling of invalid
// parameters (fopen_s calls the invalid parameter handler where fopen returned an error).

// Fixed: stdFileOpen returns the FILE* inside the uintptr_t tFileHandle, and the int-based FilioInterface carried it
// through the cast below. That only holds while a pointer fits into an int; on 64-bit builds (the Null platform host)
// stdio_open truncated the pointer and every later stdio_* call turned the truncated int back into a wild FILE* and
// crashed. Where pointers are wider than ints, an int handle is a slot of the table below instead; 32-bit builds (all
// Windows backends) keep the original casts and handle values unchanged.
#if UINTPTR_MAX > INT_MAX

#define STDIO_NUMHANDLES 64
static tFileHandle stdio_aHandles[STDIO_NUMHANDLES];

static int stdio_PutHandle(tFileHandle fh)
{
    if ( fh == 0 )
    {
        return 0;
    }

    for ( size_t i = 0; i < STD_ARRAYLEN(stdio_aHandles); ++i )
    {
        if ( stdio_aHandles[i] == 0 )
        {
            stdio_aHandles[i] = fh;
            return (int)(i + 1);
        }
    }

    stdFileClose(fh);
    return 0;
}

static tFileHandle stdio_GetHandle(int fh)
{
    return ( fh >= 1 && (size_t)fh <= STD_ARRAYLEN(stdio_aHandles) ) ? stdio_aHandles[fh - 1] : 0;
}

static void stdio_ClearHandle(int fh)
{
    if ( fh >= 1 && (size_t)fh <= STD_ARRAYLEN(stdio_aHandles) )
    {
        stdio_aHandles[fh - 1] = 0;
    }
}

#define STDIO_FROMHANDLE(stdfh) stdio_PutHandle(stdfh)
#define STDIO_TOHANDLE(fh) stdio_GetHandle(fh)
#define STDIO_RELEASE(fh) stdio_ClearHandle(fh)

#else

#define STDIO_FROMHANDLE(stdfh) ((int)(stdfh))
#define STDIO_TOHANDLE(fh) ((tFileHandle)(fh))
#define STDIO_RELEASE(fh)

#endif

int J3DAPI stdio_open(const char* pPath, const char* pMode)
{
    return STDIO_FROMHANDLE(stdFileOpen(pPath, pMode));
}

void J3DAPI stdio_close(int fh)
{
    stdFileClose(STDIO_TOHANDLE(fh));
    STDIO_RELEASE(fh);
}

int J3DAPI stdio_read(int fh, void* pBuf, int size)
{
    return (int)stdFileRead(STDIO_TOHANDLE(fh), pBuf, size);
}

int J3DAPI stdio_write(int fh, const void* pBuf, int size)
{
    return (int)stdFileWrite(STDIO_TOHANDLE(fh), pBuf, size);
}

int J3DAPI stdio_tell(int fh)
{
    return stdFileTell(STDIO_TOHANDLE(fh));
}

int J3DAPI stdio_seek(int fh, int offset, int whence)
{
    return stdFileSeek(STDIO_TOHANDLE(fh), offset, whence);
}

void J3DAPI stdio_flush(int fh)
{
    stdFileFlush(STDIO_TOHANDLE(fh));
}
