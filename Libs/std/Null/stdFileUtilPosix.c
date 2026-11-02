// Added: Null platform file utilities (the Windows ones live in std/Win95/stdFileUtilWin32.c).
// Same header contracts: the directory search reports one entry per stdFileUtil_FindNext call and
// the directory and file operations return nonzero on success and 0 on failure. Paths are resolved
// with j3dcrt_ResolvePath first, because the engine writes them the Windows way: case-insensitive
// and with backslash separators.
#ifndef _GNU_SOURCE
// Added: FNM_CASEFOLD, the case-insensitive fnmatch the Windows search matches with, is a GNU
// extension that glibc only declares when the GNU feature test macro is set.
#define _GNU_SOURCE
#endif

#include <std/General/stdFileUtil.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#include <j3dcore/j3dcrt.h>

#include <dirent.h>
#include <fnmatch.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Added: the Windows MAX_PATH size the engine's path buffers use, and the longest directory entry
// name a search reports (NAME_MAX plus the terminator).
#define STD_FILEUTIL_MAXPATH 260
#define STD_FILEUTIL_MAXNAME 256

// Added: mode bits for stdFileUtil_MkDir; the process umask still applies, as it does on Windows
// where the security descriptor argument is NULL.
#define STD_FILEUTIL_DIRMODE 0777

typedef struct sStdFileUtilFindEntry
{
    char     aName[STD_FILEUTIL_MAXNAME];
    uint32_t lastChanged;
    int      bIsDirectory;
} StdFileUtilFindEntry;

// Added: the search state stdFileUtil_FindNext keeps in FindFileData::handle, the equivalent of the
// Windows find handle that FindNextFile walks and FindClose releases.
typedef struct sStdFileUtilFindCursor
{
    size_t numEntries;
    size_t curEntry;
    StdFileUtilFindEntry* pEntries;
} StdFileUtilFindCursor;

// Added: Resolves pPath into pOut. A path that does not fit the buffer, and a null path, are used
// as given, so the operation reports the same failure the Windows call does. Returns the path to use.
static const char* stdFileUtil_Resolve(const char* pPath, char* pOut, size_t size)
{
    if ( !pPath )
    {
        pOut[0] = '\0';
        return pOut;
    }

    if ( strlen(pPath) >= size )
    {
        return pPath;
    }

    return j3dcrt_ResolvePath(pPath, pOut, size);
}

// Added: Whether a directory entry matches the search mask, case-insensitively. Windows matches
// "*.*" against every name, including names without any dot, so it is searched as "*".
static bool stdFileUtil_MaskMatches(const char* pName, const char* pMask)
{
    const char* pPattern = ( strcmp(pMask, "*.*") == 0 ) ? "*" : pMask;
    return fnmatch(pPattern, pName, FNM_CASEFOLD) == 0;
}

// Added: Case-insensitive name order, with the byte order breaking ties, so the listing does not
// depend on the order the directory happens to be read in.
static int stdFileUtil_CompareEntries(const void* pLeft, const void* pRight)
{
    const StdFileUtilFindEntry* pEntryLeft  = (const StdFileUtilFindEntry*)pLeft;
    const StdFileUtilFindEntry* pEntryRight = (const StdFileUtilFindEntry*)pRight;

    int result = strcasecmp(pEntryLeft->aName, pEntryRight->aName);
    if ( result != 0 )
    {
        return result;
    }

    return strcmp(pEntryLeft->aName, pEntryRight->aName);
}

// Added: Copies one collected entry into the caller's result, as the Windows version copies the
// find data.
static void stdFileUtil_FillInfo(tFoundFileInfo* pFileInfo, const StdFileUtilFindEntry* pEntry)
{
    STD_STRCPY(pFileInfo->aName, pEntry->aName);
    pFileInfo->lastChanged  = pEntry->lastChanged;
    pFileInfo->bIsDirectory = pEntry->bIsDirectory;
}

