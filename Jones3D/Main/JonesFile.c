#include <Jones3D/types.h> // Note, don't move as it requires including windows.h without WIN32_LEAN_AND_MEAN  being defined

#include "JonesFile.h"
#include <j3dcore/j3dhook.h>

#include <Jones3D/RTI/addresses.h>
#include <Jones3D/RTI/symbols.h>

#include <std/General/std.h>
#include <std/General/stdFileUtil.h>
#include <std/General/stdFnames.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdGob.h>

#define JF_MAX_HANDLES 32

#define JF_RES_INSTALL  0
#define JF_RES_CWD      1 // current working dir
#define JF_RES_CD       2
#define JF_RES_RESOURCE 3 // Added

#define JF_ISVALID_FH(fh) (((fh) >= (1)) ? (((fh) <= JF_MAX_HANDLES) ? 1 : 0) : 0)

bool JonesFile_bStartup = false; // Added: init to false
bool JonesFile_bOpened  = false; // Added: init to false

int JonesFile_hsInitCount;
tHostServices* JonesFile_pHS;
tHostServices JonesFile_sysHS;
tHostServices* JonesFile_pSysEnv;

JonesResource JonesFile_aResources[4] = { 0 }; // Changed: Expand array for resource path; Added: Init to 0

char JonesFile_aPathBuf[128]; // Fixed: increased buffer size to 128, was 48
JonesFileHandle JonesFile_aFileHandles[JF_MAX_HANDLES];

void JonesFile_InitServices(void);
void JonesFile_ResetServices(void);

void J3DAPI JonesFile_OpenResource(JonesResource* pRsource, const char* pPath);
void J3DAPI JonesFile_CloseResource(JonesResource* pResource);

tFileHandle J3DAPI JonesFile_OpenFile(const char* pFilename, const char* mode);
int J3DAPI JonesFile_CloseFile(tFileHandle fh);
size_t J3DAPI JonesFile_FileRead(tFileHandle fh, void* pDst, size_t size);
size_t J3DAPI JonesFile_FileWrite(tFileHandle fh, const void* pData, size_t size);
char* J3DAPI JonesFile_FileGets(tFileHandle fh, char* pStr, size_t size);
int J3DAPI JonesFile_FileEOF(tFileHandle fh);
int J3DAPI JonesFile_FileTell(tFileHandle fh);
int J3DAPI JonesFile_FileSeek(tFileHandle fh, int offset, int origin);
size_t J3DAPI JonesFile_FileSize(const char* pFilename);
int JonesFile_FilePrintf(tFileHandle fh, const char* aFormat, ...);

void JonesFile_InstallHooks(void)
{
    J3D_HOOKFUNC(JonesFile_Startup);
    J3D_HOOKFUNC(JonesFile_Shutdown);
    J3D_HOOKFUNC(JonesFile_Open);
    J3D_HOOKFUNC(JonesFile_Close);
    J3D_HOOKFUNC(JonesFile_GetCurrentCDNum);
    J3D_HOOKFUNC(JonesFile_FileExists);
    J3D_HOOKFUNC(JonesFile_SetInstallPath);
    J3D_HOOKFUNC(JonesFile_SetCDPath);
    J3D_HOOKFUNC(JonesFile_InitServices);
    J3D_HOOKFUNC(JonesFile_ResetServices);
    J3D_HOOKFUNC(JonesFile_OpenResource);
    J3D_HOOKFUNC(JonesFile_CloseResource);
    J3D_HOOKFUNC(JonesFile_OpenFile);
    J3D_HOOKFUNC(JonesFile_CloseFile);
    J3D_HOOKFUNC(JonesFile_FileRead);
    J3D_HOOKFUNC(JonesFile_FileWrite);
    J3D_HOOKFUNC(JonesFile_FileGets);
    J3D_HOOKFUNC(JonesFile_FileEOF);
    J3D_HOOKFUNC(JonesFile_FileTell);
    J3D_HOOKFUNC(JonesFile_FileSeek);
    J3D_HOOKFUNC(JonesFile_FileSize);
    J3D_HOOKFUNC(JonesFile_FilePrintf);
}

void JonesFile_ResetGlobals(void)
{}

void J3DAPI JonesFile_Startup(tHostServices* pHS)
{
    if ( !JonesFile_bStartup )
    {
        stdGob_Startup(pHS);
        JonesFile_hsInitCount = 0;
        JonesFile_bStartup    = true;
    }
}

