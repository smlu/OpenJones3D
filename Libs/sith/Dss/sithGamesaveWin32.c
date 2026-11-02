#include <std/Win95/stdSysTagsWin32.h>
#include <sith/Dss/sithGamesave.h>
#include <sith/Dss/sithGamesaveSys.h>

#include <sith/Devices/sithComm.h>

#include <std/General/stdConffile.h>
#include <std/Win95/stdDisplay.h>

// The Windows thumbnail code of the game saves, moved out of sith/Dss/sithGamesave.c.

int J3DAPI sithGamesave_WriteThumbnail(tFileHandle fh);

void J3DAPI sithGamesave_SetThumbnailImage(tVBuffer* pVBuffer)
{
    if ( pVBuffer )
    {
        if ( sithGamesave_pThumbnailImage )
        {
            stdDisplay_VBufferFree(sithGamesave_pThumbnailImage);
            sithGamesave_pThumbnailImage = NULL;
        }

        if ( sithGamesave_hBmpThumbnail )
        {
            DeleteObject(sithGamesave_hBmpThumbnail);
            sithGamesave_hBmpThumbnail = NULL;
        }

        // Clone the thumbnail image to sithGamesave_pThumbnailImage
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
}

HBITMAP sithGamesave_CreateThumbnail(void)
{
    HDC hdc       = NULL;
    HDC hdcThumb  = NULL;
    HBITMAP hBmp  = NULL;
    void* pPixmap = NULL;

    if ( sithGamesave_hBmpThumbnail )
    {
        return sithGamesave_hBmpThumbnail;
    }

    if ( sithGamesave_pThumbnailImage )
    {
        sithGamesave_pThumbnailImage = stdDisplay_VBufferConvertColorFormat(&SITHSAVEGAME_THUMBCOLORFORMAT, sithGamesave_pThumbnailImage, 0, NULL);
        const LONG imageSize = (sithGamesave_pThumbnailImage->rasterInfo.width * sithGamesave_pThumbnailImage->rasterInfo.height * sithGamesave_pThumbnailImage->rasterInfo.colorInfo.bpp) / 8;

        BITMAPINFO bmpInfo = { 0 };
        bmpInfo.bmiHeader.biSize        = sizeof(bmpInfo.bmiHeader);
        bmpInfo.bmiHeader.biWidth       = sithGamesave_pThumbnailImage->rasterInfo.width;
        bmpInfo.bmiHeader.biHeight      = -(int32_t)sithGamesave_pThumbnailImage->rasterInfo.height;
        bmpInfo.bmiHeader.biPlanes      = 1;
        bmpInfo.bmiHeader.biBitCount    = sithGamesave_pThumbnailImage->rasterInfo.colorInfo.bpp;
        bmpInfo.bmiHeader.biCompression = BI_RGB;
        bmpInfo.bmiHeader.biSizeImage   = imageSize;

        hdc = CreateCompatibleDC(NULL);
        if ( !hdc )
        {
            goto error;
        }

        // Create bmp
        hBmp = CreateDIBSection(hdc, &bmpInfo, DIB_RGB_COLORS, &pPixmap, NULL, 0);
        if ( !hBmp )
        {
            goto error;
        }

        // Copy pixels to bmp
        memcpy(pPixmap, sithGamesave_pThumbnailImage->pPixels, bmpInfo.bmiHeader.biSizeImage);

        // Now resize bmp to thumbnail size
        bmpInfo.bmiHeader.biSize        = sizeof(bmpInfo.bmiHeader);
        bmpInfo.bmiHeader.biWidth       = SITHSAVEGAME_THUMB_WIDTH;
        bmpInfo.bmiHeader.biHeight      = -(int32_t)SITHSAVEGAME_THUMB_HEIGHT;
        bmpInfo.bmiHeader.biPlanes      = 1;
        bmpInfo.bmiHeader.biBitCount    = SITHSAVEGAME_THUMBCOLORFORMAT.bpp;
        bmpInfo.bmiHeader.biCompression = BI_RGB;
        bmpInfo.bmiHeader.biSizeImage   = SITHSAVEGAME_THUMBSIZE;
        memset(&bmpInfo.bmiHeader.biXPelsPerMeter, 0, 16);

        hdcThumb = CreateCompatibleDC(NULL);
        if ( !hdcThumb )
        {
            goto error;
        }
        sithGamesave_hBmpThumbnail = CreateDIBSection(hdcThumb, &bmpInfo, 0, &pPixmap, NULL, 0);
        if ( sithGamesave_hBmpThumbnail )
        {
            // Resize bmp
            SelectObject(hdc, hBmp);
            SelectObject(hdcThumb, sithGamesave_hBmpThumbnail);
            SetStretchBltMode(hdcThumb, STRETCH_HALFTONE);

            StretchBlt(
                hdcThumb,
                0,
                0,
                SITHSAVEGAME_THUMB_WIDTH,
                SITHSAVEGAME_THUMB_HEIGHT,
                hdc,
                0,
                0,
                sithGamesave_pThumbnailImage->rasterInfo.width,
                sithGamesave_pThumbnailImage->rasterInfo.height,
                SRCCOPY
            );

            stdDisplay_VBufferFree(sithGamesave_pThumbnailImage);
            sithGamesave_pThumbnailImage = NULL;

            DeleteDC(hdc);
            DeleteObject(hBmp);
            DeleteDC(hdcThumb);

            sithGamesave_bThumbnail = 1;
            return sithGamesave_hBmpThumbnail;
        }
    }

error:
    if ( sithGamesave_pThumbnailImage )
    {
        stdDisplay_VBufferFree(sithGamesave_pThumbnailImage);
        sithGamesave_pThumbnailImage = NULL;
    }

    if ( hBmp )
    {
        DeleteObject(hBmp);
    }

    if ( hdc )
    {
        DeleteDC(hdc);
    }

    if ( hdcThumb )
    {
        DeleteDC(hdcThumb);
    }

    sithGamesave_bThumbnail = 0;
    return 0;
}

int J3DAPI sithGamesave_WriteThumbnail(tFileHandle fh)
{
    if ( !sithGamesave_bThumbnail )
    {
        return 0;
    }

    BITMAP bmp = { 0 }; // Fixed: Initialized to 0
    if ( !GetObject(sithGamesave_hBmpThumbnail, sizeof(BITMAP), &bmp) )
    {
        return 1;
    }

    const size_t imgSize = bmp.bmWidthBytes * bmp.bmHeight; // Altered: Changed to use bmp.bmWidthBytes instead of calculating (bmp.bmBitsPixel/8 * bmp.bmWidth)
    if ( imgSize != sith_g_pHS->pFileWrite(fh, bmp.bmBits, imgSize) )
    {
        return 1;
    }

    // Success
    sithGamesave_bThumbnail = 0;
    return 0;
}

HBITMAP J3DAPI sithGamesave_LoadThumbnail(const char* pFilename)
{
    HDC hdc = NULL;
    if ( !stdConffile_OpenMode(pFilename, "rb") )
    {
        return 0;
    }

    // Read nds file header
    NdsHeader header;
    if ( !stdConffile_Read(&header, sizeof(NdsHeader)) )
    {
        goto error;
    }

    if ( header.version != SITHSAVEGAME_FILEVERSION )
    {
        goto error;
    }

    if ( !header.bThumbnail )
    {
        goto error;
    }

    // Create bitmap info & DC
    BITMAPINFO bmi              = { 0 }; // Fixed: Initialized to 0
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = SITHSAVEGAME_THUMB_WIDTH;
    bmi.bmiHeader.biHeight      = -SITHSAVEGAME_THUMB_HEIGHT;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = SITHSAVEGAME_THUMBCOLORFORMAT.bpp;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage   = SITHSAVEGAME_THUMBSIZE;
    //memset(&bmi.bmiHeader.biXPelsPerMeter, 0, 16);

    hdc = CreateCompatibleDC(NULL);
    if ( !hdc )
    {
        goto error;
    }

    // Create bitmap and read thumb pixels
    void* pPixels;
    HBITMAP hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pPixels, NULL, 0);
    if ( hBitmap && stdConffile_Read(pPixels, SITHSAVEGAME_THUMBSIZE) )
    {
        DeleteDC(hdc);
        stdConffile_Close();
        return hBitmap;
    }

error:
    if ( hdc )
    {
        DeleteDC(hdc);
    }

    stdConffile_Close();
    return NULL;
}

void sithGamesaveSys_ReleaseThumbnail(void)
{
    if ( sithGamesave_hBmpThumbnail )
    {
        DeleteObject((HGDIOBJ)sithGamesave_hBmpThumbnail);
        sithGamesave_hBmpThumbnail = NULL;
        sithGamesave_bThumbnail = 0;
    }
}
