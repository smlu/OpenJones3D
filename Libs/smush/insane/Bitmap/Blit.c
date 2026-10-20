// Blit.c: C wrappers around the MASM blitters (original 0x004F0460-0x004F0590).
#include <smush/insane/Bitmap/Blit.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/addresses.h>
#include <smush/RTI/symbols.h>
#include <smush/insane/Bitmap/BlitAsm.h>
#include <smush/insane/Decode/Decode.h>
#include <smush/insane/File/Framestream.h>

void J3DAPI blit_Fill(LecBitmapHeader* pBitmap, int color)
{
    if ( pBitmap->pitch > 0 )
    {
        if ( pBitmap->colorMode == 0 )
        {
            framestream_CopyB(pBitmap->pPixels, (uint8_t)color, pBitmap->pixelSize * pBitmap->height * pBitmap->pitch);
        }
        else
        {
            uint8_t aPixel[20];
            void* pPixels = pBitmap->pPixels;
            int size      = pBitmap->height * pBitmap->width;

            decode_Begin(aPixel, pBitmap, 1, NULL, (uint8_t*)&color);
            decode_Run(pPixels, pBitmap, size, aPixel);
        }
    }
}

void J3DAPI blit_Wrapper(void* pDst, void* pSrc, int a3, int a4, int a5, int a6, int width, int height, int dstPitch, int srcPitch)
{
    blit_Masm(pSrc, pDst, a5, a6, a3, a4, width, height, srcPitch, dstPitch);
}

void Blit_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(blit_Fill);
    J3D_HOOKFUNC(blit_Wrapper);
}

void Blit_ResetGlobals(void)
{}
