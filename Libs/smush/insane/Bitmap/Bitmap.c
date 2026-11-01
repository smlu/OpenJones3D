// Bitmap.c: the bitmap service (`BITMAP=insane`) (original 0x004EA0F0-0x004EA7D0).
#include <smush/insane/Bitmap/Bitmap.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>
#include <smush/insane/Memory/Memory.h>
#include <smush/insane/Lec/LecShare.h>
#include <stddef.h>
#include <string.h>

// An allocated bitmap is a block: 8 bytes of allocator bookkeeping, the header, then the palette (indexed bitmaps) and the pixels
#define LECBITMAP_BLOCKHEAD_SIZE 8
#define LECBITMAP_PIXELS_OFFSET  (LECBITMAP_BLOCKHEAD_SIZE + (int)sizeof(LecBitmapHeader))
#define LECBITMAP_PALETTE_SIZE   256

static LecModule bitmap_module = { bitmap_Register, bitmap_Unregister, bitmap_Startup, bitmap_Shutdown };
static BitmapInterface bitmap_interface = { bitmap_Create, bitmap_CreateEx, bitmap_Destroy };

static MemoryDebugInterface* bitmap_pMemory;
static LecShare* bitmap_pShare;
static void* bitmap_pParam;
static BitmapModeService* bitmap_pMode;

int J3DAPI bitmap_Register(LecShare* pShare, void* pParam)
{
    bitmap_pShare = pShare;
    bitmap_pParam = pParam;
    pShare->Publish("LECMODULE=insane_bitmap", &bitmap_module);
    bitmap_pShare->Publish("BITMAP=insane", &bitmap_interface);
    return 0;
}

void bitmap_Unregister(void)
{
    bitmap_pShare->Unpublish("BITMAP=insane");
    bitmap_pShare->Unpublish("LECMODULE=insane_bitmap");
}

int bitmap_Startup(void)
{
    bitmap_pMemory = bitmap_pShare->Subscribe("MEMORY.workspace", bitmap_pParam);
    bitmap_pMode   = bitmap_pShare->Subscribe("VIDEO.getbm", bitmap_pParam);
    if ( bitmap_pMemory == NULL )
    {
        bitmap_pShare->ReportError("Video Module: Cannot allocate memory services.");
        return -1;
    }

    return 0;
}

void bitmap_Shutdown(void)
{
    bitmap_pShare->Unsubscribe(bitmap_pMemory);
    if ( bitmap_pMode )
    {
        bitmap_pShare->Unsubscribe(bitmap_pMode);
    }
}

