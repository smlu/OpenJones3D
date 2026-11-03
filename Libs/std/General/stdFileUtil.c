#include "std.h"
#include "stdFileUtil.h"
#include "stdFnames.h"
#include "stdMemory.h"
#include "stdUtil.h"

#include <j3dcore/j3dhook.h>
#include <std/RTI/symbols.h>

void stdFileUtil_ResetGlobals(void)
{}

FindFileData* J3DAPI stdFileUtil_NewFind(const char* path, int mode, const char* pFilter)
{
    // Added: Release-build guard before building a search path.
    STD_GUARD(path, NULL);

    FindFileData* pData = (FindFileData*)STDMALLOC(sizeof(FindFileData));
    if ( !pData )
    {
        return NULL;
    }

    memset(pData, 0, sizeof(FindFileData));

    if ( mode < 0 )
    {
        return pData;
    }

    if ( mode <= 2 )
    {
        stdFnames_MakePath(pData->aSearchFilter, sizeof(pData->aSearchFilter), path, "*.*");
        return pData;
    }

    if ( mode != 3 )
    {
        return pData;
    }

    if ( !pFilter )
    {
        // Added: Treat a NULL mode-3 filter as the same wildcard used by the default modes.
        pFilter = "*";
    }

    if ( *pFilter == '.' )
    {
        pFilter++;
    }

    STD_FORMAT(std_g_genBuffer, "*.%s", pFilter);
    stdFnames_MakePath(pData->aSearchFilter, sizeof(pData->aSearchFilter), path, std_g_genBuffer);
    return pData;
}

int J3DAPI stdFileUtil_FindQuick(const char* pPath, int mode, const char* pFilter, tFoundFileInfo* pFileInfo)
{
    FindFileData* pFileData = stdFileUtil_NewFind(pPath, mode, pFilter);
    if ( !pFileData )
    {
        return 0;
    }

    int bFound = stdFileUtil_FindNext(pFileData, pFileInfo);
    stdFileUtil_DisposeFind(pFileData);
    return bFound;
}

int J3DAPI stdFileUtil_CountMatches(const char* pPath, int mode, const char* pFilter)
{
    tFoundFileInfo fileInfo;

    int count = 0;
    FindFileData* pFileData = stdFileUtil_NewFind(pPath, mode, pFilter);
    if ( !pFileData )
    {
        return 0;
    }

    while ( stdFileUtil_FindNext(pFileData, &fileInfo) )
    {
        ++count;
    }

    stdFileUtil_DisposeFind(pFileData);
    return count;
}

