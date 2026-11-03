#ifndef STD_STDGOB_H
#define STD_STDGOB_H
#include <j3dcore/j3d.h>
#include <std/types.h>
#include <std/RTI/addresses.h>

J3D_EXTERN_C_START

void J3DAPI stdGob_Startup(tHostServices* pHS);
void J3DAPI stdGob_Shutdown();
Gob* J3DAPI stdGob_Load(const char* pFilename, int numFileHandles, int bMapFile);
int J3DAPI stdGob_LoadEntry(Gob* pGob, const char* pFilename, int numFileHandles, int bMapFile);
void J3DAPI stdGob_Free(Gob* pGob);
void J3DAPI stdGob_FreeEntry(Gob* pGob);
GobFileHandle* J3DAPI stdGob_FileOpen(Gob* pGob, const char* aName);
void J3DAPI stdGob_FileClose(GobFileHandle* pHandle);
int J3DAPI stdGob_FileSeek(GobFileHandle* pHandle, int offset, int origin);
int J3DAPI stdGob_FileTell(GobFileHandle* pHandle);
int J3DAPI stdGob_FileEOF(GobFileHandle* pHandle);
size_t J3DAPI stdGob_FileRead(GobFileHandle* pHandle, void* data, const size_t size);
const char* J3DAPI stdGob_FileGets(GobFileHandle* pGobFileHandle, char* pStr, size_t size);

// Helper hooking functions
void stdGob_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // STD_STDGOB_H
