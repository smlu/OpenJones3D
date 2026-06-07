#ifndef STD_STDFNAMES_H
#define STD_STDFNAMES_H
#include <j3dcore/j3d.h>
#include <std/types.h>
#include <std/RTI/addresses.h>

#include "stdUtil.h"

J3D_EXTERN_C_START

#define STD_MAKEPATH(outPath, path1, path2) \
    do {\
        _Static_assert(STD_ISSTRARRAY(outPath), "STD_MAKEPATH requires an array type for 'outPath'"); \
        stdFnames_MakePath(outPath, STD_ARRAYLEN(outPath), path1, path2); \
    } while ( 0 )

const char* J3DAPI stdFnames_FindMedName(const char* pFilePath);
const char* J3DAPI stdFnames_FindExt(const char* pFilePath);
void J3DAPI stdFnames_StripExtAndDot(char* pPath);
// Added: Bounded extension replacement; use this instead of stdFnames_ChangeExt for fixed buffers.
void J3DAPI stdFnames_ChangeExtEx(char* pPath, size_t size, const char* pExt);
void J3DAPI stdFnames_ChangeExt(char* pPath, const char* pExt);
void J3DAPI stdFnames_Concat(char* dst, const char* src, size_t size);
void J3DAPI stdFnames_MakePath(char* aOutPath, int size, const char* pPath1, const char* pPath2);

// Helper hooking functions
void stdFnames_InstallHooks(void);
void stdFnames_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // STD_STDFNAMES_H
