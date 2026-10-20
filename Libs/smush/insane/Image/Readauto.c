// Readauto.c: the `READANIM=insane_auto` animation-file reader (original 0x004E33F0-0x004E3810).
#include <smush/insane/Image/Readauto.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

#include <stddef.h>

static LecModule readauto_module = { readauto_Register, readauto_Unregister, readauto_Startup, readauto_Shutdown };
static ReadAnimInterface readauto_interface = {
    readauto_Query, readauto_Open, readauto_Close, readauto_Op3, readauto_Op4, readauto_Read, readauto_Recognize
};

static MemoryDebugInterface* readauto_pAlloc;
static LecShare* readauto_pFileService;
static LecShare* readauto_pShare;
static void* readauto_pParam;

void Readauto_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(readauto_Register);
    J3D_HOOKFUNC(readauto_Unregister);
    J3D_HOOKFUNC(readauto_Startup);
    J3D_HOOKFUNC(readauto_Shutdown);
    J3D_HOOKFUNC(readauto_Query);
    J3D_HOOKFUNC(readauto_Read);
    J3D_HOOKFUNC(readauto_Open);
    J3D_HOOKFUNC(readauto_Close);
    J3D_HOOKFUNC(readauto_Op3);
    J3D_HOOKFUNC(readauto_Op4);
}

void Readauto_ResetGlobals(void)
{
    readauto_pAlloc       = NULL;
    readauto_pFileService = NULL;
    readauto_pShare       = NULL;
    readauto_pParam       = NULL;
}

int J3DAPI readauto_Register(LecShare* pShare, void* pParam)
{
    readauto_pShare = pShare;
    readauto_pParam = pParam;
    pShare->Publish("LECMODULE=insane_readauto", &readauto_module);
    readauto_pShare->Publish("READANIM=insane_auto", &readauto_interface);
    return 0;
}

void readauto_Unregister(void)
{
    readauto_pShare->Unpublish("READANIM=insane_auto");
    readauto_pShare->Unpublish("LECMODULE=insane_readauto");
}

int readauto_Startup(void)
{
    readauto_pAlloc       = (MemoryDebugInterface*)readauto_pShare->Subscribe("MEMORY.workspace", readauto_pParam);
    readauto_pFileService = (LecShare*)readauto_pShare->Subscribe("FILE.io", readauto_pParam);
    if ( !readauto_pAlloc )
    {
        readauto_pShare->ReportError("ReadAuto Module: Cannot allocate memory services.");
        return -1;
    }

    if ( !readauto_pFileService )
    {
        readauto_pShare->ReportError("ReadAuto Module: Cannot allocate file services.");
        return -1;
    }

    return 0;
}

void readauto_Shutdown(void)
{
    readauto_pShare->Unsubscribe(readauto_pAlloc);
    readauto_pShare->Unsubscribe(readauto_pFileService);
}

const char* J3DAPI readauto_Query(int code)
{
    switch ( code )
    {
        case 0:
            return "Auto Reader";

        case 1:
            return "Reads an animation file automatically using published modules.";

        case 2:
            return "This module searches the system for all published READANIM modules and queries them in sequence from earliest to latest "
                   "to see if any of them are compatible with a given file.  The check is first done using filenames only.  If none are "
                   "found to match, data-level comparisons are done.";
    }

    if ( code < 100 || code >= 120 )
    {
        return "";
    }

    const char* pText = NULL;
    int count         = readauto_pShare->Count("READANIM");
    for ( int i = 0; i < count; ++i )
    {
        if ( code < 100 )
        {
            break;
        }

        void* pEnum = readauto_pShare->Enumerate("READANIM", i);
        if ( pEnum )
        {
            ReadAnimInterface* pReader = (ReadAnimInterface*)readauto_pShare->Subscribe("READANIM.chainto", pEnum);
            if ( pReader )
            {
                if ( pReader != &readauto_interface )
                {
                    int k = 100;
                    do
                    {
                        pText = pReader->Query(k);
                        ++k;
                        if ( pText && *pText )
                        {
                            --code;
                        }
                    } while ( pText && *pText && code >= 100 );
                }

                readauto_pShare->Unsubscribe(pReader);
            }
        }
    }

    if ( code < 100 )
    {
        return pText;
    }

    return "";
}

int J3DAPI readauto_Read(int a, int b, void* pFile)
{
    int rc = readauto_Open(&pFile, pFile);
    if ( rc )
    {
        return rc;
    }

    rc = readauto_Op4(pFile, a, b);
    readauto_Close(&pFile);
    return rc;
}

int J3DAPI readauto_Open(void** ppHandle, const char* pFile)
{
    ReadAutoHandle* pHandle = readauto_pAlloc->Alloc(sizeof(ReadAutoHandle), INSANE_FILE, __LINE__);
    int rc = pHandle ? 0 : -1;
    if ( rc )
    {
        return rc;
    }

    int count = readauto_pShare->Count("READANIM");
    for ( int pass = 0; pass < 2; ++pass )
    {
        for ( int i = 0; i < count; ++i )
        {
            ReadAnimInterface* pReader = (ReadAnimInterface*)readauto_pShare->Enumerate("READANIM", i);
            if ( pReader )
            {
                pHandle->pReader = readauto_pShare->Subscribe("READANIM.chainto", pReader);
                if ( pHandle->pReader )
                {
                    if ( pHandle->pReader != &readauto_interface && pHandle->pReader->Recognize(pFile, pass) )
                    {
                        rc = pHandle->pReader->Open(&pHandle->hReader, pFile);
                        if ( rc )
                        {
                            readauto_pShare->Unsubscribe(pHandle->pReader);
                            if ( pHandle )
                            {
                                readauto_pAlloc->Free(pHandle, INSANE_FILE, __LINE__);
                            }

                            return rc;
                        }

                        *ppHandle = pHandle;
                        return 0;
                    }

                    readauto_pShare->Unsubscribe(pHandle->pReader);
                }
            }
        }
    }

    readauto_pShare->ReportError("ReadAuto Module: Cannot Recognize File Type");
    if ( pHandle )
    {
        readauto_pAlloc->Free(pHandle, INSANE_FILE, __LINE__);
    }

    return -1;
}

void J3DAPI readauto_Close(void** ppHandle)
{
    ReadAutoHandle* pHandle = *ppHandle;

    pHandle->pReader->Close(&pHandle->hReader);
    readauto_pShare->Unsubscribe(pHandle->pReader);
    if ( *ppHandle )
    {
        readauto_pAlloc->Free(*ppHandle, INSANE_FILE, __LINE__);
    }

    *ppHandle = NULL;
}

int J3DAPI readauto_Op3(void* pHandle, int a, int b, int c, int d, int e)
{
    ReadAutoHandle* pAuto = pHandle;
    return pAuto->pReader->Op3(pAuto->hReader, a, b, c, d, e);
}

int J3DAPI readauto_Op4(void* pHandle, int a, int b)
{
    ReadAutoHandle* pAuto = pHandle;
    return pAuto->pReader->Op4(pAuto->hReader, a, b);
}

int J3DAPI readauto_Recognize(const char* pFile, int pass)
{
    J3D_UNUSED(pFile);
    J3D_UNUSED(pass);
    return 0;
}