void JonesFile_Shutdown(void)
{
    if ( JonesFile_bStartup )
    {
        stdGob_Shutdown();
        JonesFile_hsInitCount = 0;
        JonesFile_bStartup    = false;
    }
}

int J3DAPI JonesFile_Open(tHostServices* pEnv, const char* pInstallPath, const char* pPathCD)
{
    if ( !JonesFile_bStartup )
    {
        return 1;
    }

    if ( JonesFile_bOpened )
    {
        STDLOG_ERROR("Warning: System already open!\n");
        return 1;
    }

    memcpy(&JonesFile_sysHS, pEnv, sizeof(JonesFile_sysHS));
    JonesFile_pHS     = &JonesFile_sysHS;
    JonesFile_pSysEnv = pEnv;

    JonesFile_InitServices(); // This will override current pEnv

    // Add resource paths:
    // 0 - install path
    // 1 - current work dir
    // 2 - CD path
    // 3 - resource dir
    STD_ZEROMEM(JonesFile_aResources, sizeof(JonesFile_aResources));

    if ( pInstallPath )
    {
        JonesFile_SetInstallPath(pInstallPath);
    }

    if ( pPathCD )
    {
        JonesFile_SetCDPath(pPathCD);
    }

    // Add path to current working dir
    JonesFile_OpenResource(&JonesFile_aResources[JF_RES_CWD], ".");

    STD_ZEROMEM(JonesFile_aFileHandles, sizeof(JonesFile_aFileHandles));

    JonesFile_bOpened = true;
    return 0;
}

void JonesFile_Close(void)
{
    if ( !JonesFile_bOpened )

    {
        STDLOG_ERROR("Warning: System already closed!\n");
        return;
    }

    for ( size_t i = 0; i < JF_MAX_HANDLES; ++i )
    {
        if ( JonesFile_aFileHandles[i].bUsed )
        {
            JonesFile_CloseFile(i + 1);
            STDLOG_WARNING("File left open %s. Closed now!\n", JonesFile_aFileHandles[i].aName); // Changed: Changed to warn log from error log. Added 'Closed now!'
        }
    }

    JonesFile_ResetServices();
    JonesFile_CloseResource(&JonesFile_aResources[JF_RES_INSTALL]);
    JonesFile_CloseResource(&JonesFile_aResources[JF_RES_CD]);
    JonesFile_CloseResource(&JonesFile_aResources[JF_RES_CWD]);
    JonesFile_CloseResource(&JonesFile_aResources[JF_RES_RESOURCE]); // Added
    STD_ZEROMEM(JonesFile_aResources, sizeof(JonesFile_aResources)); // Added: clear
    JonesFile_bOpened = false;
}

int JonesFile_GetCurrentCDNum(void)
{
    char aPath[128];
    stdFnames_MakePath(aPath, sizeof(aPath), JonesFile_aResources[JF_RES_CD].aPath, "CD1.GOB");
    if ( std_g_pHS->pFileSize(aPath) )
    {
        return 1;
    }

    stdFnames_MakePath(aPath, sizeof(aPath), JonesFile_aResources[JF_RES_CD].aPath, "CD2.GOB");
    if ( std_g_pHS->pFileSize(aPath) )
    {
        return 2;
    }

    return 0;
}

int J3DAPI JonesFile_FileExists(const char* pFilename)
{
    tFileHandle fh = std_g_pHS->pFileOpen(pFilename, "rb");
    if ( !fh )
    {
        return 0;
    }

    std_g_pHS->pFileClose(fh);
    return 1;
}

void J3DAPI JonesFile_SetInstallPath(const char* pPath)
{
    JonesFile_CloseResource(&JonesFile_aResources[JF_RES_RESOURCE]); // Added
    JonesFile_CloseResource(&JonesFile_aResources[JF_RES_INSTALL]);
    if ( *pPath )
    {
        JonesFile_OpenResource(&JonesFile_aResources[JF_RES_INSTALL], pPath);

        // Added: Add resource dir path
        STD_MAKEPATH(JonesFile_aPathBuf, pPath, "Resource");
        JonesFile_OpenResource(&JonesFile_aResources[JF_RES_RESOURCE], JonesFile_aPathBuf);
    }
}