int J3DAPI bitmap_Create(LecBitmapHeader** ppBitmap, int width, int height, int type)
{
    int nColors = 0;
    int pixelSize;

    switch ( type )
    {
        case LECBITMAP_TYPE_PALETTE8:
            nColors = LECBITMAP_PALETTE_SIZE;
            // fall through
        case LECBITMAP_TYPE_INDEXED8:
            pixelSize = 1;
            break;

        case LECBITMAP_TYPE_RGB555:
        case LECBITMAP_TYPE_RGB565:
        case LECBITMAP_TYPE_ARGB1555:
        case LECBITMAP_TYPE_BGR555:
        case LECBITMAP_TYPE_BGR565:
        case LECBITMAP_TYPE_ABGR1555:
            pixelSize = 2;
            break;

        case LECBITMAP_TYPE_RGB888:
        case LECBITMAP_TYPE_BGR888:
            pixelSize = 3;
            break;

        case LECBITMAP_TYPE_ARGB8888:
        case LECBITMAP_TYPE_ABGR8888:
        case LECBITMAP_TYPE_RGBA8888:
            pixelSize = 4;
            break;

        case LECBITMAP_TYPE_DISPLAY:
            if ( bitmap_pMode != NULL )
            {
                pixelSize = bitmap_pMode->GetFormat()->pixelSize;
                break;
            }

            // TODO: [BUG] The format has "%s" for the line number, which is an int, so the log call reads an invalid string address.
            lec_Log("%s, line %s:  Type %d not supported", INSANE_FILE, __LINE__, type);
            return -1;

        default:
            // TODO: [BUG] See above, the line number is formatted as a string.
            lec_Log("%s, line %s:  Type %d not supported", INSANE_FILE, __LINE__, type);
            return -1;
    }

    int pitch    = pixelSize * width;
    char* pBlock = bitmap_pMemory->Alloc(pitch * height + nColors * (int)sizeof(uint32_t) + LECBITMAP_PIXELS_OFFSET, INSANE_FILE, __LINE__);
    if ( pBlock == NULL )
    {
        return -101;
    }

    LecBitmapHeader* pHeader = (LecBitmapHeader*)(pBlock + LECBITMAP_BLOCKHEAD_SIZE);
    *ppBitmap = pHeader;
    void* pPixels = pBlock + LECBITMAP_PIXELS_OFFSET;

    pHeader->pPixels   = pPixels;
    pHeader->width     = width;
    pHeader->pitch     = pitch;
    pHeader->height    = height;
    pHeader->pixelSize = pixelSize;
    pHeader->unknown7  = 0;
    pHeader->unknown6  = 0;

    switch ( type )
    {
        case LECBITMAP_TYPE_PALETTE8:
            pHeader->bpp              = 8;
            pHeader->colorMode        = STDCOLOR_PAL;
            pHeader->indexed.nColors  = nColors;
            pHeader->indexed.pPalette = pPixels;
            break;

        case LECBITMAP_TYPE_INDEXED8:
            pHeader->bpp              = 8;
            pHeader->colorMode        = STDCOLOR_PAL;
            pHeader->indexed.nColors  = nColors;
            pHeader->indexed.pPalette = NULL;
            break;

        case LECBITMAP_TYPE_ARGB1555:
        case LECBITMAP_TYPE_ABGR1555:
            pHeader->bpp                       = 16;
            pHeader->colorMode                 = STDCOLOR_RGBA;
            pHeader->format.redBPP             = 5;
            pHeader->format.greenBPP           = 5;
            pHeader->format.blueBPP            = 5;
            pHeader->format.alphaBPP           = 1;
            pHeader->format.redPosShift        = (type == LECBITMAP_TYPE_ARGB1555) ? 10 : 0;
            pHeader->format.greenPosShift      = 5;
            pHeader->format.bluePosShift       = (type != LECBITMAP_TYPE_ARGB1555) ? 10 : 0;
            pHeader->format.alphaPosShift      = 15;
            pHeader->format.redPosShiftRight   = 3;
            pHeader->format.greenPosShiftRight = 3;
            pHeader->format.bluePosShiftRight  = 3;
            pHeader->format.alphaPosShiftRight = 7;
            break;

        case LECBITMAP_TYPE_RGB555:
        case LECBITMAP_TYPE_BGR555:
            pHeader->bpp                       = 15;
            pHeader->colorMode                 = STDCOLOR_RGB;
            pHeader->format.redBPP             = 5;
            pHeader->format.greenBPP           = 5;
            pHeader->format.blueBPP            = 5;
            pHeader->format.redPosShift        = (type == LECBITMAP_TYPE_RGB555) ? 10 : 0;
            pHeader->format.greenPosShift      = 5;
            pHeader->format.bluePosShift       = (type != LECBITMAP_TYPE_RGB555) ? 10 : 0;
            pHeader->format.redPosShiftRight   = 3;
            pHeader->format.greenPosShiftRight = 3;
            pHeader->format.bluePosShiftRight  = 3;
            break;

        case LECBITMAP_TYPE_RGB565:
        case LECBITMAP_TYPE_BGR565:
            pHeader->bpp                       = 16;
            pHeader->colorMode                 = STDCOLOR_RGB;
            pHeader->format.redBPP             = 5;
            pHeader->format.greenBPP           = 6;
            pHeader->format.blueBPP            = 5;
            pHeader->format.redPosShift        = (type == LECBITMAP_TYPE_RGB565) ? 11 : 0;
            pHeader->format.greenPosShift      = 5;
            pHeader->format.bluePosShift       = (type != LECBITMAP_TYPE_RGB565) ? 11 : 0;
            pHeader->format.redPosShiftRight   = 3;
            pHeader->format.greenPosShiftRight = 2;
            pHeader->format.bluePosShiftRight  = 3;
            break;

        case LECBITMAP_TYPE_ARGB8888:
        case LECBITMAP_TYPE_ABGR8888:
        case LECBITMAP_TYPE_RGBA8888:
            // TODO: [BUG] For ABGR8888 the blue shift is 8 like green (it would need 16), so the channels overlap.
            pHeader->bpp                       = 32;
            pHeader->colorMode                 = STDCOLOR_RGBA;
            pHeader->format.redBPP             = 8;
            pHeader->format.greenBPP           = 8;
            pHeader->format.blueBPP            = 8;
            pHeader->format.alphaBPP           = 8;
            pHeader->format.redPosShift        = (type == LECBITMAP_TYPE_ARGB8888) ? 16 : ((type != LECBITMAP_TYPE_ABGR8888) ? 24 : 0);
            pHeader->format.greenPosShift      = (type != LECBITMAP_TYPE_RGBA8888) ? 8 : 16;
            pHeader->format.bluePosShift       = (type != LECBITMAP_TYPE_ARGB8888) ? 8 : 0;
            pHeader->format.alphaPosShift      = (type != LECBITMAP_TYPE_RGBA8888) ? 24 : 0;
            pHeader->format.redPosShiftRight   = 0;
            pHeader->format.greenPosShiftRight = 0;
            pHeader->format.bluePosShiftRight  = 0;
            pHeader->format.alphaPosShiftRight = 0;
            break;

        case LECBITMAP_TYPE_RGB888:
        case LECBITMAP_TYPE_BGR888:
            pHeader->bpp                       = 24;
            pHeader->colorMode                 = STDCOLOR_RGB;
            pHeader->format.redBPP             = 8;
            pHeader->format.greenBPP           = 8;
            pHeader->format.blueBPP            = 8;
            pHeader->format.redPosShift        = (type == LECBITMAP_TYPE_RGB888) ? 16 : 0;
            pHeader->format.greenPosShift      = 8;
            pHeader->format.bluePosShift       = (type != LECBITMAP_TYPE_RGB888) ? 16 : 0;
            pHeader->format.redPosShiftRight   = 0;
            pHeader->format.greenPosShiftRight = 0;
            pHeader->format.bluePosShiftRight  = 0;
            break;

        case LECBITMAP_TYPE_DISPLAY:
            pHeader->bpp       = bitmap_pMode->GetFormat()->bpp;
            pHeader->colorMode = bitmap_pMode->GetFormat()->colorMode;
            pHeader->indexed   = bitmap_pMode->GetFormat()->indexed;
            pHeader->format    = bitmap_pMode->GetFormat()->format;
            break;
    }

    return 0;
}

