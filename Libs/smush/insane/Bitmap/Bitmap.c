// Bitmap.c: the bitmap service (`BITMAP=insane`) (original 0x004EA0F0-0x004EA7D0).
#include <smush/insane/Bitmap/Bitmap.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>
#include <smush/insane/Memory/Memory.h>
#include <smush/insane/Lec/LecShare.h>
#include <stddef.h>
#include <string.h>

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
        case 1:
            nColors = 0x100;
            // fall through
        case 0:
            pixelSize = 1;
            break;

        case 2:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
            pixelSize = 2;
            break;

        case 4:
        case 9:
            pixelSize = 3;
            break;

        case 6:
        case 11:
        case 12:
            pixelSize = 4;
            break;

        case -1:
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
    char* pBlock = bitmap_pMemory->Alloc(pitch * height + nColors * 4 + 0x5C, INSANE_FILE, __LINE__);
    if ( pBlock == NULL )
    {
        return -101;
    }

    *ppBitmap = (LecBitmapHeader*)(pBlock + 8);
    void* pPixels = pBlock + 0x5C;

    (*ppBitmap)->pPixels   = pPixels;
    (*ppBitmap)->width     = width;
    (*ppBitmap)->pitch     = pitch;
    (*ppBitmap)->height    = height;
    (*ppBitmap)->pixelSize = pixelSize;
    (*ppBitmap)->unknown7  = 0;
    (*ppBitmap)->unknown6  = 0;

    switch ( type )
    {
        case 1:
            (*ppBitmap)->bpp              = 8;
            (*ppBitmap)->colorMode        = 0;
            (*ppBitmap)->indexed.nColors  = nColors;
            (*ppBitmap)->indexed.pPalette = pPixels;
            break;

        case 0:
            (*ppBitmap)->bpp              = 8;
            (*ppBitmap)->colorMode        = 0;
            (*ppBitmap)->indexed.nColors  = nColors;
            (*ppBitmap)->indexed.pPalette = NULL;
            break;

        case 5:
        case 10:
            (*ppBitmap)->bpp                       = 16;
            (*ppBitmap)->colorMode                 = 2;
            (*ppBitmap)->format.redBPP             = 5;
            (*ppBitmap)->format.greenBPP           = 5;
            (*ppBitmap)->format.blueBPP            = 5;
            (*ppBitmap)->format.alphaBPP           = 1;
            (*ppBitmap)->format.redPosShift        = (type == 5) ? 10 : 0;
            (*ppBitmap)->format.greenPosShift      = 5;
            (*ppBitmap)->format.bluePosShift       = (type != 5) ? 10 : 0;
            (*ppBitmap)->format.alphaPosShift      = 15;
            (*ppBitmap)->format.redPosShiftRight   = 3;
            (*ppBitmap)->format.greenPosShiftRight = 3;
            (*ppBitmap)->format.bluePosShiftRight  = 3;
            (*ppBitmap)->format.alphaPosShiftRight = 7;
            break;

        case 2:
        case 7:
            (*ppBitmap)->bpp                       = 15;
            (*ppBitmap)->colorMode                 = 1;
            (*ppBitmap)->format.redBPP             = 5;
            (*ppBitmap)->format.greenBPP           = 5;
            (*ppBitmap)->format.blueBPP            = 5;
            (*ppBitmap)->format.redPosShift        = (type == 2) ? 10 : 0;
            (*ppBitmap)->format.greenPosShift      = 5;
            (*ppBitmap)->format.bluePosShift       = (type != 2) ? 10 : 0;
            (*ppBitmap)->format.redPosShiftRight   = 3;
            (*ppBitmap)->format.greenPosShiftRight = 3;
            (*ppBitmap)->format.bluePosShiftRight  = 3;
            break;

        case 3:
        case 8:
            (*ppBitmap)->bpp                       = 16;
            (*ppBitmap)->colorMode                 = 1;
            (*ppBitmap)->format.redBPP             = 5;
            (*ppBitmap)->format.greenBPP           = 6;
            (*ppBitmap)->format.blueBPP            = 5;
            (*ppBitmap)->format.redPosShift        = (type == 3) ? 11 : 0;
            (*ppBitmap)->format.greenPosShift      = 5;
            (*ppBitmap)->format.bluePosShift       = (type != 3) ? 11 : 0;
            (*ppBitmap)->format.redPosShiftRight   = 3;
            (*ppBitmap)->format.greenPosShiftRight = 2;
            (*ppBitmap)->format.bluePosShiftRight  = 3;
            break;

        case 6:
        case 11:
        case 12:
            (*ppBitmap)->bpp                       = 32;
            (*ppBitmap)->colorMode                 = 2;
            (*ppBitmap)->format.redBPP             = 8;
            (*ppBitmap)->format.greenBPP           = 8;
            (*ppBitmap)->format.blueBPP            = 8;
            (*ppBitmap)->format.alphaBPP           = 8;
            (*ppBitmap)->format.redPosShift        = (type == 6) ? 16 : ((type != 11) ? 24 : 0);
            (*ppBitmap)->format.greenPosShift      = (type != 12) ? 8 : 16;
            (*ppBitmap)->format.bluePosShift       = (type != 6) ? 8 : 0;
            (*ppBitmap)->format.alphaPosShift      = (type != 12) ? 24 : 0;
            (*ppBitmap)->format.redPosShiftRight   = 0;
            (*ppBitmap)->format.greenPosShiftRight = 0;
            (*ppBitmap)->format.bluePosShiftRight  = 0;
            (*ppBitmap)->format.alphaPosShiftRight = 0;
            break;

        case 4:
        case 9:
            (*ppBitmap)->bpp                       = 24;
            (*ppBitmap)->colorMode                 = 1;
            (*ppBitmap)->format.redBPP             = 8;
            (*ppBitmap)->format.greenBPP           = 8;
            (*ppBitmap)->format.blueBPP            = 8;
            (*ppBitmap)->format.redPosShift        = (type == 4) ? 16 : 0;
            (*ppBitmap)->format.greenPosShift      = 8;
            (*ppBitmap)->format.bluePosShift       = (type != 4) ? 16 : 0;
            (*ppBitmap)->format.redPosShiftRight   = 0;
            (*ppBitmap)->format.greenPosShiftRight = 0;
            (*ppBitmap)->format.bluePosShiftRight  = 0;
            break;

        case -1:
            (*ppBitmap)->bpp       = bitmap_pMode->GetFormat()->bpp;
            (*ppBitmap)->colorMode = bitmap_pMode->GetFormat()->colorMode;
            (*ppBitmap)->indexed   = bitmap_pMode->GetFormat()->indexed;
            (*ppBitmap)->format    = bitmap_pMode->GetFormat()->format;
            break;
    }

    return 0;
}

