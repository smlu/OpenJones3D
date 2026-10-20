// Zfilio.c: the `FILIO=insane_zfilio` module (gz-compressed files through the executable's zlib) (original 0x004E9B30-0x004EA020).
#include <smush/insane/File/Zfilio/Zfilio.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <smush/insane/Memory/Memory.h>
#include <std/General/stdUtil.h>

static int zfilio_extCount = 1;
static const char* zfilio_extensions[ZFILIO_MAXEXTENSIONS] = { ".gz" };
static int zfilio_flags = 1;
static LecModule zfilio_module = { zfilio_Register, zfilio_Unregister, zfilio_Startup, zfilio_Shutdown };
static ZfilioInterface zfilio_interface = { zfilio_open, zfilio_close, zfilio_read, zfilio_write, zfilio_tell, zfilio_seek, zfilio_flush };

#ifdef J3D_LOCAL_RTI
FilioInterface* zfilio_pFilio = NULL;
#endif
static MemoryDebugInterface* zfilio_pMemory;
static LecShare* zfilio_pServices;
static LecDebug* zfilio_pDebug;
static void* zfilio_pParam;

#if defined(J3D_TEST)
// Added: Test-only replacement of the executable's zlib gz functions (see Zfilio.h).
static ZfilioTestGz zfilio_test_gz;

void Zfilio_TestSetGz(const ZfilioTestGz* pGz)
{
    if ( pGz ) { zfilio_test_gz = *pGz; }
    else { memset(&zfilio_test_gz, 0, sizeof(zfilio_test_gz)); }
}
#endif

// The executable's zlib is reached through trampolines (the tests replace it)
static gzFile Zfilio_gzopen(const char* pPath, const char* pMode)
{
#if defined(J3D_TEST)
    if ( zfilio_test_gz.gzopen ) { return zfilio_test_gz.gzopen(pPath, pMode); }
#endif
    return J3D_TRAMPOLINE_CALL(gzopen, pPath, pMode);
}

static int Zfilio_gzclose(gzFile file)
{
#if defined(J3D_TEST)
    if ( zfilio_test_gz.gzclose ) { return zfilio_test_gz.gzclose(file); }
#endif
    return J3D_TRAMPOLINE_CALL(gzclose, file);
}

static int Zfilio_gzread(gzFile file, void* pBuf, unsigned int len)
{
#if defined(J3D_TEST)
    if ( zfilio_test_gz.gzread ) { return zfilio_test_gz.gzread(file, pBuf, len); }
#endif
    return J3D_TRAMPOLINE_CALL(gzread, file, pBuf, len);
}

static int Zfilio_gzwrite(gzFile file, const void* pBuf, unsigned int len)
{
#if defined(J3D_TEST)
    if ( zfilio_test_gz.gzwrite ) { return zfilio_test_gz.gzwrite(file, pBuf, len); }
#endif
    return J3D_TRAMPOLINE_CALL(gzwrite, file, pBuf, len);
}

static int Zfilio_gzflush(gzFile file, int flush)
{
#if defined(J3D_TEST)
    if ( zfilio_test_gz.gzflush ) { return zfilio_test_gz.gzflush(file, flush); }
#endif
    return J3D_TRAMPOLINE_CALL(gzflush, file, flush);
}

static long Zfilio_gzseek(gzFile file, long offset, int whence)
{
#if defined(J3D_TEST)
    if ( zfilio_test_gz.gzseek ) { return zfilio_test_gz.gzseek(file, offset, whence); }
#endif
    return J3D_TRAMPOLINE_CALL(gzseek, file, offset, whence);
}

void Zfilio_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(zfilio_Register);
    J3D_HOOKFUNC(zfilio_Configure);
    J3D_HOOKFUNC(zfilio_Unregister);
    J3D_HOOKFUNC(zfilio_Startup);
    J3D_HOOKFUNC(zfilio_Shutdown);
    J3D_HOOKFUNC(zfilio_open);
    J3D_HOOKFUNC(zfilio_close);
    J3D_HOOKFUNC(zfilio_read);
    J3D_HOOKFUNC(zfilio_write);
    J3D_HOOKFUNC(zfilio_tell);
    J3D_HOOKFUNC(zfilio_seek);
    J3D_HOOKFUNC(zfilio_flush);
}