const char* JonesFile_GetInstallPath(void)
{
    return JonesFile_aResources[JF_RES_INSTALL].aPath;
}

const char* JonesFile_GetResourcePath(void)
{
    return JonesFile_aResources[JF_RES_RESOURCE].aPath;
}

const char* JonesFile_GetWorkingDirPath(void)
{
    return JonesFile_aResources[JF_RES_CWD].aPath;
}

void J3DAPI JonesFile_SetCDPath(const char* pPath)
{
    JonesFile_CloseResource(&JonesFile_aResources[JF_RES_CD]);
    if ( *pPath )
    {
        char aPath[128];
        STD_FORMAT(aPath, "%sresource", pPath);
        JonesFile_OpenResource(&JonesFile_aResources[JF_RES_CD], aPath);
    }
    else
    {
        STDLOG_WARNING("Empty CD path specified.\n");
    }
}

const char* JonesFile_GetCDPath(void)
{
    return JonesFile_aResources[JF_RES_CD].aPath;
}

void JonesFile_InitServices(void)
{
    if ( !JonesFile_hsInitCount )
    {
        JonesFile_pSysEnv->pFileOpen   = JonesFile_OpenFile;
        JonesFile_pSysEnv->pFileClose  = JonesFile_CloseFile;
        JonesFile_pSysEnv->pFileRead   = JonesFile_FileRead;
        JonesFile_pSysEnv->pFileGets   = JonesFile_FileGets;
        JonesFile_pSysEnv->pFileWrite  = JonesFile_FileWrite;
        JonesFile_pSysEnv->pFileEOF    = JonesFile_FileEOF;
        JonesFile_pSysEnv->pFileTell   = JonesFile_FileTell;
        JonesFile_pSysEnv->pFileSeek   = JonesFile_FileSeek;
        JonesFile_pSysEnv->pFileSize   = JonesFile_FileSize;
        JonesFile_pSysEnv->pFilePrintf = JonesFile_FilePrintf;
    }

    ++JonesFile_hsInitCount;
}

void JonesFile_ResetServices(void)
{
    if ( JonesFile_hsInitCount )
    {
        JonesFile_pSysEnv->pFileOpen   = JonesFile_sysHS.pFileOpen;
        JonesFile_pSysEnv->pFileClose  = JonesFile_sysHS.pFileClose;
        JonesFile_pSysEnv->pFileRead   = JonesFile_sysHS.pFileRead;
        JonesFile_pSysEnv->pFileGets   = JonesFile_sysHS.pFileGets;
        JonesFile_pSysEnv->pFileWrite  = JonesFile_sysHS.pFileWrite;
        JonesFile_pSysEnv->pFileEOF    = JonesFile_sysHS.pFileEOF;
        JonesFile_pSysEnv->pFileTell   = JonesFile_sysHS.pFileTell;
        JonesFile_pSysEnv->pFileSeek   = JonesFile_sysHS.pFileSeek;
        JonesFile_pSysEnv->pFileSize   = JonesFile_sysHS.pFileSize;
        JonesFile_pSysEnv->pFilePrintf = JonesFile_sysHS.pFilePrintf;
        --JonesFile_hsInitCount;
    }
}

void J3DAPI JonesFile_OpenResource(JonesResource* pRsource, const char* pPath)
{
    STD_STRCPY(pRsource->aPath, pPath);
    JonesFile_ResetServices();

    // Now find all *.gob files in dir
    pRsource->numGobFiles = 0;

    tFoundFileInfo fileInfo;
    FindFileData* pFileData = stdFileUtil_NewFind(pPath, /*mode=*/3, "gob");
    while ( stdFileUtil_FindNext(pFileData, &fileInfo) )
    {
        if ( fileInfo.aName[0] != '.' )
        {
            // Fixed: JonesResource stores a fixed number of GOB handles.
            if ( pRsource->numGobFiles >= STD_ARRAYLEN(pRsource->aGobFiles) )
            {
                STDLOG_WARNING("Warning: too many GOB files in '%s'. Extra files ignored.\n", pPath);
                break;
            }

            stdFnames_MakePath(JonesFile_aPathBuf, sizeof(JonesFile_aPathBuf), pPath, fileInfo.aName);
            pRsource->aGobFiles[pRsource->numGobFiles] = stdGob_Load(JonesFile_aPathBuf, /*numFileHandles*/16, /*bMapFile*/0); // TODO: After file mapping is fixed in stdGob enable file mapping here
            if ( !pRsource->aGobFiles[pRsource->numGobFiles] )
            {
                STDLOG_ERROR("Error opening gob '%s'.\n", JonesFile_aPathBuf);
                break;
            }

            ++pRsource->numGobFiles;
        }
    }

    stdFileUtil_DisposeFind(pFileData);
    JonesFile_InitServices();
}

