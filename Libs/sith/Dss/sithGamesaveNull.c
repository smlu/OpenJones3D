// Added: Null version of Libs/sith/Dss/sithGamesaveWin32.c (the GDI thumbnail code).
// Savegames are written without a thumbnail: SetThumbnailImage clones the frame the same
// way, CreateThumbnail has no GDI bitmap object to build so it fails like the Windows
// error path (which clears the cached image), WriteThumbnail returns the same 0 "nothing
// to write" the Windows version answers when no thumbnail is present, and the release
// helper clears the state without an object to delete.
#include <j3dcore/j3d.h>

#include <sith/Dss/sithGamesave.h>
#include <sith/Dss/sithGamesaveSys.h>

#include <std/Win95/stdDisplay.h>

#include <std/General/stdUtil.h>

// The thumbnail state lives in sithGamesave.c; only tSysBitmap stays NULL on this platform.
extern tVBuffer* sithGamesave_pThumbnailImage;
extern int sithGamesave_bThumbnail;
extern tSysBitmap sithGamesave_hBmpThumbnail;

void J3DAPI sithGamesave_SetThumbnailImage(tVBuffer* pVBuffer)
{
    if ( !pVBuffer )
    {
        return;
    }

    if ( sithGamesave_pThumbnailImage )
    {
        stdDisplay_VBufferFree(sithGamesave_pThumbnailImage);
        sithGamesave_pThumbnailImage = NULL;
    }

    // Added: Same clone of the given frame into a private tVBuffer as the Windows code;
    // only the HBITMAP stage of CreateThumbnail is missing on this platform.
    tRasterInfo rasterInfo = pVBuffer->rasterInfo;
    sithGamesave_pThumbnailImage = stdDisplay_VBufferNew(&rasterInfo, /*bUseVSurface=*/0, /*bUseVideoMemory=*/0);
    if ( sithGamesave_pThumbnailImage )
    {
        size_t pixelSize = sithGamesave_pThumbnailImage->rasterInfo.rowSize / (unsigned int)sithGamesave_pThumbnailImage->rasterInfo.rowWidth;
        stdDisplay_VBufferLock(pVBuffer);
        for ( size_t i = 0; i < rasterInfo.height; ++i )
        {
            memcpy(
                &sithGamesave_pThumbnailImage->pPixels[sithGamesave_pThumbnailImage->rasterInfo.rowSize * i],
                &pVBuffer->pPixels[pVBuffer->rasterInfo.rowSize * i],
                pixelSize * pVBuffer->rasterInfo.width
            );
        }

        stdDisplay_VBufferUnlock(pVBuffer);
    }
}

tSysBitmap sithGamesave_CreateThumbnail(void)
{
    // Added: On Windows the cached image is converted and resized into a DIB section; a
    // Null savegame simply carries no thumbnail, matching the Windows error path that
    // also drops the cached pixel image and clears bThumbnail.
    if ( sithGamesave_pThumbnailImage )
    {
        stdDisplay_VBufferFree(sithGamesave_pThumbnailImage);
        sithGamesave_pThumbnailImage = NULL;
    }

    sithGamesave_hBmpThumbnail = NULL;
    sithGamesave_bThumbnail = 0;
    return NULL;
}

int J3DAPI sithGamesave_WriteThumbnail(tFileHandle fh)
{
    J3D_UNUSED(fh);

    if ( !sithGamesave_bThumbnail )
    {
        // Same "no thumbnail, nothing written" answer as the Windows version.
        return 0;
    }

    // Added: Not reachable on the Null platform (CreateThumbnail always clears
    // bThumbnail); keep the Windows failure answer for safety.
    return 1;
}

void sithGamesaveSys_ReleaseThumbnail(void)
{
    if ( sithGamesave_hBmpThumbnail )
    {
        sithGamesave_hBmpThumbnail = NULL;
        sithGamesave_bThumbnail = 0;
    }
}
