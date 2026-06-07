#include "std.h"
#include "stdBmp.h"
#include "stdColor.h"
#include "stdMemory.h"
#include "stdUtil.h"

#include <j3dcore/j3dhook.h>

#include <std/RTI/symbols.h>
#include <std/Win95/stdDisplay.h>

#include <stdint.h>

static const uint16_t BMP_TYPE = 0x4D42;

void stdBmp_InstallHooks(void)
{
    J3D_HOOKFUNC(stdBmp_WriteVBuffer);
    J3D_HOOKFUNC(stdBmp_Load);
}

void stdBmp_ResetGlobals(void)
{}

int J3DAPI stdBmp_WriteVBuffer(const char* pFilename, tVBuffer* pVBuffer)
{
    STD_ASSERTREL(pFilename != NULL);
    STD_ASSERTREL(pVBuffer != NULL);

    // Added: Keep release builds from dereferencing invalid screenshot inputs.
    STD_GUARD(pFilename && pVBuffer, 1);

    if ( pVBuffer->rasterInfo.colorInfo.colorMode == STDCOLOR_PAL )
    {
        return 1;
    }

    ColorInfo ciBmp = stdColor_cfRGB888; // Note, changed encoding format due to little-endian fix in stdColor_ColorConvertOneRow

    size_t width  = pVBuffer->rasterInfo.width;
    size_t height = pVBuffer->rasterInfo.height;

    // Fixed: Reject invalid BMP dimensions before size calculations.
    if ( !width || !height || width > INT32_MAX || height > INT32_MAX )
    {
        return 1;
    }

    size_t bytesPerPixel = ciBmp.bpp / 8u;
    if ( !bytesPerPixel || ciBmp.bpp % 8u || width > (SIZE_MAX - 3u) / bytesPerPixel )
    {
        return 1;
    }

    // Fixed: Use ciBmp.bpp for the output row size and include BMP DWORD row padding.
    size_t rowBytes = width * bytesPerPixel;
    size_t stride = (rowBytes + 3u) & ~3u;
    if ( height > SIZE_MAX / stride )
    {
        return 1;
    }

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    size_t imageSize = stride * height;
    size_t headerSize = sizeof(fileHeader) + sizeof(infoHeader);

    // Fixed: Keep BMP file/image sizes representable in the 32-bit header fields.
    if ( imageSize > UINT32_MAX || headerSize > UINT32_MAX - imageSize )
    {
        return 1;
    }

    fileHeader.bfType      = BMP_TYPE;
    fileHeader.bfOffBits   = sizeof(fileHeader) + sizeof(infoHeader);
    fileHeader.bfSize      = fileHeader.bfOffBits + (DWORD)imageSize;
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;

    STD_ZEROMEM(&infoHeader, sizeof(infoHeader));
    infoHeader.biSize          = sizeof(infoHeader);
    infoHeader.biWidth         = pVBuffer->rasterInfo.width;
    infoHeader.biHeight        = pVBuffer->rasterInfo.height;
    infoHeader.biPlanes        = 1;
    infoHeader.biBitCount      = (WORD)ciBmp.bpp;
    infoHeader.biXPelsPerMeter = 2834;
    infoHeader.biYPelsPerMeter = 2834;
    infoHeader.biCompression   = BI_RGB;
    infoHeader.biSizeImage     = (DWORD)imageSize;

    tFileHandle fh = std_g_pHS->pFileOpen(pFilename, "wb");
    if ( !fh )
    {
        STDLOG_ERROR("Unable to open file '%s' for writing.\n", pFilename);

        return 1;
    }

    if ( std_g_pHS->pFileWrite(fh, &fileHeader, sizeof(fileHeader)) != sizeof(fileHeader) )
    {
        STDLOG_ERROR("Error attempting to write %zu bytes to file '%s'.\n", sizeof(fileHeader), pFilename);
        std_g_pHS->pFileClose(fh);
        return 1;
    }

    if ( std_g_pHS->pFileWrite(fh, &infoHeader, sizeof(infoHeader)) != sizeof(infoHeader) )
    {
        STDLOG_ERROR("Error attempting to write %zu bytes to file '%s'.\n", sizeof(infoHeader), pFilename);

        std_g_pHS->pFileClose(fh);
        return 1;
    }

    uint8_t* pRow = (uint8_t*)STDMALLOC(stride);
    if ( !pRow )
    {
        std_g_pHS->pFileClose(fh);
        return 1;
    }

    STD_ZEROMEM(pRow, stride);

    int res = 0;
    // Fixed: Avoid reading pixels from an unlocked VBuffer.
    if ( !stdDisplay_VBufferLock(pVBuffer) )
    {
        STDFREE(pRow);
        std_g_pHS->pFileClose(fh);
        return 1;
    }

    for ( int rowIdx = (int)pVBuffer->rasterInfo.height - 1; rowIdx >= 0; rowIdx-- )
    {
        stdColor_ColorConvertOneRow(
            pRow,
            &ciBmp,
            &pVBuffer->pPixels[pVBuffer->rasterInfo.rowSize * rowIdx],
            &pVBuffer->rasterInfo.colorInfo,
            pVBuffer->rasterInfo.width,
            0,
            0
        );

        size_t nWritten = std_g_pHS->pFileWrite(fh, pRow, stride);
        if ( nWritten != stride )
        {
            STDLOG_ERROR("Error attempting to write %zu bytes to file '%s'.\n", stride, pFilename);
            res = 1;
            break;
        }
    }

    STDFREE(pRow);
    stdDisplay_VBufferUnlock(pVBuffer);
    std_g_pHS->pFileClose(fh);
    return res;
}

