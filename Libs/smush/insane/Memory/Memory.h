#ifndef SMUSH_INSANE_MEMORY_MEMORY_H
#define SMUSH_INSANE_MEMORY_MEMORY_H
#include <assert.h>
#include <stddef.h>
#include <j3dcore/j3d.h>
#include <smush/insane/lec.h>
#include <smush/insane/Util/List.h>

/*
 * INSANE's memory services (insane_memory). `MEMORY=insane` is plain malloc/free/realloc; `MEMORY=insane_debug`
 * (also what modules receive when they subscribe to `MEMORY.workspace`) records the caller's file and line for every
 * request.
 */

J3D_EXTERN_C_START

typedef struct sMemoryInterface
{
    void* (*Alloc)(size_t size);
    void  (*Free)(void* pBlock);
    void* (*Realloc)(void* pBlock, size_t size);
    void* pReserved;
} MemoryInterface;
static_assert(sizeof(MemoryInterface) == 0x10, "sizeof(MemoryInterface) == 0x10");

typedef struct sMemoryDebugInterface
{
    void* (*Alloc)(int size, const char* pFile, int line);
    void  (*Free)(void* pBlock, const char* pFile, int line);
    void* (*Realloc)(void* pBlock, int size, const char* pFile, int line);
} MemoryDebugInterface;
static_assert(sizeof(MemoryDebugInterface) == 0xC, "sizeof(MemoryDebugInterface) == 0xC");

/* The debug allocator's per-block record (allocated with malloc, listed in the allocator's LecList). */
typedef struct sMemoryRecord
{
    LecNode link;
    void* pBlock;
    int size;
    int line;
    const char* pFile;
    int refs;
} MemoryRecord;
static_assert(sizeof(MemoryRecord) == 0x1C, "sizeof(MemoryRecord) == 0x1C");

int J3DAPI memory_Register(LecShare* pShare, void* pParam);
void memory_Unregister(void);
void* J3DAPI memory_debugAlloc(int size, const char* pFile, int line);
int J3DAPI memory_Track(char* pRaw, int size, const char* pFile, int line, const char* pOp);
void J3DAPI memory_debugFree(void* pBlock, const char* pFile, int line);
int J3DAPI memory_Check(char* pRaw, int size, const char* pFile, int line, const char* pOp);
void* J3DAPI memory_debugRealloc(void* pBlock, int size, const char* pFile, int line);

void Memory_InstallHooks(void);
void Memory_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_MEMORY_MEMORY_H
