// Cdstream.c: the file ring-buffer streamer (`STREAMER=insane_streamer`) (original 0x004E4D50-0x004E5470).
#include <smush/insane/File/Cdstream.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <smush/insane/File/Framestream.h>
#include <std/General/stdUtil.h>

static LecModule streamer_module = { streamer_Register, streamer_Unregister, streamer_Startup, streamer_Shutdown };
static StreamerInterface streamer_interface = {
    streamer_Create, streamer_Destroy, streamer_SetFile, streamer_IsActive, streamer_Fill, streamer_Advance, streamer_Reset, streamer_Percent
};

static MemoryDebugInterface* streamer_pAlloc;
static LecShare* streamer_pShare;
static FilioInterface* streamer_pFile;
static void* streamer_pParam;
static int streamer_wrap;

void Cdstream_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(streamer_Register);
    J3D_HOOKFUNC(streamer_Unregister);
    J3D_HOOKFUNC(streamer_Startup);
    J3D_HOOKFUNC(streamer_Shutdown);
    J3D_HOOKFUNC(streamer_Create);
    J3D_HOOKFUNC(streamer_Destroy);
    J3D_HOOKFUNC(streamer_Reset);
    J3D_HOOKFUNC(streamer_Fill);
    J3D_HOOKFUNC(streamer_SetFile);
    J3D_HOOKFUNC(streamer_Prime);
    J3D_HOOKFUNC(streamer_IsActive);
    J3D_HOOKFUNC(streamer_Advance);
    J3D_HOOKFUNC(streamer_Cancel);
    J3D_HOOKFUNC(streamer_InRange);
    J3D_HOOKFUNC(streamer_Percent);
}

void Cdstream_ResetGlobals(void)
{
    streamer_pAlloc = NULL;
    streamer_pShare = NULL;
    streamer_pFile  = NULL;
    streamer_pParam = NULL;
    streamer_wrap   = 0;
}

int J3DAPI streamer_Register(LecShare* pShare, void* pParam)
{
    streamer_pShare = pShare;
    streamer_pParam = pParam;
    pShare->Publish("LECMODULE=insane_streamer", &streamer_module);
    streamer_pShare->Publish("STREAMER=insane_streamer", &streamer_interface);
    return 0;
}

void streamer_Unregister(void)
{
    streamer_pShare->Unpublish("STREAMER=insane_streamer");
    streamer_pShare->Unpublish("LECMODULE=insane_streamer");
}

int streamer_Startup(void)
{
    streamer_pAlloc = (MemoryDebugInterface*)streamer_pShare->Subscribe("MEMORY.workspace", streamer_pParam);
    streamer_pFile  = (FilioInterface*)streamer_pShare->Subscribe("FILIO.io", streamer_pParam);
    if ( !streamer_pAlloc )
    {
        streamer_pShare->ReportError("Streamer Module: Cannot allocate memory services.");
        return -1;
    }

    if ( !streamer_pFile )
    {
        streamer_pShare->ReportError("Streamer Module: Cannot allocate filio services.");
        return -1;
    }

    return 0;
}

void streamer_Shutdown(void)
{
    streamer_pShare->Unsubscribe(streamer_pAlloc);
    streamer_pShare->Unsubscribe(streamer_pFile);
}

int J3DAPI streamer_Create(Stream** ppStream, int ringSize, int frameSize)
{
    Stream* pStream = streamer_pAlloc->Alloc(sizeof(Stream), INSANE_FILE, __LINE__);
    int rc = pStream ? 0 : -1;
    if ( rc == 0 )
    {
        pStream->pfnRetry = NULL;

        char* pBuf = streamer_pAlloc->Alloc(ringSize + frameSize, INSANE_FILE, __LINE__);
        pStream->pBuf = pBuf;
        rc = pBuf ? 0 : -1;
        if ( rc == 0 )
        {
            pStream->ringSize    = ringSize & ~(STREAMER_SECTOR_SIZE - 1);
            pStream->maxFrame    = frameSize;
            pStream->pEnd        = pBuf + pStream->ringSize;
            pStream->hFile       = 0;
            pStream->startOffset = 0;
            pStream->reopen      = 0;
            memset(pBuf, 0, pStream->ringSize);
            *ppStream = pStream;
        }
        else if ( pStream )
        {
            streamer_pAlloc->Free(pStream, INSANE_FILE, __LINE__);
        }
    }

    return rc;
}

