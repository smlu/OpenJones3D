#include "stdFnames.h"
#include "stdUtil.h"

#include <j3dcore/j3dhook.h>
#include <std/RTI/symbols.h>

void stdFnames_InstallHooks(void)
{
    J3D_HOOKFUNC(stdFnames_FindMedName);
    J3D_HOOKFUNC(stdFnames_FindExt);
    J3D_HOOKFUNC(stdFnames_StripExtAndDot);
    J3D_HOOKFUNC(stdFnames_ChangeExt);
    J3D_HOOKFUNC(stdFnames_Concat);
    J3D_HOOKFUNC(stdFnames_MakePath);
}

void stdFnames_ResetGlobals(void)
{
}

const char* J3DAPI stdFnames_FindMedName(const char* pFilePath)
{
    // Added: Release-build guard for callers that pass a NULL path.
    STD_GUARD(pFilePath, "");

    const char* pName = strrchr(pFilePath, '\\');
    if ( !pName )
    {
        return pFilePath;
    }

    if ( *pName == '\\' )
    {
        char chr = 0;
        do
        {
            chr = *++pName;
        } while ( chr == '\\' );
    }

    return pName;
}

const char* J3DAPI stdFnames_FindExt(const char* pFilePath)
{
    // Added: Release-build guard for callers that pass a NULL path.
    STD_GUARD(pFilePath, NULL);

    const char* pFile = stdFnames_FindMedName(pFilePath);
    const char* pExt = strrchr(pFile, '.');
    if ( pExt )
    {
        ++pExt;
    }

    return pExt;
}

void J3DAPI stdFnames_StripExtAndDot(char* pPath)
{
    // Added: Release-build guard before modifying the path buffer.
    STD_GUARD_VOID(pPath);

    char* pExt = (char*)stdFnames_FindExt(pPath);
    if ( pExt )
    {
        *(pExt - 1) = 0;
    }
}

void J3DAPI stdFnames_ChangeExtEx(char* pPath, size_t size, const char* pExt)
{
    // Added: Bounded extension replacement variant for fixed-size buffers.
    STD_GUARD_VOID(pPath && pExt && size);

    stdFnames_StripExtAndDot(pPath);

    // Added: Accept callers that pass either "ext" or ".ext".
    if ( *pExt == '.' )
    {
        ++pExt;
    }

    size_t len = strnlen_s(pPath, size);
    // Fixed: Do not append if the buffer is already unterminated within its bounds.
    if ( len >= size )
    {
        return;
    }

    if ( len < size - 1 )
    {
        pPath[len++] = '.';
        pPath[len] = '\0';
    }

    stdUtil_StringCat(pPath, size, pExt);
}

void J3DAPI stdFnames_ChangeExt(char* pPath, const char* pExt)
{
    // Altered: Preserve the original ABI while delegating extension changes to stdFnames_ChangeExtEx.
    STD_GUARD_VOID(pPath && pExt);

    stdFnames_ChangeExtEx(pPath, strlen(pPath) + strlen(pExt) + 2u, pExt);
}

void J3DAPI stdFnames_Concat(char* path1, const char* path2, size_t size)
{
    // Added: Release-build guard before concatenating path buffers.
    STD_GUARD_VOID(path1 && path2);

    if ( !size )
    {
        return;
    }

    size_t len = strnlen_s(path1, size);
    // Fixed: Empty base paths have no last character to inspect.
    if ( len > 0 && path1[len - 1] != '\\' && len < size - 1 )
    {
        path1[len] = '\\';
        path1[++len]   = '\0';
    }

    if ( *path2 == '\\' )
    {
        path2++;
    }

    stdUtil_StringCat(path1, size, path2); // TODO: log if path can't be concatenated
}

void J3DAPI stdFnames_MakePath(char* aOutPath, int size, const char* pPath1, const char* pPath2)
{
    // Added: Release-build guard before writing the output path.
    STD_GUARD_VOID(aOutPath && size > 0);

    // Fixed: Allow a missing base path and only append a second component when present.
    stdUtil_StringCopy(aOutPath, size, pPath1 ? pPath1 : "");
    if ( pPath2 )
    {
        stdFnames_Concat(aOutPath, pPath2, size);
    }
}