void Zfilio_ResetGlobals(void)
{
#ifdef J3D_LOCAL_RTI
    zfilio_pFilio = NULL;
#endif
    // Otherwise zfilio_pFilio lives in the executable and Smush_ResetGlobals clears it
    zfilio_extCount = 1;
    memset((void*)zfilio_extensions, 0, sizeof(zfilio_extensions));
    zfilio_extensions[0] = ".gz";
    zfilio_flags     = 1;
    zfilio_pMemory   = NULL;
    zfilio_pServices = NULL;
    zfilio_pDebug    = NULL;
    zfilio_pParam    = NULL;
}

int J3DAPI zfilio_Register(LecShare* pShare, void* pParam)
{
    zfilio_pServices = pShare;
    zfilio_pParam    = pParam;
    pShare->Publish("LECMODULE=insane_zfilio", &zfilio_module);
    zfilio_pServices->Publish("FILIO=insane_zfilio", &zfilio_interface);
    return 0;
}

int J3DAPI zfilio_Configure(int tag, ...)
{
    va_list args;
    va_start(args, tag);
    while ( tag )
    {
        switch ( tag )
        {
            case 1:
                if ( zfilio_extCount < ZFILIO_MAXEXTENSIONS )
                {
                    zfilio_extensions[zfilio_extCount++] = va_arg(args, const char*);
                }
                else if ( zfilio_pDebug )
                {
                    zfilio_pDebug->Print("Too many zfilio extension set");
                }

                // TODO: [BUG] The extension argument is not consumed when the table is full, so the next tag read takes it
                break;

            default:
                va_end(args);
                return -1;
        }

        tag = va_arg(args, int);
    }

    va_end(args);
    return 0;
}

void zfilio_Unregister(void)
{
    zfilio_pServices->Unpublish("FILIO=insane_zfilio");
    zfilio_pServices->Unpublish("LECMODULE=insane_zfilio");
}

int zfilio_Startup(void)
{
    zfilio_pFilio  = (FilioInterface*)zfilio_pServices->Subscribe("FILIO.chainto", zfilio_pParam);
    zfilio_pMemory = (MemoryDebugInterface*)zfilio_pServices->Subscribe("MEMORY.buffers", zfilio_pParam);
    zfilio_pDebug  = (LecDebug*)zfilio_pServices->Subscribe("VIRTHERC.debug", zfilio_pParam);
    if ( !zfilio_pFilio )
    {
        zfilio_pServices->ReportError("Zfile Module: Cannot allocate filio services.");
        return -1;
    }

    if ( !zfilio_pMemory )
    {
        zfilio_pServices->ReportError("Zfile Module: Cannot allocate memory services.");
        return -1;
    }

    return 0;
}

void zfilio_Shutdown(void)
{
    zfilio_pServices->Unsubscribe(zfilio_pFilio);
    zfilio_pServices->Unsubscribe(zfilio_pMemory);
    if ( zfilio_pDebug )
    {
        zfilio_pServices->Unsubscribe(zfilio_pDebug);
    }
}