void J3DAPI streamer_Destroy(Stream** ppStream)
{
    if ( ppStream )
    {
        Stream* pStream = *ppStream;
        if ( pStream->hFile )
        {
            streamer_pFile->Close(pStream->hFile);
            pStream->hFile = 0;
        }

        if ( pStream->pBuf )
        {
            streamer_pAlloc->Free(pStream->pBuf, INSANE_FILE, __LINE__);
        }

        pStream->pBuf = NULL;
        if ( *ppStream )
        {
            streamer_pAlloc->Free(*ppStream, INSANE_FILE, __LINE__);
        }

        *ppStream = NULL;
    }
}

void J3DAPI streamer_Reset(Stream* pStream)
{
    pStream->active  = 0;
    pStream->pRead   = pStream->pBuf;
    pStream->pWrite  = pStream->pBuf;
    pStream->ready   = 0;
    pStream->pending = 0;
    pStream->skip    = 0;
    if ( pStream->hFile )
    {
        streamer_pFile->Close(pStream->hFile);
        pStream->hFile = 0;
    }

    pStream->busy = 0;
}

void J3DAPI streamer_Fill(Stream* pStream, int limit)
{
    int bPrimed = !streamer_Prime(pStream);
    if ( pStream->hFile && pStream->filePos < pStream->fileSize )
    {
        int avail;
        char* pWrite = pStream->pWrite;
        if ( pWrite <= pStream->pRead && pWrite != pStream->pBuf )
        {
            avail = pStream->pEnd - pStream->pRead;
        }
        else if ( pWrite == pStream->pBuf && pStream->pEnd - pStream->pRead > STREAMER_SECTOR_SIZE )
        {
            avail = pStream->pEnd - pStream->pRead - STREAMER_SECTOR_SIZE;
        }
        else
        {
            avail = (pWrite - pStream->pRead - STREAMER_SECTOR_SIZE) & ~(STREAMER_SECTOR_SIZE - 1);
            if ( avail < STREAMER_SECTOR_SIZE )
            {
                return;
            }
        }

        if ( !bPrimed )
        {
            if ( avail > limit )
            {
                avail = limit;
            }
        }
        else if ( avail > pStream->maxFrame )
        {
            avail = pStream->maxFrame;
        }

        int n = (pStream->fileSize - pStream->filePos < avail) ? pStream->fileSize - pStream->filePos : avail;
        if ( streamer_pFile->Read(pStream->hFile, pStream->pRead, n) == n )
        {
            pStream->filePos += avail;
            char* pNext = pStream->pRead + avail;
            if ( pNext >= pStream->pEnd )
            {
                pNext -= pStream->ringSize;
            }

            pStream->pRead = pNext;
            if ( pStream->active )
            {
                pStream->ready = 1;
            }
        }
        else
        {
            streamer_pFile->Seek(pStream->hFile, pStream->filePos, SEEK_SET);
            if ( pStream->pfnRetry )
            {
                pStream->pfnRetry(STREAMER_RETRY_READ, NULL);
            }
        }
    }
}

int J3DAPI streamer_SetFile(Stream* pStream, const char* pName, int startOffset)
{
    if ( pName )
    {
        int hFile = streamer_pFile->Open(pName, "rb");
        if ( !hFile )
        {
            return -1;
        }

        streamer_pFile->Close(hFile);
        // Fixed: The original strcpy overflows name (0xC8 bytes) on a longer path
        STD_STRCPY(pStream->name, pName);
    }
    else
    {
        pStream->name[0] = 0;
    }

    pStream->reopen      = 1;
    pStream->startOffset = startOffset;
    return 0;
}

