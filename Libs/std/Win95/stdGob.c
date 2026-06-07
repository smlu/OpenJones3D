#include "stdGob.h"
#include <j3dcore/j3dhook.h>
#include <std/General/std.h>
#include <std/General/stdHashTbl.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/RTI/symbols.h>

#include <limits.h>
#include <stdint.h>

static bool bStartup = false;
static char stdGob_aEntryNameBuff[128] = { 0 };

static tHostServices stdGob_hs   = { 0 };
static tHostServices* stdGob_pHS = NULL;

// TODO: The file read functions don't have implemented interacting with memory mapped file.

static bool stdGob_IsEntryInBounds(const tGobFileEntry* pEntry, size_t gobSize)
{
    if ( pEntry->offset < 0 || pEntry->size < 0 )
    {
        return false;
    }

    size_t entryOffset = (size_t)pEntry->offset;
    size_t entrySize   = (size_t)pEntry->size;
    return entryOffset <= gobSize && entrySize <= gobSize - entryOffset;
}

void stdGob_InstallHooks(void)
{
    J3D_HOOKFUNC(stdGob_Startup);
    J3D_HOOKFUNC(stdGob_Shutdown);
    J3D_HOOKFUNC(stdGob_Load);
    J3D_HOOKFUNC(stdGob_LoadEntry);
    J3D_HOOKFUNC(stdGob_Free);
    J3D_HOOKFUNC(stdGob_FreeEntry);
    J3D_HOOKFUNC(stdGob_FileOpen);
    J3D_HOOKFUNC(stdGob_FileClose);
    J3D_HOOKFUNC(stdGob_FileSeek);
    J3D_HOOKFUNC(stdGob_FileTell);
    J3D_HOOKFUNC(stdGob_FileEOF);
    J3D_HOOKFUNC(stdGob_FileRead);
    J3D_HOOKFUNC(stdGob_FileGets);
}

void stdGob_ResetGlobals(void)
{}

void J3DAPI stdGob_Startup(tHostServices* pHS)
{
    STD_ASSERTREL(bStartup == false);
    STD_ASSERT(pHS); // Added: Guard against null pointer before copying host services.
    STD_COPYMEM(&stdGob_hs, pHS, sizeof(stdGob_hs));
    stdGob_pHS = &stdGob_hs;
    bStartup   = true;
}

void J3DAPI stdGob_Shutdown()
{
    // Fixed: Shutdown should clear the active service state only when the module was started.
    if ( bStartup )
    {
        STDLOG_STATUS("System shutdown.\n");
        bStartup   = false;
        stdGob_pHS = NULL;
    }
}

Gob* J3DAPI stdGob_Load(const char* pFilename, int numFileHandles, int bMMapFile)
{
    Gob* pGob = (Gob*)STDMALLOC(sizeof(Gob));
    if ( !pGob )
    {
        STDLOG_ERROR("Error allocating memory for Gob file.\n");
        return NULL;
    }

    STD_ZEROMEM(pGob, sizeof(Gob));
    if ( !stdGob_LoadEntry(pGob, pFilename, numFileHandles, bMMapFile) )
    {
        stdMemory_Free(pGob);
        return NULL;
    }

    STDLOG_STATUS("Gob %s opened.\n", pFilename);
    return pGob;
}