HBITMAP J3DAPI stdBmp_Load(const char* pFilename)
{
    if ( !pFilename )
    {
        return NULL;
    }

    tFileHandle fh = std_g_pHS->pFileOpen(pFilename, "rb");
    if ( !fh )
    {
        STDLOG_ERROR("Error: Invalid filename '%s'.\n", pFilename);
        return NULL;
    }

    BITMAPFILEHEADER bmpHeader; static_assert(sizeof(bmpHeader) == 14, "BITMAPFILEHEADER size mismatch");
    size_t nRead = std_g_pHS->pFileRead(fh, &bmpHeader, sizeof(bmpHeader));

    BITMAPINFOHEADER infoHeader; static_assert(sizeof(infoHeader) == 40, "BITMAPINFOHEADER size mismatch");
    nRead += std_g_pHS->pFileRead(fh, &infoHeader, sizeof(infoHeader));

    if ( nRead != (sizeof(bmpHeader) + sizeof(infoHeader)) )
    {
        STDLOG_ERROR("Error: Cannot read File's header.\n");
        std_g_pHS->pFileClose(fh);
        return NULL;
    }

    if ( bmpHeader.bfType != BMP_TYPE )
    {
        STDLOG_ERROR("Error: File's header does not contain 'BM' so its not a BMP file.\n");
        std_g_pHS->pFileClose(fh);
        return NULL;
    }

    if ( infoHeader.biSize != sizeof(BITMAPINFOHEADER) )
    {
        STDLOG_ERROR("Error: File's info header contains wrong header size.  The file may not be a BMP.\n");
        std_g_pHS->pFileClose(fh);
        return NULL;
    }

    HBITMAP hbmp = NULL;
    switch ( infoHeader.biBitCount )
    {
        case 24u:
        {
            // Fixed: Only accept supported uncompressed bottom-up 24 bpp BMPs.
            if ( infoHeader.biCompression != BI_RGB || infoHeader.biWidth <= 0 || infoHeader.biHeight <= 0 )
            {
                STDLOG_ERROR("Unsupported or invalid 24 bpp BMP header.\n");
                break;
            }

            // Fixed: Include BMP DWORD row padding when validating and reading pixel data.
            size_t width  = (size_t)infoHeader.biWidth;
            size_t height = (size_t)infoHeader.biHeight;
            if ( width > (SIZE_MAX - 3u) / 3u )
            {
                STDLOG_ERROR("BMP dimensions are too large.\n");
                break;
            }

            // Fixed: Reject BMPs with row sizes that would overflow size calculations or the header field.
            size_t rowBytes = width * 3u;
            size_t stride = (rowBytes + 3u) & ~3u;
            if ( height > SIZE_MAX / stride )
            {
                STDLOG_ERROR("BMP image size is too large.\n");
                break;
            }

            // Fixed: Keep the DIB image size representable in the 32-bit header field.
            size_t imageSize = stride * height;
            if ( imageSize > UINT32_MAX )
            {
                STDLOG_ERROR("BMP image size is too large.\n");
                break;
            }

            // Fixed: Reject BMPs with file sizes that would overflow size calculations or the header field.
            if ( imageSize > SIZE_MAX - (size_t)bmpHeader.bfOffBits )
            {
                STDLOG_ERROR("BMP file size is too large.\n");
                break;
            }

            // Fixed: Reject truncated BMPs before reading into the DIB section.
            size_t minFileSize = (size_t)bmpHeader.bfOffBits + imageSize;
            size_t fileSize = std_g_pHS->pFileSize(pFilename);
            if ( fileSize && fileSize < minFileSize )
            {
                STDLOG_ERROR("BMP file is truncated.\n");
                break;
            }

            BITMAPINFO bmi;
            STD_ZEROMEM(&bmi, sizeof(bmi));
            STD_COPYMEM(&bmi.bmiHeader, &infoHeader, sizeof(BITMAPINFOHEADER));
            bmi.bmiHeader.biSizeImage = (DWORD)imageSize;

            HDC hdc = CreateCompatibleDC(NULL);
            if ( hdc )
            {
                void* ppvBits;
                hbmp = CreateDIBSection(hdc, &bmi, 0, &ppvBits, NULL, 0);
                if ( hbmp )
                {
                    // Fixed: Seek to bfOffBits instead of assuming pixel data follows the headers.
                    if ( std_g_pHS->pFileSeek(fh, (int)bmpHeader.bfOffBits, SEEK_SET) )
                    {
                        DeleteObject(hbmp);
                        hbmp = NULL;
                    }
                    else
                    {
                        nRead = std_g_pHS->pFileRead(fh, ppvBits, imageSize);
                        if ( nRead != imageSize )
                        {
                            // Fixed: Delete the created bitmap. Original code leaked the bitmap on read errors.
                            DeleteObject(hbmp);
                            hbmp = NULL;
                            STDLOG_ERROR("Unable to read all the data from your BMP file. It may be corrupted.\n");
                        }
                    }
                }

                DeleteDC(hdc); // Fixed: Delete hdcThumbnail in case of an error
            }
            break;
        }

        case 8u:
            STDLOG_ERROR("8 bpp BMP not yet supported.\n");
            break;

        case 16u:
            STDLOG_ERROR("16 bpp BMP not yet supported.\n");
            break;

        case 32u:
            STDLOG_ERROR("32 bpp BMP not yet supported.\n");
            break;

        default:
            STDLOG_ERROR("This BMP file uses a bit-depth I do not support yet.\n");
            break;
    }

    std_g_pHS->pFileClose(fh);
    return hbmp;
}