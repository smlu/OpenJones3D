#include "stdGob.h"
#include <j3dcore/j3dhook.h>
#include <std/General/std.h>
#include <std/General/stdHashTbl.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/RTI/symbols.h>

static bool bStartup = false;
static char stdGob_aEntryNameBuff[128] = { 0 };

static tHostServices stdGob_hs   = { 0 };
static tHostServices* stdGob_pHS = NULL;

// TODO: The file read functions don't have implemented interacting with memory mapped file.

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
{
    /*memset(&stdGob_aEntryNameBuff, 0, sizeof(stdGob_aEntryNameBuff));
    memset(&stdGob_hs, 0, sizeof(stdGob_hs));
    memset(&bStartup, 0, sizeof(bStartup));
    memset(&stdGob_pHS, 0, sizeof(stdGob_pHS));*/
}

void J3DAPI stdGob_Startup(tHostServices* pHS)
{
    STD_ASSERTREL(bStartup == false);
    memcpy(&stdGob_hs, pHS, sizeof(stdGob_hs));
    stdGob_pHS = &stdGob_hs;
    bStartup   = true;
}

void J3DAPI stdGob_Shutdown()
{
    if ( !bStartup )
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

    memset(pGob, 0, sizeof(Gob));
    if ( !stdGob_LoadEntry(pGob, pFilename, numFileHandles, bMMapFile) )
    {
        return NULL;
    }

    STDLOG_STATUS("Gob %s opened.\n", pFilename);
    return pGob;
}

int J3DAPI stdGob_LoadEntry(Gob* pGob, const char* pFilename, int numFileHandles, int bMMapFile)
{
    STD_STRCPY(pGob->aFilePath, pFilename);
    pGob->numHandles = numFileHandles;
    pGob->pCurHandle = NULL;

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
        return 0;
    }

    pGob->aHandles = (GobFileHandle*)STDMALLOC(sizeof(GobFileHandle) * pGob->numHandles);
    if ( !pGob->aHandles )
    {
        STDLOG_ERROR("Error allocating memory for file handles.\n");
        return 0;
    }

    memset(pGob->aHandles, 0, sizeof(GobFileHandle) * pGob->numHandles);

    GobFileHeader fileHeader;
    size_t bytesRead = stdGob_pHS->pFileRead(pGob->hGobFile, &fileHeader, sizeof(GobFileHeader));
    STD_ASSERTREL(bytesRead == sizeof(GobFileHeader));

    if ( strncmp(fileHeader.signature, "GOB ", 4u) )
    {
        STDLOG_ERROR("Error: Bad signature in header of gob file.\n");
        return 0;
    }

    if ( fileHeader.version != 20 )
    {
        STDLOG_ERROR("Error: Bad version %d for gob file\n", fileHeader.version);
        return 0;
    }

    stdGob_pHS->pFileSeek(pGob->hGobFile, fileHeader.dirOffset, 0);
    stdGob_pHS->pFileRead(pGob->hGobFile, &pGob->directory.numEntries, 4);

    pGob->directory.aEntries = (tGobFileEntry*)STDMALLOC(sizeof(tGobFileEntry) * pGob->directory.numEntries);
    if ( !pGob->directory.aEntries )
    {
        STDLOG_ERROR("Error allocating memory for directory.\n");
        return 0;
    }

    pGob->pDirHash = stdHashtbl_New(1024u);
    STD_ASSERTREL(pGob->pDirHash);

    tGobFileEntry* pCurEntry = pGob->directory.aEntries;
    for ( uint32_t i = 0; i < pGob->directory.numEntries; ++i )
    {
        bytesRead = stdGob_pHS->pFileRead(pGob->hGobFile, pCurEntry, sizeof(tGobFileEntry));
        STD_ASSERTREL(bytesRead == sizeof(tGobFileEntry));
        stdHashtbl_Add(pGob->pDirHash, pCurEntry->aName, pCurEntry);
        ++pCurEntry;
    }

    return 1;
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
    if ( pGob->bFileMap )
    {
        UnmapViewOfFile(pGob->pBase);
        CloseHandle(pGob->hMapFile);
        CloseHandle(pGob->hFile);
        return;
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

    stdGob_pHS->pFileClose(pGob->hGobFile);
    pGob->hGobFile = 0;
}

GobFileHandle* J3DAPI stdGob_FileOpen(Gob* pGob, const char* aName)
{
    tGobFileEntry* pEntry;
    size_t numHandles;
    GobFileHandle* pHandle;
    size_t handleNum;

    STD_ASSERTREL(pGob);
    STD_ASSERTREL(aName);

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

    STD_ASSERTREL(pHandle->bUsed);
    pGob = pHandle->pGob;
    pHandle->bUsed = 0;
    if ( pHandle == pGob->pCurHandle )
    {
        pGob->pCurHandle = NULL;
    }
}

int J3DAPI stdGob_FileSeek(GobFileHandle* pHandle, int offset, int origin)
{
    STD_ASSERTREL(pHandle);

    switch ( origin )
    {
        case 0:
            pHandle->offset = offset;
            break;
        case 1:
            pHandle->offset = offset + pHandle->offset;
            break;
        case 2:
            pHandle->offset = offset + pHandle->pEntry->size;
            break;
        default:
            return 0;
    }

    if ( pHandle == pHandle->pGob->pCurHandle )
    {
        pHandle->pGob->pCurHandle = NULL;
    }

    return 1;
}

int J3DAPI stdGob_FileTell(GobFileHandle* pHandle)
{
    STD_ASSERTREL(pHandle);
    return pHandle->offset;
}

int J3DAPI stdGob_FileEOF(GobFileHandle* pHandle)
{
    STD_ASSERTREL(pHandle);
    return pHandle->offset >= pHandle->pEntry->size - 1;
}

size_t J3DAPI stdGob_FileRead(GobFileHandle* pHandle, void* data, const size_t size)
{
    if ( pHandle->pGob->pCurHandle != pHandle )
    {
        stdGob_pHS->pFileSeek(pHandle->pGob->hGobFile, pHandle->offset + pHandle->pEntry->offset, 0);
        pHandle->pGob->pCurHandle = pHandle;
    }

    size_t nRead = size;
    if ( (pHandle->pEntry->size - pHandle->offset) < size )
    {
        nRead = pHandle->pEntry->size - pHandle->offset;
    }

    nRead = stdGob_pHS->pFileRead(pHandle->pGob->hGobFile, data, nRead);
    pHandle->offset += nRead;
    return nRead;
}

const char* J3DAPI stdGob_FileGets(GobFileHandle* pGobFileHandle, char* pStr, size_t size)
{
    if ( (pGobFileHandle->pEntry->size - 1) < pGobFileHandle->offset )
    {
        return NULL;
    }

    Gob* pGob = pGobFileHandle->pGob;
    if ( pGob->pCurHandle != pGobFileHandle )
    {
        stdGob_pHS->pFileSeek(pGob->hGobFile, pGobFileHandle->offset + pGobFileHandle->pEntry->offset, 0);
        pGobFileHandle->pGob->pCurHandle = pGobFileHandle;
    }

    int nRead = size;
    if ( (pGobFileHandle->pEntry->size - pGobFileHandle->offset + 1) < size )
    {
        nRead = pGobFileHandle->pEntry->size - pGobFileHandle->offset + 1;
    }

    pStr = stdGob_pHS->pFileGets(pGobFileHandle->pGob->hGobFile, pStr, nRead);
    if ( pStr )
    {
        pGobFileHandle->offset += strlen(pStr);
    }

    return pStr;
}