void J3DAPI JonesFile_CloseResource(JonesResource* pResource)
{
    for ( size_t i = 0; i < pResource->numGobFiles; ++i )
    {
        stdGob_Free(pResource->aGobFiles[i]);
    }

    pResource->numGobFiles = 0;
}

tFileHandle J3DAPI JonesFile_OpenFile(const char* pFilename, const char* mode)
{
    size_t handle;
    for ( handle = 0; handle < JF_MAX_HANDLES && JonesFile_aFileHandles[handle].bUsed; ++handle )
    {
        ;
    }

    if ( handle >= JF_MAX_HANDLES )
    {
        STDLOG_ERROR("Out of file handles!\n");
        return 0;
    }

    // Try to open file by provided path
    bool bFileFound = false;
    tFileHandle fh = JonesFile_pHS->pFileOpen(pFilename, mode);
    if ( fh )
    {
        bFileFound = true;
        JonesFile_aFileHandles[handle].bFileHandle = 1;
        JonesFile_aFileHandles[handle].hFile       = fh;
        JonesFile_aFileHandles[handle].bUsed       = 1;
        STD_STRCPY(JonesFile_aFileHandles[handle].aName, pFilename);
    }
    else // No file was found, try to find it in stored resources
    {
        // Search the resources:
        // 0 - install path
        // 1 - current work dir path
        // 2 - CD path
        JonesResource* pCurRes = JonesFile_aResources;
        for ( size_t resNum = 0; resNum < STD_ARRAYLEN(JonesFile_aResources) && !bFileFound; ++resNum )
        {
            if ( pCurRes->aPath[0] )
            {
                if ( pCurRes != &JonesFile_aResources[JF_RES_CD] )// If not CD resource
                {
                    stdFnames_MakePath(JonesFile_aPathBuf, sizeof(JonesFile_aPathBuf), pCurRes->aPath, pFilename);
                    fh = JonesFile_pHS->pFileOpen(JonesFile_aPathBuf, mode);
                    if ( fh )
                    {
                        bFileFound = true;
                        JonesFile_aFileHandles[handle].bFileHandle = 1;
                        JonesFile_aFileHandles[handle].hFile       = fh;
                        JonesFile_aFileHandles[handle].bUsed       = 1;
                        STD_STRCPY(JonesFile_aFileHandles[handle].aName, JonesFile_aPathBuf);
                    }
                }

                if ( !bFileFound )
                {
                    // File was not found on disk try to search for file in GOB resources
                    for ( size_t gobNum = 0; gobNum < pCurRes->numGobFiles && !bFileFound; ++gobNum )
                    {
                        GobFileHandle* pGFH = stdGob_FileOpen(pCurRes->aGobFiles[gobNum], pFilename);
                        if ( pGFH )
                        {
                            bFileFound = true;
                            JonesFile_aFileHandles[handle].bFileHandle    = 0;
                            JonesFile_aFileHandles[handle].pGobFileHandle = pGFH;
                            JonesFile_aFileHandles[handle].bUsed          = 1;
                            STD_STRCPY(JonesFile_aFileHandles[handle].aName, pFilename);
                        }
                    }
                }
            }

            ++pCurRes;
        }
    }

    if ( bFileFound )
    {
        return handle + 1; // Note, file handle gets incremented here by one... I guess so the valid handle is always greater than 0
    }

    return 0;
}

int J3DAPI JonesFile_CloseFile(tFileHandle fh)
{
    STD_ASSERTREL(JF_ISVALID_FH(fh));
    JonesFileHandle* pHandle = &JonesFile_aFileHandles[fh - 1];  // Note, fh has to be decremented here because OpenFile incremented it by 1 so the valid handle is always greater than 0
    if ( pHandle->bUsed )
    {
        if ( pHandle->bFileHandle )
        {
            JonesFile_pHS->pFileClose(pHandle->hFile);
            pHandle->hFile = 0;
        }
        else
        {
            stdGob_FileClose(pHandle->pGobFileHandle);
            pHandle->pGobFileHandle = NULL;
        }
    }

    STD_ZEROMEM(pHandle, sizeof(JonesFileHandle));
    return 0;
}

