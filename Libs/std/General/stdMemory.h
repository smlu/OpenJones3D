#ifndef STD_STDMEMORY_H
#define STD_STDMEMORY_H
#include <j3dcore/j3d.h>
#include <std/types.h>
#include <std/RTI/addresses.h>

J3D_EXTERN_C_START

#ifdef J3D_32BIT
#  define STDMEMORY_FREEDPTR ((void*)0xDDDDDDDDUL)
#else
#  define STDMEMORY_FREEDPTR ((void*)0xDDDDDDDDDDDDDDDDULL)
#endif

/**
 * @brief Macro for allocating memory using stdMemory_Malloc
 * @param size - The size of memory to allocate in bytes
 * @return Pointer to the allocated memory
*/
#define STDMALLOC(size) \
    stdMemory_Malloc((size), J3D_FILE, __LINE__)

#define STDFREE(pData)\
    stdMemory_Free((pData))

/**
 * @brief Macro for re-allocating memory using stdMemory_Realloc
 * @brief data for re-allocating memory using stdMemory_Realloc
 * @param data - Existing allocated data to re-alloc
 * @param size - The size of memory to allocate in bytes
 * @return Pointer to the newly allocated memory
*/
#define STDREALLOC(data, size) \
    stdMemory_Realloc((data), (size), J3D_FILE, __LINE__)

#ifdef J3D_LOCAL_RTI
extern tMemoryState stdMemory_g_curState;
#else
#   define stdMemory_g_curState J3D_DECL_FAR_VAR(stdMemory_g_curState, tMemoryState)
#endif

int  stdMemory_Startup(void);
void  stdMemory_Shutdown(void);

int  stdMemory_Open(void);
void  stdMemory_Close(void);

void* J3DAPI stdMemory_Malloc(size_t size, const char* pFilename, size_t line);
void J3DAPI stdMemory_Free(void* pBytes);
void* J3DAPI stdMemory_Realloc(void* pBytes, size_t size, const char* file, size_t line);

void* J3DAPI stdMemory_BlockMalloc(size_t size);
void J3DAPI stdMemory_BlockFree(void* pMemory);
void* J3DAPI stdMemory_BlockRealloc(void* pData, size_t size);
void* J3DAPI stdMemory_BlockAlloc(size_t size);

// Helper hooking functions
void stdMemory_InstallHooks(void);
void stdMemory_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // STD_STDMEMORY_H
