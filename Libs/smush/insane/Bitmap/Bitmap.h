#ifndef SMUSH_INSANE_BITMAP_BITMAP_H
#define SMUSH_INSANE_BITMAP_BITMAP_H
#include <assert.h>
#include <j3dcore/j3d.h>
#include <smush/RTI/addresses.h>
#include <smush/insane/lec.h>
#include <std/types.h>

/*
 * INSANE's bitmap header and the `BITMAP=insane` service Bitmap.c publishes (`bitmap_interface`): Bitmap.c allocates the
 * 0x54-byte header with its pixels, the decoders (Decode.c, D_bloc16.c), the blitters (Blit.c, Mixer.c) and the player
 * read it. The game's SmushBitmap (SmushPlay.h) is this header: SmushPlay_PlayMovie hands its blit callback a
 * LecBitmapHeader.
 */

J3D_EXTERN_C_START

/* The pixel layouts bitmap_Create's `type` selects (names inferred from the bit layouts bitmap_Create sets up). */
typedef enum eLecBitmapType
{
    LECBITMAP_TYPE_DISPLAY   = -1, /* the display mode's format (VIDEO.getbm) */
    LECBITMAP_TYPE_INDEXED8  = 0,  /* 8 bpp, no palette attached */
    LECBITMAP_TYPE_PALETTE8  = 1,  /* 8 bpp, 256 entry palette stored before the pixels */
    LECBITMAP_TYPE_RGB555    = 2,
    LECBITMAP_TYPE_RGB565    = 3,
    LECBITMAP_TYPE_RGB888    = 4,
    LECBITMAP_TYPE_ARGB1555  = 5,
    LECBITMAP_TYPE_ARGB8888  = 6,
    LECBITMAP_TYPE_BGR555    = 7,
    LECBITMAP_TYPE_BGR565    = 8,
    LECBITMAP_TYPE_BGR888    = 9,
    LECBITMAP_TYPE_ABGR1555  = 10,
    LECBITMAP_TYPE_ABGR8888  = 11, /* blue's shift is 8, not 16 (see bitmap_Create) */
    LECBITMAP_TYPE_RGBA8888  = 12,
} LecBitmapType;

typedef struct sBitmapFormat
{
    int redBPP;
    int greenBPP;
    int blueBPP;
    int redPosShift;
    int greenPosShift;
    int bluePosShift;
    int redPosShiftRight;
    int greenPosShiftRight;
    int bluePosShiftRight;
    int alphaBPP;
    int alphaPosShift;
    int alphaPosShiftRight;
} BitmapFormat;
static_assert(sizeof(BitmapFormat) == 0x30, "sizeof(BitmapFormat) == 0x30");

typedef struct sLecBitmapHeader
{
    void* pPixels;          /* 0x00 */
    int width;              /* 0x04 */
    int height;             /* 0x08 */
    int pixelSize;          /* 0x0C */
    int bpp;                /* 0x10 */
    int pitch;              /* 0x14 */
    int unknown6;           /* 0x18 */
    int unknown7;           /* 0x1C */
    int colorMode;          /* 0x20 */
    union
    {
        BitmapFormat format;    /* 0x24 */
        struct
        {
            void* pPalette;
            int nColors;
        } indexed;
    };
} LecBitmapHeader;
static_assert(sizeof(LecBitmapHeader) == 0x54, "sizeof(LecBitmapHeader) == 0x54");

typedef struct sBitmapInterface
{
    int  (*Create)(LecBitmapHeader** ppBitmap, int width, int height, int type);
    int  (*CreateEx)(LecBitmapHeader** ppBitmap, int width, int height, int colorMode, int a5, int a6, const void* pFormat);
    void (*Destroy)(LecBitmapHeader** ppBitmap);
} BitmapInterface;
static_assert(sizeof(BitmapInterface) == 0xC, "sizeof(BitmapInterface) == 0xC");

/* The display-mode service bitmap_Startup subscribes to (slot roles inferred). */
typedef struct sBitmapModeService
{
    void* pfn00;
    LecBitmapHeader* (*GetFormat)(void);
} BitmapModeService;
static_assert(sizeof(BitmapModeService) == 8, "sizeof(BitmapModeService) == 8");

int J3DAPI bitmap_Register(LecShare* pShare, void* pParam);
void bitmap_Unregister(void);
int bitmap_Startup(void);
void bitmap_Shutdown(void);
int J3DAPI bitmap_Create(LecBitmapHeader** ppBitmap, int width, int height, int type);
int J3DAPI bitmap_CreateEx(LecBitmapHeader** ppBitmap, int width, int height, int colorMode, int a5, int a6, const void* pFormat);
void J3DAPI bitmap_Destroy(LecBitmapHeader** ppBitmap);

void Bitmap_InstallHooks(void);
void Bitmap_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_BITMAP_BITMAP_H
