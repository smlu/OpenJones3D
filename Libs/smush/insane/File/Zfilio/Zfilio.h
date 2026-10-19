#ifndef SMUSH_INSANE_FILE_ZFILIO_ZFILIO_H
#define SMUSH_INSANE_FILE_ZFILIO_ZFILIO_H
#include <assert.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/addresses.h>
#include <smush/insane/lec.h>
#include <smush/insane/filio.h>

/*
 * Zfilio.c: the `FILIO=insane_zfilio` module: opens the files whose extension is in its list (".gz", plus what the host adds
 * with zfilio_Configure, e.g. ".snm"/".znm" from SmushPlay) with zlib's gz functions of the game executable (gzopen etc.,
 * RTI/addresses.h), tracking a logical position because gz streams seek lazily. It chains every other file to the module
 * published as `FILIO.chainto` and keeps that interface in the global zfilio_pFilio, which the executable's own gzio.c reads.
 */

J3D_EXTERN_C_START

#define ZFILIO_MAXEXTENSIONS 50

/* The executable's zlib 1.0.9 gz functions are reached through RTI/addresses.h (gzopen_ADDR etc.); gzFile is an opaque handle. */
typedef void* gzFile;

typedef struct sZfileHandle
{
    gzFile hFile;
    int isGz;
    int gzSize;
    int gzPos;
    int pos;
} ZfileHandle;
static_assert(sizeof(ZfileHandle) == 0x14, "sizeof(ZfileHandle) == 0x14");

typedef struct sZfilioInterface
{
    ZfileHandle* (*Open)(const char* pPath, const char* pMode);
    void (*Close)(ZfileHandle* pFile);
    int  (*Read)(ZfileHandle* pFile, void* pBuf, int size);
    int  (*Write)(ZfileHandle* pFile, const void* pBuf, int size);
    int  (*Tell)(ZfileHandle* pFile);
    int  (*Seek)(ZfileHandle* pFile, int offset, int whence);
    void (*Flush)(ZfileHandle* pFile);
} ZfilioInterface;
static_assert(sizeof(ZfilioInterface) == 0x1C, "sizeof(ZfilioInterface) == 0x1C");

/*
 * zfilio_pFilio lives in the executable (.bss 0x0183E220, COMMON) because the executable's gzio.c reads it. It is the only INSANE
 * datum that stays at its original address; Smush_ResetGlobals clears it.
 */
#ifdef J3D_LOCAL_RTI
extern FilioInterface* zfilio_pFilio;
#else
#   define zfilio_pFilio J3D_DECL_FAR_VAR(zfilio_pFilio, FilioInterface*)
#endif

int J3DAPI zfilio_Register(LecShare* pShare, void* pParam);
int J3DAPI zfilio_Configure(int tag, ...);
void zfilio_Unregister(void);
int zfilio_Startup(void);
void zfilio_Shutdown(void);
ZfileHandle* J3DAPI zfilio_open(const char* pPath, const char* pMode);
void J3DAPI zfilio_close(ZfileHandle* pFile);
int J3DAPI zfilio_read(ZfileHandle* pFile, void* pBuf, int size);
int J3DAPI zfilio_write(ZfileHandle* pFile, const void* pBuf, int size);
int J3DAPI zfilio_tell(ZfileHandle* pFile);
int J3DAPI zfilio_seek(ZfileHandle* pFile, int offset, int whence);
void J3DAPI zfilio_flush(ZfileHandle* pFile);

void Zfilio_InstallHooks(void);
void Zfilio_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_FILE_ZFILIO_ZFILIO_H
