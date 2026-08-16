#ifndef SITH_TESTS_SITHTHINGTESTSUPPORT_H
#define SITH_TESTS_SITHTHINGTESTSUPPORT_H

#include <j3dcore/j3d.h>

#include <stddef.h>

#include <std/types.h>

J3D_EXTERN_C_START

void sithThingTest_ResetAllocationFailure(void);
void sithThingTest_ResetLoadState(void);
void sithThingTest_SetFailAllocCall(size_t failCall);
size_t sithThingTest_GetNumAllocCalls(void);
size_t sithThingTest_GetNumLiveAllocs(void);

void* J3DAPI sithThingTest_Malloc(size_t size);
void J3DAPI sithThingTest_Free(void* pMemory);
void* J3DAPI sithThingTest_MemoryMalloc(size_t size, const char* pFile, size_t line);
void J3DAPI sithThingTest_MemoryFree(void* pMemory);

void sithThingTest_ResetHashtables(void);
tHashTable* J3DAPI sithThingTest_HashtblNew(size_t size);
void J3DAPI sithThingTest_HashtblFree(tHashTable* pTable);
int J3DAPI sithThingTest_HashtblAdd(tHashTable* pTable, const char* pName, void* pData);
void* J3DAPI sithThingTest_HashtblFind(const tHashTable* pTable, const char* pName);
int J3DAPI sithThingTest_HashtblRemove(tHashTable* pTable, const char* pName);

J3D_EXTERN_C_END

#endif // SITH_TESTS_SITHTHINGTESTSUPPORT_H