int J3DAPI bitmap_CreateEx(LecBitmapHeader** ppBitmap, int width, int height, int colorMode, int a5, int a6, const void* pFormat)
{
    int nColors = 0;
    if ( colorMode == 0 )
    {
        nColors = ((const int*)pFormat)[1];
    }

    char* pBlock = bitmap_pMemory->Alloc(width * height * a5 + nColors * 4 + 0x5C, INSANE_FILE, __LINE__);
    if ( pBlock == NULL )
    {
        return -101;
    }

    *ppBitmap = (LecBitmapHeader*)(pBlock + 8);
    void* pPixels = pBlock + 0x5C;

    (*ppBitmap)->pPixels   = pPixels;
    (*ppBitmap)->width     = width;
    (*ppBitmap)->pitch     = width * a5;
    (*ppBitmap)->height    = height;
    (*ppBitmap)->pixelSize = a5;
    (*ppBitmap)->bpp       = a6;
    (*ppBitmap)->unknown7  = 0;
    (*ppBitmap)->unknown6  = 0;
    (*ppBitmap)->colorMode = colorMode;

    switch ( colorMode )
    {
        case 0:
            (*ppBitmap)->indexed.nColors  = nColors;
            (*ppBitmap)->indexed.pPalette = pPixels;
            break;

        case 1:
        case 3:
        case 5:
            memcpy(&(*ppBitmap)->format, pFormat, 9 * sizeof(int));
            break;

        case 2:
        case 4:
        case 6:
            memcpy(&(*ppBitmap)->format, pFormat, sizeof(BitmapFormat));
            break;

        case 7:
            memcpy(&(*ppBitmap)->format, pFormat, 3 * sizeof(int));
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