int J3DAPI bitmap_CreateEx(LecBitmapHeader** ppBitmap, int width, int height, int colorMode, int pixelSize, int bpp, const void* pFormat)
{
    int nColors = 0;
    if ( colorMode == STDCOLOR_PAL )
    {
        nColors = ((const int*)pFormat)[1];
    }

    char* pBlock = bitmap_pMemory->Alloc(width * height * pixelSize + nColors * (int)sizeof(uint32_t) + LECBITMAP_PIXELS_OFFSET, INSANE_FILE, __LINE__);
    if ( pBlock == NULL )
    {
        return -101;
    }

    LecBitmapHeader* pHeader = (LecBitmapHeader*)(pBlock + LECBITMAP_BLOCKHEAD_SIZE);
    *ppBitmap = pHeader;
    void* pPixels = pBlock + LECBITMAP_PIXELS_OFFSET;

    pHeader->pPixels   = pPixels;
    pHeader->width     = width;
    pHeader->pitch     = width * pixelSize;
    pHeader->height    = height;
    pHeader->pixelSize = pixelSize;
    pHeader->bpp       = bpp;
    pHeader->unknown7  = 0;
    pHeader->unknown6  = 0;
    pHeader->colorMode = colorMode;

    switch ( colorMode )
    {
        case STDCOLOR_PAL:
            pHeader->indexed.nColors  = nColors;
            pHeader->indexed.pPalette = pPixels;
            break;

        case 1:
        case 3:
        case 5:
            memcpy(&pHeader->format, pFormat, offsetof(BitmapFormat, alphaBPP));
            break;

        case 2:
        case 4:
        case 6:
            memcpy(&pHeader->format, pFormat, sizeof(BitmapFormat));
            break;

        case 7:
            memcpy(&pHeader->format, pFormat, offsetof(BitmapFormat, redPosShift));
            break;
    }

    return 0;
}

void J3DAPI bitmap_Destroy(LecBitmapHeader** ppBitmap)
{
    char* pBlock = (char*)*ppBitmap - 8;

    // TODO: [BUG] pBlock is never NULL, even for a NULL bitmap: the check is made on the pointer already moved to the block start.
    if ( pBlock )
    {
        bitmap_pMemory->Free(pBlock, INSANE_FILE, __LINE__);
    }

    *ppBitmap = NULL;
}

void Bitmap_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(bitmap_Register);
    J3D_HOOKFUNC(bitmap_Unregister);
    J3D_HOOKFUNC(bitmap_Startup);
    J3D_HOOKFUNC(bitmap_Shutdown);
    J3D_HOOKFUNC(bitmap_Create);
    J3D_HOOKFUNC(bitmap_CreateEx);
    J3D_HOOKFUNC(bitmap_Destroy);
}

void Bitmap_ResetGlobals(void)
{
    bitmap_pMemory = NULL;
    bitmap_pShare  = NULL;
    bitmap_pParam  = NULL;
    bitmap_pMode   = NULL;
}
