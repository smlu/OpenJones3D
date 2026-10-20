#ifndef SMUSH_INSANE_IMAGE_READAUTO_H
#define SMUSH_INSANE_IMAGE_READAUTO_H
#include <assert.h>
#include <j3dcore/j3d.h>
#include <smush/RTI/addresses.h>
#include <smush/insane/lec.h>
#include <smush/insane/readanim.h>
#include <smush/insane/Memory/Memory.h>

/* Readauto.c: the `READANIM=insane_auto` reader that picks one of the registered READANIM readers per file. */

J3D_EXTERN_C_START

typedef struct sReadAutoHandle
{
    ReadAnimInterface* pReader;
    void* hReader;
} ReadAutoHandle;
static_assert(sizeof(ReadAutoHandle) == 8, "sizeof(ReadAutoHandle) == 8");

int J3DAPI readauto_Register(LecShare* pShare, void* pParam);
void readauto_Unregister(void);
int readauto_Startup(void);
void readauto_Shutdown(void);
const char* J3DAPI readauto_Query(int code);
int J3DAPI readauto_Read(int a, int b, void* pFile);
int J3DAPI readauto_Open(void** ppHandle, const char* pFile);
void J3DAPI readauto_Close(void** ppHandle);
int J3DAPI readauto_Op3(void* pHandle, int a, int b, int c, int d, int e);
int J3DAPI readauto_Op4(void* pHandle, int a, int b);
int J3DAPI readauto_Recognize(const char* pFile, int pass); // Folded onto 0x004E74A0; no address of its own, never hook

void Readauto_InstallHooks(void);
void Readauto_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_IMAGE_READAUTO_H
