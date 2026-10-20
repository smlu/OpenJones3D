#ifndef SMUSH_INSANE_FILE_STDFILE_H
#define SMUSH_INSANE_FILE_STDFILE_H
#include <assert.h>
#include <j3dcore/j3d.h>
#include <smush/RTI/addresses.h>
#include <smush/insane/lec.h>
#include <smush/insane/filio.h>

/* Stdfile.c: the `FILE=insane_stdfile` service: FILIO's seven entry points plus size, gets, eof, printf, getc and putc. */

J3D_EXTERN_C_START

typedef struct sStdFileInterface
{
    int   (*Open)(const char* pPath, const char* pMode);
    void  (*Close)(int fh);
    int   (*Read)(int fh, void* pBuf, int size);
    int   (*Write)(int fh, const void* pBuf, int size);
    int   (*Tell)(int fh);
    int   (*Seek)(int fh, int offset, int whence);
    void  (*Flush)(int fh);
    int   (*Size)(int fh);
    char* (*Gets)(int fh, char* pBuf, int size);
    int   (*Eof)(int fh);
    int   (*Printf)(int fh, const char* pFormat, ...);
    int   (*Getc)(int fh);
    int   (*Putc)(int fh, int c);
} StdFileInterface;
static_assert(sizeof(StdFileInterface) == 0x34, "sizeof(StdFileInterface) == 0x34");

int J3DAPI stdfile_Register(LecShare* pShare, void* pParam);
void stdfile_Unregister(void);
int stdfile_Startup(void);
void stdfile_Shutdown(void);
int J3DAPI stdfile_open(const char* pPath, const char* pMode);
void J3DAPI stdfile_close(int fh);
int J3DAPI stdfile_read(int fh, void* pBuf, int size);
int J3DAPI stdfile_write(int fh, const void* pBuf, int size);
int J3DAPI stdfile_tell(int fh);
int J3DAPI stdfile_seek(int fh, int offset, int whence);
void J3DAPI stdfile_flush(int fh);
int J3DAPI stdfile_size(int fh);
char* J3DAPI stdfile_gets(int fh, char* pBuf, int size);
int J3DAPI stdfile_eof(int fh);
int J3DAPI stdfile_printf(int fh, const char* pFormat, ...);
int J3DAPI stdfile_getc(int fh);
int J3DAPI stdfile_putc(int fh, int c);

void Stdfile_InstallHooks(void);
void Stdfile_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_FILE_STDFILE_H
