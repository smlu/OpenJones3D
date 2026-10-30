// Stdio.c: the `FILIO=insane_stdio` module (C stdio) (original 0x004EA020-0x004EA0F0).
#include <smush/insane/File/Stdio.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

#include <stddef.h>
#include <std/General/std.h>

static LecModule stdio_module = { stdio_Register, stdio_Unregister, stdio_Startup, stdio_Shutdown };
static FilioInterface stdio_filio = { stdio_open, stdio_close, stdio_read, stdio_write, stdio_tell, stdio_seek, stdio_flush };

static void* stdio_pParam;
static LecShare* stdio_pShare;

void Stdio_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(stdio_Register);
    J3D_HOOKFUNC(stdio_Unregister);
    J3D_HOOKFUNC(stdio_open);
    J3D_HOOKFUNC(stdio_close);
    J3D_HOOKFUNC(stdio_flush);
}

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
int J3DAPI stdio_open(const char* pPath, const char* pMode)
{
    return (int)stdFileOpen(pPath, pMode);
}

void J3DAPI stdio_close(int fh)
{
    stdFileClose((tFileHandle)fh);
}

int J3DAPI stdio_read(int fh, void* pBuf, int size)
{
    return (int)stdFileRead((tFileHandle)fh, pBuf, size);
}

int J3DAPI stdio_write(int fh, const void* pBuf, int size)
{
    return (int)stdFileWrite((tFileHandle)fh, pBuf, size);
}

int J3DAPI stdio_tell(int fh)
{
    return stdFileTell((tFileHandle)fh);
}

int J3DAPI stdio_seek(int fh, int offset, int whence)
{
    return stdFileSeek((tFileHandle)fh, offset, whence);
}

void J3DAPI stdio_flush(int fh)
{
    stdFileFlush((tFileHandle)fh);
}
