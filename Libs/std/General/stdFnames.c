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
    const char* pFile = stdFnames_FindMedName(pFilePath);
    const char* pExt = strrchr(pFile, '.');
    if ( pExt ) {
        ++pExt;
    }

    return pExt;
}

void J3DAPI stdFnames_StripExtAndDot(char* pPath)
{
    char* pExt = (char*)stdFnames_FindExt(pPath);
    if ( pExt )
    {
        *(pExt - 1) = 0;
    }
}

void J3DAPI stdFnames_ChangeExt(char* pPath, const char* pExt)
{
    stdFnames_StripExtAndDot(pPath);

    // TODO: Very unsafe way to cat string. Made so just to shut up complaining from msvc. Find some safer way...
    stdUtil_StringCat(pPath, strlen(pPath) + 2, ".");
    stdUtil_StringCat(pPath, strlen(pPath) + 1 + strlen(pExt), pExt);
}

void J3DAPI stdFnames_Concat(char* path1, const char* path2, size_t size)
{
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

    if ( *path2 == '\\' ) {
        path2++;
    }

    stdUtil_StringCat(path1, size, path2); // TODO: log if path can't be concatenated
}

void J3DAPI stdFnames_MakePath(char* aOutPath, int size, const char* pPath1, const char* pPath2)
{
    stdUtil_StringCopy(aOutPath, size, pPath1);
    stdFnames_Concat(aOutPath, pPath2, size);
}
