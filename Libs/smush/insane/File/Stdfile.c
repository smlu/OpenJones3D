// Stdfile.c: the `FILE=insane_stdfile` service (FILIO plus size, gets, eof, printf, getc, putc) (original 0x004E9810-0x004E9B30).
#include <smush/insane/File/Stdfile.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/addresses.h>
#include <smush/RTI/symbols.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static LecModule stdfile_module = { stdfile_Register, stdfile_Unregister, stdfile_Startup, stdfile_Shutdown };
static StdFileInterface stdfile_interface = {
    stdfile_open, stdfile_close, stdfile_read, stdfile_write, stdfile_tell, stdfile_seek, stdfile_flush,
    stdfile_size, stdfile_gets, stdfile_eof, stdfile_printf, stdfile_getc, stdfile_putc
};

static LecShare* stdfile_pShare;
static FilioInterface* stdfile_pChained;
static void* stdfile_pParam;

void Stdfile_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(stdfile_Register);
    J3D_HOOKFUNC(stdfile_Unregister);
    J3D_HOOKFUNC(stdfile_Startup);
    J3D_HOOKFUNC(stdfile_Shutdown);
    J3D_HOOKFUNC(stdfile_open);
    J3D_HOOKFUNC(stdfile_close);
    J3D_HOOKFUNC(stdfile_read);
    J3D_HOOKFUNC(stdfile_write);
    J3D_HOOKFUNC(stdfile_tell);
    J3D_HOOKFUNC(stdfile_seek);
    J3D_HOOKFUNC(stdfile_flush);
    J3D_HOOKFUNC(stdfile_size);
    J3D_HOOKFUNC(stdfile_gets);
    J3D_HOOKFUNC(stdfile_eof);
    J3D_HOOKFUNC(stdfile_printf);
    J3D_HOOKFUNC(stdfile_getc);
    J3D_HOOKFUNC(stdfile_putc);
}

void Stdfile_ResetGlobals(void)
{
    stdfile_pShare   = NULL;
    stdfile_pChained = NULL;
    stdfile_pParam   = NULL;
}

int J3DAPI stdfile_Register(LecShare* pShare, void* pParam)
{
    stdfile_pShare = pShare;
    stdfile_pParam = pParam;
    pShare->Publish("LECMODULE=insane_stdfile", &stdfile_module);
    stdfile_pShare->Publish("FILE=insane_stdfile", &stdfile_interface);
    return 0;
}

void stdfile_Unregister(void)
{
    stdfile_pShare->Unpublish("FILE=insane_stdfile");
    stdfile_pShare->Unpublish("LECMODULE=insane_stdfile");
}

int stdfile_Startup(void)
{
    stdfile_pChained = (FilioInterface*)stdfile_pShare->Subscribe("FILIO.chainto", stdfile_pParam);
    if ( !stdfile_pChained )
    {
        stdfile_pShare->ReportError("StdFile Module: Cannot allocate filio services.");
        return -1;
    }

    return 0;
}

void stdfile_Shutdown(void)
{
    stdfile_pShare->Unsubscribe(stdfile_pChained);
}

int J3DAPI stdfile_open(const char* pPath, const char* pMode)
{
    return stdfile_pChained->Open(pPath, pMode);
}

void J3DAPI stdfile_close(int fh)
{
    stdfile_pChained->Close(fh);
}

int J3DAPI stdfile_read(int fh, void* pBuf, int size)
{
    return stdfile_pChained->Read(fh, pBuf, size);
}

int J3DAPI stdfile_write(int fh, const void* pBuf, int size)
{
    return stdfile_pChained->Write(fh, pBuf, size);
}

int J3DAPI stdfile_tell(int fh)
{
    return stdfile_pChained->Tell(fh);
}

int J3DAPI stdfile_seek(int fh, int offset, int whence)
{
    return stdfile_pChained->Seek(fh, offset, whence);
}

void J3DAPI stdfile_flush(int fh)
{
    stdfile_pChained->Flush(fh);
}

int J3DAPI stdfile_size(int fh)
{
    int pos = stdfile_pChained->Tell(fh);
    stdfile_pChained->Seek(fh, 0, 2);
    int size = stdfile_pChained->Tell(fh);
    stdfile_pChained->Seek(fh, pos, 0);
    return size;
}

char* J3DAPI stdfile_gets(int fh, char* pBuf, int size)
{
    // TODO: [BUG] The result is not null-terminated when size characters are read without hitting a newline (or end of file)
    for ( int i = 0; i < size; ++i )
    {
        int c = stdfile_getc(fh);
        if ( c == -1 )
        {
            if ( i == 0 )
            {
                return NULL;
            }

            pBuf[i] = 0;
            break;
        }

        pBuf[i] = (char)c;
        if ( c == '\n' && i + 1 < size )
        {
            pBuf[i + 1] = 0;
            break;
        }
    }

    return pBuf;
}

int J3DAPI stdfile_eof(int fh)
{
    return stdfile_tell(fh) >= stdfile_size(fh);
}

int J3DAPI stdfile_printf(int fh, const char* pFormat, ...)
{
    char aBuf[500];

    va_list args;
    va_start(args, pFormat);
    // Fixed: The original uses vsprintf, which overflows the 500 byte buffer on longer output
    vsnprintf_s(aBuf, sizeof(aBuf), sizeof(aBuf) - 1, pFormat, args);
    va_end(args);
    return stdfile_pChained->Write(fh, aBuf, (int)strlen(aBuf));
}

int J3DAPI stdfile_getc(int fh)
{
    // The original reads the byte into the low byte of the handle parameter itself
    if ( stdfile_pChained->Read(fh, &fh, 1) )
    {
        return fh & 0xFF;
    }

    return -1;
}

int J3DAPI stdfile_putc(int fh, int c)
{
    if ( stdfile_pChained->Write(fh, &c, 1) )
    {
        return c & 0xFF;
    }

    return -1;
}