ZfileHandle* J3DAPI zfilio_open(const char* pPath, const char* pMode)
{
    ZfileHandle* pFile = zfilio_pMemory->Alloc(sizeof(ZfileHandle), INSANE_FILE, __LINE__);
    if ( !pFile )
    {
        if ( zfilio_pDebug && (zfilio_flags & 1) )
        {
            zfilio_pDebug->Print("Zfile Memory Alloc fail");
        }

        return NULL;
    }

    pFile->hFile = NULL;
    pFile->isGz  = 0;
    if ( (*pMode == 'r' || *pMode == 'R') && pMode[1] != '+' && (pMode[1] == 0 || pMode[2] != '+') )
    {
        for ( int i = 0; i < zfilio_extCount; ++i )
        {
            const char* pExt = strrchr(pPath, '.');
            if ( pExt && stdUtil_StrCmp(pExt, zfilio_extensions[i]) == 0 )
            {
                pFile->hFile = Zfilio_gzopen(pPath, pMode);
                if ( pFile->hFile )
                {
                    pFile->isGz  = 1;
                    pFile->gzPos = 0;
                    pFile->pos   = 0;

                    // The uncompressed size is the last 4 bytes of a gz file
                    // TODO: [BUG] The original leaves size uninitialized when the read returns fewer than 4 bytes
                    int size = 0; // Fixed: Init to 0
                    int fh   = zfilio_pFilio->Open(pPath, pMode);
                    zfilio_pFilio->Seek(fh, -4, 2);
                    zfilio_pFilio->Read(fh, &size, 4);
                    pFile->gzSize = size;
                    zfilio_pFilio->Close(fh);
                    return pFile;
                }

                break;
            }
        }
    }

    pFile->hFile = (gzFile)(intptr_t)zfilio_pFilio->Open(pPath, pMode);
    if ( pFile->hFile )
    {
        return pFile;
    }

    zfilio_pMemory->Free(pFile, INSANE_FILE, __LINE__);
    return NULL;
}

void J3DAPI zfilio_close(ZfileHandle* pFile)
{
    if ( pFile->hFile )
    {
        if ( pFile->isGz )
        {
            Zfilio_gzclose(pFile->hFile);
        }
        else
        {
            zfilio_pFilio->Close((int)(intptr_t)pFile->hFile);
        }
    }

    if ( pFile )
    {
        zfilio_pMemory->Free(pFile, INSANE_FILE, __LINE__);
    }
}

int J3DAPI zfilio_read(ZfileHandle* pFile, void* pBuf, int size)
{
    int rc = 0;
    if ( pFile->hFile )
    {
        if ( pFile->isGz )
        {
            if ( pFile->gzPos != pFile->pos )
            {
                Zfilio_gzseek(pFile->hFile, pFile->pos, 0);
                pFile->gzPos = pFile->pos;
            }

            rc = Zfilio_gzread(pFile->hFile, pBuf, size);
            pFile->gzPos += size;
            pFile->pos   += size;
            return rc;
        }

        rc = zfilio_pFilio->Read((int)(intptr_t)pFile->hFile, pBuf, size);
    }

    return rc;
}

int J3DAPI zfilio_write(ZfileHandle* pFile, const void* pBuf, int size)
{
    int rc = 0;
    if ( pFile->hFile )
    {
        if ( pFile->isGz )
        {
            rc = Zfilio_gzwrite(pFile->hFile, pBuf, size);
            pFile->gzPos += size;
            pFile->pos   += size;
            return rc;
        }

        rc = zfilio_pFilio->Write((int)(intptr_t)pFile->hFile, pBuf, size);
    }

    return rc;
}

int J3DAPI zfilio_tell(ZfileHandle* pFile)
{
    int pos = 0;
    if ( pFile->hFile )
    {
        if ( pFile->isGz )
        {
            return pFile->pos;
        }

        pos = zfilio_pFilio->Tell((int)(intptr_t)pFile->hFile);
    }

    return pos;
}

int J3DAPI zfilio_seek(ZfileHandle* pFile, int offset, int whence)
{
    if ( pFile->hFile )
    {
        if ( pFile->isGz )
        {
            switch ( whence )
            {
                case 0:
                    pFile->pos = offset;
                    break;

                case 1:
                    pFile->pos += offset;
                    break;

                case 2:
                    pFile->pos = pFile->gzSize + offset;
                    break;
            }
        }
        else
        {
            return zfilio_pFilio->Seek((int)(intptr_t)pFile->hFile, offset, whence);
        }
    }

    // TODO: [BUG] Falls off the end: the original returns no value here (retail leaves pFile in eax) for gz files and for a closed handle; 0 is returned
    return 0;
}

void J3DAPI zfilio_flush(ZfileHandle* pFile)
{
    if ( pFile->hFile )
    {
        if ( pFile->isGz )
        {
            Zfilio_gzflush(pFile->hFile, 4);
        }
        else
        {
            zfilio_pFilio->Flush((int)(intptr_t)pFile->hFile);
        }
    }
}
