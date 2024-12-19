
#include <Windows.h> // timeGetTime, include first to avoid undefined warning due to WIN32_LEAN_AND_MEAN

#include "std.h"
#include "stdMemory.h"
#include "stdPlatform.h"
#include "stdUtil.h"

#include <j3dcore/j3dhook.h>
#include <std/RTI/symbols.h>

static bool stdPlatform_bAssert = false;

void stdPlatform_InstallHooks(void)
{
    J3D_HOOKFUNC(stdPlatform_InitServices);
    J3D_HOOKFUNC(stdPlatform_ClearServices);
    J3D_HOOKFUNC(stdPlatform_GetTimeMsec);
    J3D_HOOKFUNC(stdPlatform_Assert);
    J3D_HOOKFUNC(stdPlatform_Printf);
    J3D_HOOKFUNC(stdPlatform_AllocHandle);
    J3D_HOOKFUNC(stdPlatform_FreeHandle);
    J3D_HOOKFUNC(stdPlatform_ReallocHandle);
    J3D_HOOKFUNC(stdPlatform_LockHandle);
    J3D_HOOKFUNC(stdPlatform_UnlockHandle);
}

void stdPlatform_ResetGlobals(void)
{
    //memset(&stdPlatform_bAssert, 0, sizeof(stdPlatform_bAssert));
}

// TODO: stdFile* functions should be moved here to stdPlatform

int J3DAPI stdPlatform_InitServices(tHostServices* pHS)
{
    pHS->unknown1 = 1000.0f; // sec to msec converter constant

    pHS->pMessagePrint = stdPlatform_Printf;
    pHS->pStatusPrint  = stdPlatform_Printf;
    pHS->pWarningPrint = stdPlatform_Printf;
    pHS->pErrorPrint   = stdPlatform_Printf;
    pHS->pDebugPrint   = stdPlatform_Printf;

    pHS->pAssert = stdPlatform_Assert;
    pHS->pAtExit = NULL;

    pHS->pMalloc  = stdMemory_BlockMalloc;
    pHS->pFree    = stdMemory_BlockFree;
    pHS->pRealloc = stdMemory_BlockRealloc;

    pHS->pGetTimeMsec = stdPlatform_GetTimeMsec;

    pHS->pFileOpen      = stdFileOpen;
    pHS->pFileClose     = stdFileClose;
    pHS->pFileRead      = stdFileRead;
    pHS->pFileGets      = stdFileGets;
    pHS->pFileWrite     = stdFileWrite;
    pHS->pFileEOF       = stdFileEof;
    pHS->pFileTell      = stdFileTell;
    pHS->pFileSeek      = stdFileSeek;
    pHS->pFileSize      = stdFileSize;
    pHS->pFilePrintf    = stdFilePrintf;
    pHS->pFileGetws     = stdFileGetws;

    pHS->pAllocHandle   = stdPlatform_AllocHandle;
    pHS->pFreeHandle    = stdPlatform_FreeHandle;
    pHS->pReallocHandle = stdPlatform_ReallocHandle;

    pHS->pLockHandle   = stdPlatform_LockHandle;
    pHS->pUnlockHandle = stdPlatform_UnlockHandle;

    return CoInitialize(0);
}

void J3DAPI stdPlatform_ClearServices(tHostServices* pHS)
{
    memset(pHS, 0, sizeof(tHostServices));
    CoUninitialize();
}

unsigned int stdPlatform_GetTimeMsec(void)
{
    return timeGetTime(); // TODO: Use more precise timer
}

J3DNORETURN void J3DAPI stdPlatform_Assert(const char* pErrorStr, const char* pFilename, int linenum)
{
    if ( stdPlatform_bAssert )
    {
        DebugBreak();
        exit(1);
    }
    stdPlatform_bAssert = true;

    size_t pos     = 0;
    size_t fnpos   = 0;
    bool bFoundSep = false;

    for ( char chr = *pFilename; chr; ++pos )
    {
        if ( chr == '\\' )
        {
            bFoundSep = true;
            fnpos     = pos;
        }

        chr = pFilename[pos + 1];
    }

    if ( bFoundSep )
    {
        ++fnpos;
    }

    char aText[512];
    STD_FORMAT(aText, "%s(%d):  %s\n", &pFilename[fnpos], linenum, pErrorStr);
    std_g_pHS->pErrorPrint("ASSERT: %s", aText);
    MessageBoxA(NULL, aText, "Assert Handler", MB_TASKMODAL);

    DebugBreak();
    exit(1);
}

int stdPlatform_Printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(std_g_genBuffer, sizeof(std_g_genBuffer), format, args);
    va_end(args);

    OutputDebugStringA(std_g_genBuffer); // TODO: write also to printf
    return 1;
}

void* J3DAPI stdPlatform_AllocHandle(size_t size)
{
    return malloc(size);
}

void J3DAPI stdPlatform_FreeHandle(void* pData)
{
    free(pData);
}

void* J3DAPI stdPlatform_ReallocHandle(void* pMemory, size_t newSize)
{
    return realloc(pMemory, newSize);
}

int J3DAPI stdPlatform_LockHandle(int a1)
{
    return a1;
}

void J3DAPI stdPlatform_UnlockHandle()
{

}