size_t J3DAPI JonesFile_FileRead(tFileHandle fh, void* pDst, size_t size)
{
    STD_ASSERTREL(JF_ISVALID_FH(fh));
    JonesFileHandle* pHandle = &JonesFile_aFileHandles[fh - 1];
    if ( pHandle->bFileHandle )
    {
        return JonesFile_pHS->pFileRead(pHandle->hFile, pDst, size);
    }
    else
    {
        return stdGob_FileRead(pHandle->pGobFileHandle, pDst, size);
    }
}

size_t J3DAPI JonesFile_FileWrite(tFileHandle fh, const void* pData, size_t size)
{
    STD_ASSERTREL(JF_ISVALID_FH(fh));
    JonesFileHandle* pHandle = &JonesFile_aFileHandles[fh - 1];
    if ( pHandle->bFileHandle )
    {
        return JonesFile_pHS->pFileWrite(pHandle->hFile, pData, size);
    }

    STDLOG_ERROR("Error tried to write to bundled file '%s'\n", pHandle->aName);
    return 0;
}

char* J3DAPI JonesFile_FileGets(tFileHandle fh, char* pStr, size_t size)
{
    STD_ASSERTREL(JF_ISVALID_FH(fh));
    JonesFileHandle* pHandle = &JonesFile_aFileHandles[fh - 1];
    if ( pHandle->bFileHandle )
    {
        return JonesFile_pHS->pFileGets(pHandle->hFile, pStr, size);
    }
    else
    {
        return (char*)stdGob_FileGets(pHandle->pGobFileHandle, pStr, size);
    }
}

int J3DAPI JonesFile_FileEOF(tFileHandle fh)
{
    STD_ASSERTREL(JF_ISVALID_FH(fh));
    JonesFileHandle* pHandle = &JonesFile_aFileHandles[fh - 1];
    if ( pHandle->bFileHandle )
    {
        return JonesFile_pHS->pFileEOF(pHandle->hFile);
    }
    else
    {
        return stdGob_FileEOF(pHandle->pGobFileHandle);
    }
}

int J3DAPI JonesFile_FileTell(tFileHandle fh)
{
    STD_ASSERTREL(JF_ISVALID_FH(fh));
    JonesFileHandle* pHandle = &JonesFile_aFileHandles[fh - 1];
    if ( pHandle->bFileHandle )
    {
        return JonesFile_pHS->pFileTell(pHandle->hFile);
    }
    else
    {
        return stdGob_FileTell(pHandle->pGobFileHandle);
    }
}

int J3DAPI JonesFile_FileSeek(tFileHandle fh, int offset, int origin)
{
    STD_ASSERTREL(JF_ISVALID_FH(fh));
    JonesFileHandle* pHandle = &JonesFile_aFileHandles[fh - 1];
    if ( pHandle->bFileHandle )
    {
        return JonesFile_pHS->pFileSeek(pHandle->hFile, offset, origin);
    }
    else
    {
        return stdGob_FileSeek(pHandle->pGobFileHandle, offset, origin);
    }
}

size_t J3DAPI JonesFile_FileSize(const char* pFilename)
{
    return JonesFile_pHS->pFileSize(pFilename);
}

int JonesFile_FilePrintf(tFileHandle fh, const char* aFormat, ...)
{
    STD_ASSERTREL(JF_ISVALID_FH(fh));

    va_list args;
    va_start(args, aFormat);
    size_t size = vsnprintf(std_g_genBuffer, sizeof(std_g_genBuffer), aFormat, args);
    va_end(args);

    JonesFileHandle* pHandle = &JonesFile_aFileHandles[fh - 1];
    if ( pHandle->bFileHandle )
    {
        JonesFile_pHS->pFileWrite(pHandle->hFile, std_g_genBuffer, size); // Fixed: Fixed writing to file by using pFileWrite function instead of pFilePrintf
    }
    else
    {
        STDLOG_ERROR("Error tried to printf to bundled file '%s'\n", pHandle->aName);
    }

    return 0;
}