int J3DAPI stdGob_LoadEntry(Gob* pGob, const char* pFilename, int numFileHandles, int bMMapFile)
{
    // Added: Release-build guard before clearing and loading the Gob entry.
    STD_GUARD(pGob && pFilename, 0);

    STD_ZEROMEM(pGob, sizeof(*pGob));
    STD_STRCPY(pGob->aFilePath, pFilename);
    pGob->numHandles = numFileHandles;
    pGob->pCurHandle = NULL;

    if ( pGob->numHandles <= 0 )
    {
        STDLOG_ERROR("Error: Invalid Gob file handle count.\n");
        goto error;
    }

    // Fixed: Avoid overflowing the handle-array allocation.
    if ( (size_t)pGob->numHandles > SIZE_MAX / sizeof(GobFileHandle) )
    {
        STDLOG_ERROR("Error: Gob file handle count is too large.\n");
        goto error;
    }

    if ( bMMapFile )
    {
        // Fixed: The mmap path never populated the directory/hash table, so fall back to streamed I/O.
        STDLOG_WARNING("Warning: Gob memory mapping is not supported; using streamed file I/O.\n");
        bMMapFile = 0;
    }

    if ( bMMapFile )
    {
        pGob->hFile = CreateFileA(pGob->aFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_ALWAYS | CREATE_NEW, FILE_FLAG_RANDOM_ACCESS, NULL);
        STD_ASSERTREL(pGob->hFile != NULL);
        STD_ASSERTREL(pGob->hFile != (HANDLE)((HFILE)-1));

        pGob->hMapFile = CreateFileMappingA(pGob->hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        if ( pGob->hMapFile )
        {
            pGob->bFileMap = 1;
            pGob->aHandles = (GobFileHandle*)LocalAlloc(LMEM_ZEROINIT, sizeof(GobFileHandle) * pGob->numHandles);
            if ( pGob->aHandles )
            {
                pGob->pBase = MapViewOfFile(pGob->hMapFile, FILE_MAP_READ, 0, 0, 0);
                STD_ASSERTREL(pGob->pBase != NULL);
                return 1;
            }
            else
            {
                UnmapViewOfFile(pGob->pBase);
                CloseHandle(pGob->hMapFile);
            }
        }
        else
        {
            CloseHandle(pGob->hFile);
        }
    }

    pGob->bFileMap = 0;
    pGob->hGobFile = stdGob_pHS->pFileOpen(pGob->aFilePath, "rb");
    if ( !pGob->hGobFile )
    {
        STDLOG_ERROR("Error opening Gob file.\n");
        goto error;
    }

    pGob->aHandles = (GobFileHandle*)STDMALLOC(sizeof(GobFileHandle) * pGob->numHandles);
    if ( !pGob->aHandles )
    {
        STDLOG_ERROR("Error allocating memory for file handles.\n");
        goto error;
    }

    STD_ZEROMEM(pGob->aHandles, sizeof(GobFileHandle) * pGob->numHandles);

    size_t gobSize = stdGob_pHS->pFileSize(pGob->aFilePath);
    if ( gobSize < sizeof(GobFileHeader) )
    {
        STDLOG_ERROR("Error: Gob file is too small.\n");
        goto error;
    }

    GobFileHeader fileHeader;
    size_t bytesRead = stdGob_pHS->pFileRead(pGob->hGobFile, &fileHeader, sizeof(GobFileHeader));
    if ( bytesRead != sizeof(GobFileHeader) )
    {
        STDLOG_ERROR("Error reading Gob file header.\n");
        goto error;
    }

    if ( !strneq(fileHeader.signature, "GOB ", 4u) )
    {
        STDLOG_ERROR("Error: Bad signature in header of gob file.\n");
        goto error;
    }

    if ( fileHeader.version != 20 )
    {
        STDLOG_ERROR("Error: Bad version %d for gob file\n", fileHeader.version);
        goto error;
    }

    // Fixed: The directory offset is attacker-controlled in .gob files, so validate it before seeking.
    if ( fileHeader.dirOffset < 0 || (size_t)fileHeader.dirOffset > gobSize - sizeof(uint32_t) )
    {
        STDLOG_ERROR("Error: Gob directory offset is out of range.\n");
        goto error;
    }

    stdGob_pHS->pFileSeek(pGob->hGobFile, fileHeader.dirOffset, 0);
    if ( stdGob_pHS->pFileRead(pGob->hGobFile, &pGob->directory.numEntries, sizeof(pGob->directory.numEntries)) != sizeof(pGob->directory.numEntries) )
    {
        STDLOG_ERROR("Error reading Gob directory entry count.\n");
        goto error;
    }

    size_t dirBytes = gobSize - (size_t)fileHeader.dirOffset - sizeof(uint32_t);
    if ( pGob->directory.numEntries > dirBytes / sizeof(tGobFileEntry) )
    {
        STDLOG_ERROR("Error: Gob directory entry count is out of range.\n");
        goto error;
    }

    pGob->directory.aEntries = (tGobFileEntry*)STDMALLOC(sizeof(tGobFileEntry) * pGob->directory.numEntries);
    if ( !pGob->directory.aEntries )
    {
        STDLOG_ERROR("Error allocating memory for directory.\n");
        goto error;
    }

    pGob->pDirHash = stdHashtbl_New(1024u);
    STD_ASSERTREL(pGob->pDirHash);
    // Added: Keep release builds from using a NULL directory hash.
    if ( !pGob->pDirHash )
    {
        STDLOG_ERROR("Error allocating memory for directory hash.\n");
        goto error;
    }

    tGobFileEntry* pCurEntry = pGob->directory.aEntries;
    for ( uint32_t i = 0; i < pGob->directory.numEntries; ++i )
    {
        bytesRead = stdGob_pHS->pFileRead(pGob->hGobFile, pCurEntry, sizeof(tGobFileEntry));
        if ( bytesRead != sizeof(tGobFileEntry) )
        {
            STDLOG_ERROR("Error reading Gob directory entry.\n");
            goto error;
        }

        pCurEntry->aName[STD_ARRAYLEN(pCurEntry->aName) - 1] = '\0';
        if ( !stdGob_IsEntryInBounds(pCurEntry, gobSize) )
        {
            STDLOG_ERROR("Error: Gob directory entry '%s' is out of range.\n", pCurEntry->aName);
            goto error;
        }

        stdHashtbl_Add(pGob->pDirHash, pCurEntry->aName, pCurEntry);
        ++pCurEntry;
    }

    return 1;

error:
    stdGob_FreeEntry(pGob);
    return 0;
}

void J3DAPI stdGob_Free(Gob* pGob)
{
    if ( !pGob )
    {
        STDLOG_ERROR("Warning: attempt to free NULL Gob file ptr.\n");
        return;
    }

    stdGob_FreeEntry(pGob);
    stdMemory_Free(pGob);
}

void J3DAPI stdGob_FreeEntry(Gob* pGob)
{
    STD_ASSERTREL(pGob != NULL);
    // Added: Keep release builds from dereferencing a NULL Gob entry.
    STD_GUARD_VOID(pGob);

    if ( pGob->bFileMap )
    {
        // Fixed: Release all mmap resources and keep the handle state clear.
        if ( pGob->pBase )
        {
            UnmapViewOfFile(pGob->pBase);
            pGob->pBase = NULL;
        }

        if ( pGob->hMapFile )
        {
            CloseHandle(pGob->hMapFile);
            pGob->hMapFile = NULL;
        }

        if ( pGob->hFile && pGob->hFile != INVALID_HANDLE_VALUE )
        {
            CloseHandle(pGob->hFile);
            pGob->hFile = NULL;
        }

        if ( pGob->aHandles )
        {
            LocalFree(pGob->aHandles);
            pGob->aHandles = NULL;
        }

        pGob->bFileMap = 0;
    }

    if ( pGob->directory.aEntries )
    {
        stdMemory_Free(pGob->directory.aEntries);
        pGob->directory.aEntries = 0;
    }

    if ( pGob->pDirHash )
    {
        stdHashtbl_Free(pGob->pDirHash);
        pGob->pDirHash = NULL;
    }

    if ( pGob->aHandles )
    {
        stdMemory_Free(pGob->aHandles);
        pGob->aHandles = NULL;
    }

    if ( pGob->hGobFile )
    {
        stdGob_pHS->pFileClose(pGob->hGobFile);
        pGob->hGobFile = 0;
    }
}

GobFileHandle* J3DAPI stdGob_FileOpen(Gob* pGob, const char* aName)
{
    tGobFileEntry* pEntry;
    size_t numHandles;
    GobFileHandle* pHandle;
    size_t handleNum;

    STD_ASSERTREL(pGob);
    STD_ASSERTREL(aName);
    // Added: Release-build guard before using the Gob directory and handle array.
    STD_GUARD(pGob && aName, NULL);

    if ( !pGob->pDirHash || !pGob->aHandles || pGob->numHandles <= 0 )
    {
        return NULL;
    }

    STD_STRCPY(stdGob_aEntryNameBuff, aName);
    stdUtil_ToLower(stdGob_aEntryNameBuff);

    pEntry = (tGobFileEntry*)stdHashtbl_Find(pGob->pDirHash, stdGob_aEntryNameBuff);
    if ( !pEntry )
    {
        return NULL;
    }

    numHandles = pGob->numHandles;
    pHandle    = pGob->aHandles;
    handleNum =  0;
    if ( !numHandles )
    {
        STDLOG_WARNING("Warning: out of file handles for gob %s.\n", pGob->aFilePath);
        return NULL;
    }

    while ( pHandle->bUsed )
    {
        ++pHandle;
        if ( ++handleNum >= numHandles )
        {
            STDLOG_WARNING("Warning: out of file handles for gob %s.\n", pGob->aFilePath);
            return NULL;
        }
    }

    pHandle->pEntry = pEntry;
    pHandle->pGob   = pGob;
    pHandle->bUsed  = 1;
    pHandle->offset = 0;
    return pHandle;
}

void J3DAPI stdGob_FileClose(GobFileHandle* pHandle)
{
    Gob* pGob;

    STD_ASSERTREL(pHandle);
    STD_GUARD_VOID(pHandle);

    STD_ASSERTREL(pHandle->bUsed);
    // Added: Release-build guard for invalid or already-closed Gob handles.
    if ( !pHandle->bUsed )
    {
        return;
    }

    pGob = pHandle->pGob;
    pHandle->bUsed = 0;
    if ( pGob && pHandle == pGob->pCurHandle )
    {
        pGob->pCurHandle = NULL;
    }

    pHandle->pGob   = NULL;
    pHandle->pEntry = NULL;
    pHandle->offset = 0;
}

int J3DAPI stdGob_FileSeek(GobFileHandle* pHandle, int offset, int origin)
{
    STD_ASSERTREL(pHandle);
    STD_GUARD(pHandle, 0);  // Added: Guard before using handle internals.

    // Added: Validate the Gob handle's entry and parent Gob before seeking, as they are required for bounds checking and seek offset calculation.
    if ( !pHandle->pEntry || !pHandle->pGob )
    {
        return 0;
    }

    long long newOffset;
    switch ( origin )
    {
        case 0:
            newOffset = offset;
            break;
        case 1:
            newOffset = (long long)pHandle->offset + offset;
            break;
        case 2:
            newOffset = (long long)pHandle->pEntry->size + offset;
            break;
        default:
            return 0;
    }

    // Fixed: Reject negative and past-entry seeks so later reads cannot underflow entry size.
    if ( newOffset < 0 || newOffset > pHandle->pEntry->size )
    {
        return 0;
    }

    pHandle->offset = (int)newOffset;

    if ( pHandle == pHandle->pGob->pCurHandle )
    {
        pHandle->pGob->pCurHandle = NULL;
    }

    return 1;
}

int J3DAPI stdGob_FileTell(GobFileHandle* pHandle)
{
    STD_ASSERTREL(pHandle);
    STD_GUARD(pHandle, -1); // Added: guard for invalid Gob handles.

    return pHandle->offset;
}

int J3DAPI stdGob_FileEOF(GobFileHandle* pHandle)
{
    STD_ASSERTREL(pHandle);
    STD_GUARD(pHandle, 1); // Added: guard for invalid Gob handles.

    // Added: Validate the Gob handle's entry before checking EOF, as it is required for bounds checking.
    if ( !pHandle->pEntry )
    {
        return 1;
    }

    return pHandle->offset >= pHandle->pEntry->size;
}

size_t J3DAPI stdGob_FileRead(GobFileHandle* pHandle, void* data, const size_t size)
{
    // Added: Release-build guard before reading through the Gob handle.
    STD_GUARD(pHandle && data && size, 0);

    // Added: Validate the Gob handle's entry and parent Gob before reading, as they are required for bounds checking and seek offset calculation.
    if ( !pHandle->pEntry || !pHandle->pGob )
    {
        return 0;
    }

    // Fixed: Reject invalid offsets before calculating remaining bytes.
    if ( pHandle->offset < 0 || pHandle->pEntry->size < 0 || pHandle->offset > pHandle->pEntry->size )
    {
        return 0;
    }

    size_t nRead = size;
    size_t remaining = (size_t)(pHandle->pEntry->size - pHandle->offset);
    if ( remaining < nRead )
    {
        nRead = remaining;
    }

    long long seekOffset = (long long)pHandle->offset + pHandle->pEntry->offset;
    if ( seekOffset < 0 || seekOffset > INT_MAX )
    {
        return 0;
    }

    if ( pHandle->pGob->pCurHandle != pHandle )
    {
        stdGob_pHS->pFileSeek(pHandle->pGob->hGobFile, (int)seekOffset, 0);
        pHandle->pGob->pCurHandle = pHandle;
    }

    nRead = stdGob_pHS->pFileRead(pHandle->pGob->hGobFile, data, nRead);
    pHandle->offset += nRead;
    return nRead;
}

const char* J3DAPI stdGob_FileGets(GobFileHandle* pGobFileHandle, char* pStr, size_t size)
{
    // Added: Release-build guard before reading through the Gob handle.
    STD_GUARD(pGobFileHandle && pStr && size, NULL);

    // Added: Validate the Gob handle's entry and parent Gob before reading, as they are required for bounds checking and seek offset calculation.
    if ( !pGobFileHandle->pEntry || !pGobFileHandle->pGob )
    {
        return NULL;
    }

    // Fixed: Reject invalid offsets before calculating remaining bytes.
    if ( pGobFileHandle->offset < 0 || pGobFileHandle->pEntry->size < 0 || pGobFileHandle->offset >= pGobFileHandle->pEntry->size )
    {
        return NULL;
    }

    Gob* pGob = pGobFileHandle->pGob;
    if ( pGob->pCurHandle != pGobFileHandle )
    {
        long long seekOffset = (long long)pGobFileHandle->offset + pGobFileHandle->pEntry->offset;
        if ( seekOffset < 0 || seekOffset > INT_MAX )
        {
            return NULL;
        }

        stdGob_pHS->pFileSeek(pGob->hGobFile, (int)seekOffset, 0);
        pGobFileHandle->pGob->pCurHandle = pGobFileHandle;
    }

    size_t nRead = size;
    size_t remaining = (size_t)(pGobFileHandle->pEntry->size - pGobFileHandle->offset);
    if ( remaining + 1 < nRead )
    {
        nRead = remaining + 1;
    }

    pStr = stdGob_pHS->pFileGets(pGobFileHandle->pGob->hGobFile, pStr, nRead);
    if ( pStr )
    {
        pGobFileHandle->offset += strlen(pStr);
    }

    return pStr;
}