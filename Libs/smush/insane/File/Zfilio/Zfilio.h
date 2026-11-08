#ifndef SMUSH_INSANE_FILE_ZFILIO_ZFILIO_H
#define SMUSH_INSANE_FILE_ZFILIO_ZFILIO_H
#include <assert.h>
#include <zlib.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/addresses.h>
#include <smush/insane/lec.h>
#include <smush/insane/filio.h>

/*
 * Zfilio.c: the `FILIO=insane_zfilio` module: opens the files whose extension is in its list (".gz", plus what the host adds
 * with zfilio_Configure, e.g. ".snm"/".znm" from SmushPlay) with linked zlib's gz functions, tracking a logical position
 * because gz streams seek lazily. The gzip backend reads compressed physical files directly. Plain input is rejected
 * by the gzip opener so it uses the chained file interface. Other files and gzip-size probes also use the module
 * published as `FILIO.chainto`, kept in the global zfilio_pFilio.
 */

J3D_EXTERN_C_START

#define ZFILIO_MAXEXTENSIONS 50

#define ZFILIO_TAG_EXTENSION 1 /* zfilio_Configure: followed by a const char* extension; the list ends with a 0 tag */

#define ZFILIO_FLAG_REPORT 1 /* print allocation failures through the debug service */

#define ZFILIO_Z_FINISH Z_FINISH /* zlib's gzflush mode */

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

extern FilioInterface* zfilio_pFilio;

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

#if defined(J3D_TEST)
// Unset callbacks use linked zlib.
typedef struct sZfilioTestGz
{
    gzFile (J3DAPI* gzopen)(const char*, const char*);
    int (J3DAPI* gzclose)(gzFile);
    int (J3DAPI* gzread)(gzFile, void*, unsigned int);
    int (J3DAPI* gzwrite)(gzFile, const void*, unsigned int);
    int (J3DAPI* gzflush)(gzFile, int);
    long (J3DAPI* gzseek)(gzFile, long, int);
} ZfilioTestGz;

void Zfilio_TestSetGz(const ZfilioTestGz* pGz);
#endif

void Zfilio_InstallHooks(void);
void Zfilio_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_FILE_ZFILIO_ZFILIO_H