int J3DAPI streamer_Prime(Stream* pStream)
{
    int rc = STREAMER_PRIME_IDLE;
    if ( pStream->reopen )
    {
        if ( pStream->name[0] )
        {
            int hFile = streamer_pFile->Open(pStream->name, "rb");
            while ( !hFile )
            {
                if ( !pStream->pfnRetry || pStream->pfnRetry(STREAMER_RETRY_OPEN, pStream->name) )
                {
                    pStream->reopen = 0;
                    return STREAMER_PRIME_FAILED;
                }

                hFile = streamer_pFile->Open(pStream->name, "rb");
            }

            if ( pStream->hFile )
            {
                streamer_pFile->Close(pStream->hFile);
                pStream->hFile = 0;
            }

            pStream->hFile = hFile;
            streamer_pFile->Seek(hFile, 0, SEEK_END);
            pStream->fileSize = streamer_pFile->Tell(pStream->hFile);
        }

        streamer_pFile->Seek(pStream->hFile, pStream->startOffset & ~(STREAMER_SECTOR_SIZE - 1), SEEK_SET);
        pStream->busy++;
        pStream->filePos = pStream->startOffset & ~(STREAMER_SECTOR_SIZE - 1);
        pStream->skip    = pStream->startOffset & (STREAMER_SECTOR_SIZE - 1);
        pStream->pStart  = pStream->pRead;
        pStream->active  = 1;
        pStream->ready   = 0;
        pStream->reopen  = 0;
        pStream->busy--;
        rc = 0;
    }

    return rc;
}

int J3DAPI streamer_IsActive(Stream* pStream)
{
    if ( pStream->ready && pStream->active )
    {
        return 1;
    }

    return 0;
}

int J3DAPI streamer_Advance(Stream* pStream, int n)
{
    if ( pStream->busy || !pStream->hFile )
    {
        return STREAMER_NOTREADY;
    }

    if ( n > pStream->maxFrame )
    {
        streamer_pShare->ReportError("File Streamer:  Invalid frame size %ld", n);
        return STREAMER_BADSIZE;
    }

    if ( !streamer_wrap )
    {
        pStream->pWrite += pStream->pending;
        pStream->pending = 0;
        if ( pStream->pWrite >= pStream->pEnd )
        {
            pStream->pWrite -= pStream->ringSize;
        }

        streamer_wrap = 1;
    }

    if ( n < 0 )
    {
        if ( pStream->active )
        {
            pStream->pending = 0;
            pStream->pWrite  = pStream->pStart;
        }

        streamer_wrap = 0;
        return STREAMER_NOTREADY;
    }

    if ( pStream->active )
    {
        streamer_Cancel(pStream);
        streamer_wrap = 0;
        return 0;
    }

    if ( pStream->skip )
    {
        n += pStream->skip;
    }

    if ( !streamer_InRange(pStream, n) )
    {
        return STREAMER_NOTREADY;
    }

    streamer_wrap    = 0;
    pStream->pending = n;

    int len = pStream->pWrite - pStream->pEnd + n;
    if ( len > 0 )
    {
        framestream_Copy(pStream->pEnd, pStream->pBuf, len);
    }

    if ( !pStream->active && pStream->skip )
    {
        int offset    = pStream->skip;
        pStream->skip = 0;
        return (int)(pStream->pWrite + offset);
    }

    return (int)pStream->pWrite;
}

int J3DAPI streamer_Cancel(Stream* pStream)
{
    if ( pStream->active )
    {
        pStream->active  = 0;
        pStream->ready   = 0;
        pStream->pWrite  = pStream->pStart;
        pStream->pending = 0;
        return 0;
    }

    return -1;
}

int J3DAPI streamer_InRange(Stream* pStream, int n)
{
    if ( pStream->pWrite <= pStream->pRead )
    {
        if ( pStream->pRead >= pStream->pWrite + n )
        {
            return 1;
        }
    }
    else if ( pStream->pRead + pStream->ringSize >= pStream->pWrite + n )
    {
        return 1;
    }

    return 0;
}

int J3DAPI streamer_Percent(Stream* pStream)
{
    if ( !pStream->hFile )
    {
        return 100;
    }

    if ( pStream->filePos >= pStream->fileSize - STREAMER_SECTOR_SIZE )
    {
        return 100;
    }

    if ( pStream->pWrite <= pStream->pRead )
    {
        return (pStream->pRead - pStream->pWrite) * 100 / pStream->ringSize;
    }

    return (pStream->ringSize - (pStream->pWrite - pStream->pRead)) * 100 / pStream->ringSize;
}