// Added: Reads the directory part of pFilter (a "dir/mask" search path), keeps every entry whose
// name matches the mask, "." and ".." included, and sorts them. Returns NULL when the directory is
// missing or holds no match, so the caller reports the search as failed with nothing to release.
static StdFileUtilFindCursor* stdFileUtil_NewCursor(const char* pFilter)
{
    StdFileUtilFindCursor* pCursor = NULL;
    StdFileUtilFindEntry*  pEntries = NULL;

    char aPath[STD_FILEUTIL_MAXPATH];
    j3dcrt_ResolvePath(pFilter, aPath, sizeof(aPath));

    // Split the resolved path into the directory to read and the mask to match its names against.
    // A filter without a directory part searches the current directory.
    char*       pMask    = strrchr(aPath, '/');
    const char* pDirPath;
    if ( pMask )
    {
        *pMask = '\0';
        ++pMask;
        pDirPath = ( aPath[0] ) ? aPath : "/";
    }
    else
    {
        pDirPath = ".";
        pMask    = aPath;
    }

    DIR* pDir = opendir(pDirPath);
    if ( !pDir )
    {
        return NULL;
    }

    size_t numEntries    = 0;
    size_t numAllocated  = 0;
    struct dirent* pDirent;
    while ( ( pDirent = readdir(pDir) ) != NULL )
    {
        if ( !stdFileUtil_MaskMatches(pDirent->d_name, pMask) )
        {
            continue;
        }

        if ( strlen(pDirent->d_name) >= STD_FILEUTIL_MAXNAME )
        {
            continue;
        }

        if ( numEntries >= numAllocated )
        {
            size_t numNew = ( numAllocated ) ? numAllocated * 2 : 16;
            if ( numNew > SIZE_MAX / sizeof(StdFileUtilFindEntry) )
            {
                numNew = SIZE_MAX / sizeof(StdFileUtilFindEntry);
            }

            StdFileUtilFindEntry* pNew = (StdFileUtilFindEntry*)STDREALLOC(pEntries,
                numNew * sizeof(StdFileUtilFindEntry));
            if ( !pNew )
            {
                break;
            }

            pEntries     = pNew;
            numAllocated = numNew;
        }

        char aEntryPath[STD_FILEUTIL_MAXPATH + STD_FILEUTIL_MAXNAME];
        if ( strlen(pDirPath) + strlen(pDirent->d_name) + 2 > sizeof(aEntryPath) )
        {
            continue;
        }
        STD_FORMAT(aEntryPath, "%s/%s", pDirPath, pDirent->d_name);

        // Fixed: A dangling link still has a directory entry, and the Windows listing reports it,
        // so fall back to the link itself and then to an entry without metadata.
        struct stat st;
        STD_ZEROMEM(&st, sizeof(st));
        if ( stat(aEntryPath, &st) != 0 )
        {
            lstat(aEntryPath, &st);
        }

        StdFileUtilFindEntry* pEntry = &pEntries[numEntries];
        STD_STRCPY(pEntry->aName, pDirent->d_name);

        // Altered: Keep the original 32-bit field conversion explicit until tFoundFileInfo can use time_t.
        pEntry->lastChanged  = (uint32_t)st.st_mtime;
        pEntry->bIsDirectory = S_ISDIR(st.st_mode);
        ++numEntries;
    }

    closedir(pDir);

    if ( numEntries == 0 )
    {
        stdMemory_Free(pEntries);
        return NULL;
    }

    qsort(pEntries, numEntries, sizeof(StdFileUtilFindEntry), stdFileUtil_CompareEntries);

    pCursor = (StdFileUtilFindCursor*)STDMALLOC(sizeof(StdFileUtilFindCursor));
    if ( !pCursor )
    {
        stdMemory_Free(pEntries);
        return NULL;
    }

    pCursor->numEntries = numEntries;
    pCursor->curEntry   = 0;
    pCursor->pEntries   = pEntries;
    return pCursor;
}

void J3DAPI stdFileUtil_DisposeFind(FindFileData* ffData)
{
    if ( ffData )
    {
        if ( ffData->nFoundFiles && ffData->handle != NULL )
        {
            // Added: The collected listing is the data the Windows FindClose releases.
            StdFileUtilFindCursor* pCursor = (StdFileUtilFindCursor*)ffData->handle;
            stdMemory_Free(pCursor->pEntries);
            stdMemory_Free(pCursor);
        }

        stdMemory_Free(ffData);
    }
}

int J3DAPI stdFileUtil_FindNext(FindFileData* ffData, tFoundFileInfo* pFileInfo)
{
    if ( !ffData )
    {
        return 0;
    }

    // Added: Release-build guard before writing search-result metadata.
    STD_GUARD(pFileInfo, 0);

    int nFoundFiles = ffData->nFoundFiles;

    StdFileUtilFindCursor* pCursor;
    if ( nFoundFiles )
    {
        pCursor = (StdFileUtilFindCursor*)ffData->handle;
    }
    else
    {
        // Added: A failed search stores the null handle stdFileUtil_DisposeFind reads as nothing to
        // release, and leaves nFoundFiles alone so the same FindFileData can be searched again.
        pCursor = stdFileUtil_NewCursor(ffData->aSearchFilter);
        ffData->handle = (tSysHandle)pCursor;
    }

    if ( !pCursor || pCursor->curEntry >= pCursor->numEntries )
    {
        return 0;
    }

    stdFileUtil_FillInfo(pFileInfo, &pCursor->pEntries[pCursor->curEntry]);
    ++pCursor->curEntry;

    // Fixed: Count only successful results so a failed first search can be retried safely.
    ffData->nFoundFiles = nFoundFiles + 1;
    return 1;
}

int J3DAPI stdFileUtil_MkDir(const char* pPath)
{
    char aPath[STD_FILEUTIL_MAXPATH];
    return ( mkdir(stdFileUtil_Resolve(pPath, aPath, sizeof(aPath)), STD_FILEUTIL_DIRMODE) == 0 ) ? 1 : 0;
}

int J3DAPI stdFileUtil_FileExists(const char* pFilename)
{
    struct stat st;

    char aPath[STD_FILEUTIL_MAXPATH];
    return ( stat(stdFileUtil_Resolve(pFilename, aPath, sizeof(aPath)), &st) == 0 ) ? 1 : 0;
}

int J3DAPI stdFileUtil_RmDir(const char* pDir)
{
    char aPath[STD_FILEUTIL_MAXPATH];
    return ( rmdir(stdFileUtil_Resolve(pDir, aPath, sizeof(aPath))) == 0 ) ? 1 : 0;
}

int J3DAPI stdFileUtil_DelFile(const char* pFilename)
{
    char aPath[STD_FILEUTIL_MAXPATH];
    return ( unlink(stdFileUtil_Resolve(pFilename, aPath, sizeof(aPath))) == 0 ) ? 1 : 0;
}
