// D_bloc16.c: the blocky16 frame-object decoder (original 0x004E8510-0x004E9810).
#include <smush/insane/Flobject/D_bloc16.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

#include <stdarg.h>
#include <stddef.h>
#include <smush/insane/Bitmap/Bitmap.h>
#include <smush/insane/Decode/Decode.h>
#include <smush/insane/File/Framestream.h>
#include <smush/insane/Flobject/Bloc16Asm.h>
#include <smush/insane/Memory/Memory.h>
#include <smush/insane/Sound/Mixer.h>

#define DBLOC16_ABS(a)    ((a) < 0 ? -(a) : (a))
#define DBLOC16_MAX(a, b) ((a) > (b) ? (a) : (b))

static LecModule dbloc16_module = { dbloc16_Register, dbloc16_Unregister, dbloc16_Startup, dbloc16_Shutdown };
static Bl16Interface dbloc16_interface = { dbloc16_Name, dbloc16_GetTag, dbloc16_GetFlags, dbloc16_Reset, dbloc16_Parse, dbloc16_Decode, dbloc16_Query1, dbloc16_Query };

// Codec 2's 255 motion vectors (x, y); dbloc16_MakeTables turns them into pixel offsets for the frame width
static const Bl16Motion dbloc16_motion[255] = {
    { 0, 0 }, { -1, -43 }, { 6, -43 }, { -9, -42 }, { 13, -41 }, { -16, -40 }, { 19, -39 }, { -23, -36 },
    { 26, -34 }, { -2, -33 }, { 4, -33 }, { -29, -32 }, { -9, -32 }, { 11, -31 }, { -16, -29 }, { 32, -29 },
    { 18, -28 }, { -34, -26 }, { -22, -25 }, { -1, -25 }, { 3, -25 }, { -7, -24 }, { 8, -24 }, { 24, -23 },
    { 36, -23 }, { -12, -22 }, { 13, -21 }, { -38, -20 }, { 0, -20 }, { -27, -19 }, { -4, -19 }, { 4, -19 },
    { -17, -18 }, { -8, -17 }, { 8, -17 }, { 18, -17 }, { 28, -17 }, { 39, -17 }, { -12, -15 }, { 12, -15 },
    { -21, -14 }, { -1, -14 }, { 1, -14 }, { -41, -13 }, { -5, -13 }, { 5, -13 }, { 21, -13 }, { -31, -12 },
    { -15, -11 }, { -8, -11 }, { 8, -11 }, { 15, -11 }, { -2, -10 }, { 1, -10 }, { 31, -10 }, { -23, -9 },
    { -11, -9 }, { -5, -9 }, { 4, -9 }, { 11, -9 }, { 42, -9 }, { 6, -8 }, { 24, -8 }, { -18, -7 },
    { -7, -7 }, { -3, -7 }, { -1, -7 }, { 2, -7 }, { 18, -7 }, { -43, -6 }, { -13, -6 }, { -4, -6 },
    { 4, -6 }, { 8, -6 }, { -33, -5 }, { -9, -5 }, { -2, -5 }, { 0, -5 }, { 2, -5 }, { 5, -5 },
    { 13, -5 }, { -25, -4 }, { -6, -4 }, { -3, -4 }, { 3, -4 }, { 9, -4 }, { -19, -3 }, { -7, -3 },
    { -4, -3 }, { -2, -3 }, { -1, -3 }, { 0, -3 }, { 1, -3 }, { 2, -3 }, { 4, -3 }, { 6, -3 },
    { 33, -3 }, { -14, -2 }, { -10, -2 }, { -5, -2 }, { -3, -2 }, { -2, -2 }, { -1, -2 }, { 0, -2 },
    { 1, -2 }, { 2, -2 }, { 3, -2 }, { 5, -2 }, { 7, -2 }, { 14, -2 }, { 19, -2 }, { 25, -2 },
    { 43, -2 }, { -7, -1 }, { -3, -1 }, { -2, -1 }, { -1, -1 }, { 0, -1 }, { 1, -1 }, { 2, -1 },
    { 3, -1 }, { 10, -1 }, { -5, 0 }, { -3, 0 }, { -2, 0 }, { -1, 0 }, { 1, 0 }, { 2, 0 },
    { 3, 0 }, { 5, 0 }, { 7, 0 }, { -10, 1 }, { -7, 1 }, { -3, 1 }, { -2, 1 }, { -1, 1 },
    { 0, 1 }, { 1, 1 }, { 2, 1 }, { 3, 1 }, { -43, 2 }, { -25, 2 }, { -19, 2 }, { -14, 2 },
    { -5, 2 }, { -3, 2 }, { -2, 2 }, { -1, 2 }, { 0, 2 }, { 1, 2 }, { 2, 2 }, { 3, 2 },
    { 5, 2 }, { 7, 2 }, { 10, 2 }, { 14, 2 }, { -33, 3 }, { -6, 3 }, { -4, 3 }, { -2, 3 },
    { -1, 3 }, { 0, 3 }, { 1, 3 }, { 2, 3 }, { 4, 3 }, { 19, 3 }, { -9, 4 }, { -3, 4 },
    { 3, 4 }, { 7, 4 }, { 25, 4 }, { -13, 5 }, { -5, 5 }, { -2, 5 }, { 0, 5 }, { 2, 5 },
    { 5, 5 }, { 9, 5 }, { 33, 5 }, { -8, 6 }, { -4, 6 }, { 4, 6 }, { 13, 6 }, { 43, 6 },
    { -18, 7 }, { -2, 7 }, { 0, 7 }, { 2, 7 }, { 7, 7 }, { 18, 7 }, { -24, 8 }, { -6, 8 },
    { -42, 9 }, { -11, 9 }, { -4, 9 }, { 5, 9 }, { 11, 9 }, { 23, 9 }, { -31, 10 }, { -1, 10 },
    { 2, 10 }, { -15, 11 }, { -8, 11 }, { 8, 11 }, { 15, 11 }, { 31, 12 }, { -21, 13 }, { -5, 13 },
    { 5, 13 }, { 41, 13 }, { -1, 14 }, { 1, 14 }, { 21, 14 }, { -12, 15 }, { 12, 15 }, { -39, 17 },
    { -28, 17 }, { -18, 17 }, { -8, 17 }, { 8, 17 }, { 17, 18 }, { -4, 19 }, { 0, 19 }, { 4, 19 },
    { 27, 19 }, { 38, 20 }, { -13, 21 }, { 12, 22 }, { -36, 23 }, { -24, 23 }, { -8, 24 }, { 7, 24 },
    { -3, 25 }, { 1, 25 }, { 22, 25 }, { 34, 26 }, { -18, 28 }, { -32, 29 }, { 16, 29 }, { -11, 31 },
    { 9, 32 }, { 29, 32 }, { -4, 33 }, { 2, 33 }, { -26, 34 }, { 23, 36 }, { -19, 39 }, { 16, 40 },
    { -13, 41 }, { 9, 42 }, { -6, 43 }, { 1, 43 }, { 0, 0 }, { 0, 0 }, { 0, 0 }
};
static int dbloc16_lastWidth = -1;

// The glyph edge points (x and y tables for the 4x4 and the 8x8 glyphs)
static const int dbloc16_small1[16] = { 0, 1, 2, 3, 3, 3, 3, 2, 1, 0, 0, 0, 1, 2, 2, 1 };
static const int dbloc16_small2[16] = { 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 2, 1, 1, 1, 2, 2 };
static const int dbloc16_big1[16]   = { 0, 2, 5, 7, 7, 7, 7, 7, 7, 5, 2, 0, 0, 0, 0, 0 };
static const int dbloc16_big2[16]   = { 0, 0, 0, 0, 1, 3, 4, 6, 7, 7, 7, 7, 6, 4, 3, 1 };

static int dbloc16_prevSeq = -1;

static MemoryDebugInterface* dbloc16_pMemory = NULL;
static BitmapInterface* dbloc16_pBitmap      = NULL;
static LecShare* dbloc16_pShare              = NULL;
static void* dbloc16_pParam                  = NULL;
static Bl16Blit dbloc16_pBlit                = NULL;
static LecBitmapHeader* dbloc16_pShown       = NULL;
static uint16_t* dbloc16_pCur                = NULL;
static uint16_t* dbloc16_pLut                = NULL;
static int dbloc16_red                       = 0;
static int dbloc16_green                     = 0;
static int dbloc16_blue                      = 0;
static int dbloc16_depth                     = 0;
static LecBitmapHeader* dbloc16_pBitmap0     = NULL;
static LecBitmapHeader* dbloc16_pBitmap1     = NULL;
static LecBitmapHeader* dbloc16_pBitmap2     = NULL;
static int dbloc16_size                      = 0;
static uint16_t* dbloc16_pLast               = NULL;

// The two delta buffers, the motion offsets and the glyph tables codec 2 reads
static uint16_t* dbloc16_pBuf0;
static Bl16Big dbloc16_big[BL16_NUMTABLES];
static uint16_t* dbloc16_pBuf1;
static Bl16Small dbloc16_small[BL16_NUMTABLES];
static int16_t dbloc16_offsets[256];

void D_bloc16_InstallHooks(void)
{
    J3D_HOOKFUNC(dbloc16_Register);
    J3D_HOOKFUNC(dbloc16_Configure);
    J3D_HOOKFUNC(dbloc16_Unregister);
    J3D_HOOKFUNC(dbloc16_Startup);
    J3D_HOOKFUNC(dbloc16_Shutdown);
    J3D_HOOKFUNC(dbloc16_Name);
    J3D_HOOKFUNC(dbloc16_GetTag);
    J3D_HOOKFUNC(dbloc16_Reset);
    J3D_HOOKFUNC(dbloc16_Parse);
    J3D_HOOKFUNC(dbloc16_Alloc);
    J3D_HOOKFUNC(dbloc16_Free);
    J3D_HOOKFUNC(dbloc16_Decode);
    J3D_HOOKFUNC(dbloc16_Query);
    J3D_HOOKFUNC(dbloc16_MakeTables);
    J3D_HOOKFUNC(dbloc16_MakeGlyphs);
    J3D_HOOKFUNC(dbloc16_DecodeFrame);
    J3D_HOOKFUNC(dbloc16_BuildLut);
    J3D_HOOKFUNC(dbloc16_Scale);
    J3D_HOOKFUNC(dbloc16_Average);
    J3D_HOOKFUNC(dbloc16_ScalePal);
}

void D_bloc16_ResetGlobals(void)
{}

int J3DAPI dbloc16_Register(LecShare* pShare, void* pParam)
{
    dbloc16_pShare = pShare;
    dbloc16_pParam = pParam;
    pShare->Publish("LECMODULE=insane_blocky16_flobdec", &dbloc16_module);
    dbloc16_pShare->Publish("FLOBDEC=insane_blocky16", &dbloc16_interface);
    return 0;
}

int J3DAPI dbloc16_Configure(int tag, ...)
{
    va_list args;

    va_start(args, tag);
    while ( tag )
    {
        switch ( tag )
        {
            case 1:
                dbloc16_pBlit = va_arg(args, Bl16Blit);
                break;

            case 2:
                *va_arg(args, Bl16Blit*) = dbloc16_pBlit;
                break;

            case 3:
                *va_arg(args, LecBitmapHeader**) = dbloc16_pShown;
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

void dbloc16_Unregister(void)
{
    dbloc16_pShare->Unpublish("FLOBDEC=insane_blocky16");
    dbloc16_pShare->Unpublish("LECMODULE=insane_blocky16_flobdec");
}

int dbloc16_Startup(void)
{
    dbloc16_pBitmap = dbloc16_pShare->Subscribe("BITMAP.clear", dbloc16_pParam);
    dbloc16_pMemory = dbloc16_pShare->Subscribe("MEMORY.workspace", dbloc16_pParam);
    if ( dbloc16_pBitmap == NULL )
    {
        dbloc16_pShare->ReportError("FlobDec-Blocky16 Module: Cannot allocate bitmap services.");
        return -1;
    }

    if ( dbloc16_pMemory == NULL )
    {
        dbloc16_pShare->ReportError("FlobDec-Blocky16 Module: Cannot allocate memory services.");
        return -1;
    }

    return 0;
}

void dbloc16_Shutdown(void)
{
    dbloc16_Reset();
    dbloc16_pShare->Unsubscribe(dbloc16_pBitmap);
    dbloc16_pShare->Unsubscribe(dbloc16_pMemory);
}

const char* J3DAPI dbloc16_Name(int index)
{
    switch ( index )
    {
        case 0:
            return "Blocky16-Tile";

        case 1:
            return "Decompresses a block16-tile video frame.";

        case 2:
            return "This module decompresses the block16-tile compressed animations used in Outlaws, Monkey Island III, and Shadows of the Empire PC. Images are deltad, so they must be decompressed sequentially from frame 0.";

        default:
            return "";
    }
}

int dbloc16_GetTag(void)
{
    return 'Bl16';
}

int dbloc16_GetFlags(void)
{
    return 0;
}

void dbloc16_Reset(void)
{
    dbloc16_Free();
}

int J3DAPI dbloc16_Parse(const Bl16Parse* pData, int size)
{
    int width  = pData->width;
    int height = (pData->height + 7) & ~7;

    if ( pData->depth != dbloc16_depth || dbloc16_pBitmap0 == NULL || width != dbloc16_pBitmap0->width || height != dbloc16_pBitmap0->height )
    {
        dbloc16_Free();
        dbloc16_Alloc(width, height, pData->depth);
    }

    return 0;
}

int J3DAPI dbloc16_Alloc(int width, int height, int depth)
{
    dbloc16_size = width * height;
    dbloc16_MakeGlyphs(4);
    dbloc16_MakeGlyphs(8);
    dbloc16_red   = 0;
    dbloc16_green = 0;
    dbloc16_blue  = 0;

    int rc = dbloc16_pBitmap->Create(&dbloc16_pBitmap0, width, height, depth);
    if ( rc == 0 )
    {
        rc = dbloc16_pBitmap->Create(&dbloc16_pBitmap1, width, height, depth);
        if ( rc == 0 )
        {
            rc = dbloc16_pBitmap->Create(&dbloc16_pBitmap2, width, height, depth);
            if ( rc == 0 )
            {
                dbloc16_depth = depth;
                dbloc16_pBuf0 = dbloc16_pBitmap0->pPixels;
                dbloc16_pBuf1 = dbloc16_pBitmap1->pPixels;
                dbloc16_pCur  = dbloc16_pBitmap2->pPixels;
                framestream_CopyB(dbloc16_pBuf0, 0, dbloc16_size * 2);
                framestream_CopyB(dbloc16_pBuf1, 0, dbloc16_size * 2);
                framestream_CopyB(dbloc16_pCur, 0, dbloc16_size * 2);

                dbloc16_pLut = dbloc16_pMemory->Alloc(0x20000, INSANE_FILE, __LINE__);
                rc = dbloc16_pLut ? 0 : -1;
                if ( rc == 0 ) {
                    return 0;
                }

                dbloc16_pBitmap->Destroy(&dbloc16_pBitmap2);
            }

            dbloc16_pBitmap->Destroy(&dbloc16_pBitmap1);
        }

        dbloc16_pBitmap->Destroy(&dbloc16_pBitmap0);
    }

    return rc;
}

void dbloc16_Free(void)
{
    if ( dbloc16_pBitmap0 != NULL ) {
        dbloc16_pBitmap->Destroy(&dbloc16_pBitmap0);
    }

    if ( dbloc16_pBitmap1 != NULL ) {
        dbloc16_pBitmap->Destroy(&dbloc16_pBitmap1);
    }

    if ( dbloc16_pBitmap2 != NULL ) {
        dbloc16_pBitmap->Destroy(&dbloc16_pBitmap2);
    }

    if ( dbloc16_pLut != NULL )
    {
        dbloc16_pMemory->Free(dbloc16_pLut, INSANE_FILE, __LINE__);
        dbloc16_pLut = NULL;
    }

    dbloc16_pShown = NULL;
    dbloc16_size   = 0;
}

int J3DAPI dbloc16_Decode(void* a1, int a2, int a3, int a4, void* a5, const uint8_t* pData, int size, int mode, int a9, int a10)
{
    return dbloc16_DecodeFrame(a1, (MixerRect*)a2, a3, a4, a5, (Bl16Header*)pData, mode);
}

int dbloc16_Query1(void)
{
    return -1;
}

int J3DAPI dbloc16_Query(const Bl16Header* pData, int tag, int* pX, int* pY, int* pWidth, int* pHeight)
{
    if ( tag != 0x100 ) {
        return -1;
    }

    if ( pX ) {
        *pX = pData->x;
    }

    if ( pY ) {
        *pY = pData->y;
    }

    if ( pWidth ) {
        *pWidth = pData->width;
    }

    if ( pHeight ) {
        *pHeight = pData->height;
    }

    return 0;
}

void J3DAPI dbloc16_MakeTables(int width)
{
    if ( width == dbloc16_lastWidth ) {
        return;
    }

    dbloc16_lastWidth = width;
    for ( int i = 0; i < 255; i++ ) {
        dbloc16_offsets[i] = (int16_t)(dbloc16_motion[i].y * width + dbloc16_motion[i].x);
    }

    for ( int i = 0; i < 256; i++ )
    {
        for ( int j = 0; j < dbloc16_small[i].n1; j++ ) {
            dbloc16_small[i].off1[j] = (uint16_t)((dbloc16_small[i].idx1[j] >> 2) * width + (dbloc16_small[i].idx1[j] & 3));
        }

        for ( int j = 0; j < dbloc16_small[i].n2; j++ ) {
            dbloc16_small[i].off2[j] = (uint16_t)((dbloc16_small[i].idx2[j] >> 2) * width + (dbloc16_small[i].idx2[j] & 3));
        }

        for ( int j = 0; j < dbloc16_big[i].n1; j++ ) {
            dbloc16_big[i].off1[j] = (uint16_t)((dbloc16_big[i].idx1[j] >> 3) * width + (dbloc16_big[i].idx1[j] & 7));
        }

        for ( int j = 0; j < dbloc16_big[i].n2; j++ ) {
            dbloc16_big[i].off2[j] = (uint16_t)((dbloc16_big[i].idx2[j] >> 3) * width + (dbloc16_big[i].idx2[j] & 7));
        }
    }
}

void J3DAPI dbloc16_MakeGlyphs(int size)
{
    const int* pEdgeX = NULL;
    const int* pEdgeY = NULL;
    int glyph[64];

    if ( size == 8 )
    {
        pEdgeX = dbloc16_big1;
        pEdgeY = dbloc16_big2;
        for ( int i = 0; i < 256; i++ ) {
            dbloc16_big[i].n1 = 0;
        }

        for ( int i = 0; i < 256; i++ ) {
            dbloc16_big[i].n2 = 0;
        }
    }
    else if ( size == 4 )
    {
        pEdgeX = dbloc16_small1;
        pEdgeY = dbloc16_small2;
        for ( int i = 0; i < 256; i++ ) {
            dbloc16_small[i].n1 = 0;
        }

        for ( int i = 0; i < 256; i++ ) {
            dbloc16_small[i].n2 = 0;
        }
    }

    int entry = 0;
    for ( int x = 0; x < 16; x++ )
    {
        for ( int y = 0; y < 16; y++ )
        {
            int edge1;
            int y1 = pEdgeY[x];
            if ( y1 == 0 ) {
                edge1 = 0;
            }
            else if ( y1 == size - 1 ) {
                edge1 = 1;
            }
            else if ( pEdgeX[x] == 0 ) {
                edge1 = 2;
            }
            else if ( pEdgeX[x] == size - 1 ) {
                edge1 = 3;
            }
            else {
                edge1 = 4;
            }

            int edge2;
            int y2 = pEdgeY[y];
            if ( y2 == 0 ) {
                edge2 = 0;
            }
            else if ( y2 == size - 1 ) {
                edge2 = 1;
            }
            else if ( pEdgeX[y] == 0 ) {
                edge2 = 2;
            }
            else if ( pEdgeX[y] == size - 1 ) {
                edge2 = 3;
            }
            else {
                edge2 = 4;
            }

            int i = size * size;
            while ( i-- != 0 ) {
                glyph[i] = 0;
            }

            int n  = DBLOC16_MAX(DBLOC16_ABS(pEdgeX[x] - pEdgeX[y]), DBLOC16_ABS(pEdgeY[x] - pEdgeY[y])) + 1;
            int x1 = pEdgeX[x];
            int x2 = pEdgeX[y];
            for ( i = 0; i < n; i++ )
            {
                int px = x1;
                int py = y1;
                if ( n > 1 )
                {
                    px = (x1 * i + ((n - 1) >> 1) + x2 * (n - 1 - i)) / (n - 1);
                    py = (y1 * i + ((n - 1) >> 1) + y2 * (n - 1 - i)) / (n - 1);
                }

                glyph[py * size + px] = 1;
                if ( (edge1 == 2 && edge2 == 3) || (edge2 == 2 && edge1 == 3) || (edge1 == 0 && edge2 != 1) || (edge2 == 0 && edge1 != 1) )
                {
                    for ( int k = py; k >= 0; k-- ) {
                        glyph[k * size + px] = 1;
                    }
                }
                else if ( (edge1 == 1 && edge2 != 0) || (edge2 == 1 && edge1 != 0) )
                {
                    for ( int k = py; k < size; k++ ) {
                        glyph[k * size + px] = 1;
                    }
                }
                else if ( (edge1 == 0 && edge2 == 1) || (edge2 == 0 && edge1 == 1) || (edge1 == 3 && edge2 != 2) || (edge2 == 3 && edge1 != 2) )
                {
                    for ( int k = px; k < size; k++ ) {
                        glyph[py * size + k] = 1;
                    }
                }
                else if ( (edge1 == 2 && edge2 != 3) || (edge2 == 2 && edge1 != 3) )
                {
                    for ( int k = px; k >= 0; k-- ) {
                        glyph[py * size + k] = 1;
                    }
                }
            }

            if ( size == 8 )
            {
                i = 64;
                while ( i-- != 0 )
                {
                    if ( glyph[i] != 0 )
                    {
                        dbloc16_big[entry].idx1[dbloc16_big[entry].n1] = (uint8_t)i;
                        dbloc16_big[entry].n1++;
                    }
                    else
                    {
                        dbloc16_big[entry].idx2[dbloc16_big[entry].n2] = (uint8_t)i;
                        dbloc16_big[entry].n2++;
                    }
                }
            }
            else if ( size == 4 )
            {
                i = 16;
                while ( i-- != 0 )
                {
                    if ( glyph[i] != 0 )
                    {
                        dbloc16_small[entry].idx1[dbloc16_small[entry].n1] = (uint8_t)i;
                        dbloc16_small[entry].n1++;
                    }
                    else
                    {
                        dbloc16_small[entry].idx2[dbloc16_small[entry].n2] = (uint8_t)i;
                        dbloc16_small[entry].n2++;
                    }
                }
            }

            entry++;
        }
    }
}

int J3DAPI dbloc16_DecodeFrame(LecBitmapHeader* pBitmap, MixerRect* pClip, int x, int y, void* pPalette, Bl16Header* pHdr, int flags)
{
    int rc;

    int width      = pHdr->width;
    int height     = pHdr->height;
    uint8_t* pGfx  = pHdr->data;
    if ( dbloc16_pBitmap0 == NULL || width != dbloc16_pBitmap0->width || ((height + 7) & ~7) != dbloc16_pBitmap0->height )
    {
        dbloc16_Free();
        dbloc16_Alloc(width, (height + 7) & ~7, pHdr->depth ? pHdr->depth : 8);
    }

    int offset1 = (int)(dbloc16_pBuf1 - dbloc16_pCur);
    int offset2 = (int)(dbloc16_pBuf0 - dbloc16_pCur);
    if ( pBitmap ) {
        dbloc16_BuildLut(pBitmap);
    }

    int nextLine = width * 8 - (width & ~7);
    if ( pHdr->seq == 0 )
    {
        dbloc16_MakeTables(width);
        decode_Run((uint8_t*)dbloc16_pBuf0, dbloc16_pBitmap0, dbloc16_pBitmap0->width * dbloc16_pBitmap0->height, pHdr->fill);
        decode_Run((uint8_t*)dbloc16_pBuf1, dbloc16_pBitmap0, dbloc16_pBitmap0->width * dbloc16_pBitmap0->height, pHdr->fill);
        dbloc16_prevSeq = -1;
    }

    switch ( pHdr->codec )
    {
        case 0:
            framestream_Copy(dbloc16_pCur, pGfx, width * height * 2);
            dbloc16_pLast = dbloc16_pCur;
            break;

        case 6:
        {
            uint16_t* pDst = dbloc16_pCur;
            int n          = width * height;
            while ( n-- != 0 ) {
                *pDst++ = pHdr->palette[*pGfx++];
            }

            dbloc16_pLast = dbloc16_pCur;
        } break;

        case 1:
            dbloc16_Scale(dbloc16_pCur, (uint16_t*)pGfx, width, height);
            dbloc16_pLast = dbloc16_pCur;
            break;

        case 7:
            dbloc16_ScalePal(dbloc16_pCur, pGfx, width, height, pHdr->palette);
            dbloc16_pLast = dbloc16_pCur;
            break;

        case 8:
        {
            decode_RleInit(pGfx);
            uint16_t* pDst = dbloc16_pCur;
            int n          = width * height;
            while ( n-- != 0 ) {
                *pDst++ = pHdr->palette[decode_RleByte()];
            }

            decode_RleEnd();
            dbloc16_pLast = dbloc16_pCur;
        } break;

        case 5:
            decode_Rle((uint8_t*)dbloc16_pCur, pGfx, pHdr->rleSize);
            dbloc16_pLast = dbloc16_pCur;
            break;

        case 4:
            if ( (flags & 4) && !pHdr->rotate )
            {
                dbloc16_prevSeq = pHdr->seq;
                return 0;
            }

            framestream_Copy(dbloc16_pCur, dbloc16_pBuf0, width * height * 2);
            dbloc16_pLast = dbloc16_pCur;
            break;

        case 3:
            if ( (flags & 4) && !pHdr->rotate )
            {
                dbloc16_prevSeq = pHdr->seq;
                return 0;
            }

            framestream_Copy(dbloc16_pCur, dbloc16_pBuf1, width * height * 2);
            dbloc16_pLast = dbloc16_pCur;
            break;

        case 2:
            if ( (flags & 4) && !pHdr->rotate )
            {
                dbloc16_prevSeq = pHdr->seq;
                return 0;
            }

            if ( pHdr->seq == dbloc16_prevSeq + 1 )
            {
                dbloc16_Codec2(dbloc16_pCur, pGfx, offset1, offset2, width, dbloc16_offsets, pHdr->params, height, nextLine + width, dbloc16_small, dbloc16_big, pHdr->palette);
                dbloc16_pLast = dbloc16_pCur;
            }
            break;
    }

    if ( dbloc16_pLast != NULL && pBitmap != NULL )
    {
        uint16_t* pLut = (dbloc16_red || dbloc16_green) ? dbloc16_pLut : NULL;
        if ( dbloc16_pBlit != NULL )
        {
            if ( dbloc16_pLast == dbloc16_pBitmap0->pPixels )
            {
                dbloc16_pShown = dbloc16_pBitmap0;
                rc = dbloc16_pBlit(pBitmap, pClip, x, y, dbloc16_pBitmap0, pPalette, pLut);
            }
            else if ( dbloc16_pLast == dbloc16_pBitmap1->pPixels )
            {
                dbloc16_pShown = dbloc16_pBitmap1;
                rc = dbloc16_pBlit(pBitmap, pClip, x, y, dbloc16_pBitmap1, pPalette, pLut);
            }
            else if ( dbloc16_pLast == dbloc16_pBitmap2->pPixels )
            {
                dbloc16_pShown = dbloc16_pBitmap2;
                rc = dbloc16_pBlit(pBitmap, pClip, x, y, dbloc16_pBitmap2, pPalette, pLut);
            }
        }
        else
        {
            if ( dbloc16_pLast == dbloc16_pBitmap0->pPixels )
            {
                dbloc16_pShown = dbloc16_pBitmap0;
                mixer_Blit(pBitmap, pClip, x, y, dbloc16_pBitmap0, pPalette, pLut);
            }
            else if ( dbloc16_pLast == dbloc16_pBitmap1->pPixels )
            {
                dbloc16_pShown = dbloc16_pBitmap1;
                mixer_Blit(pBitmap, pClip, x, y, dbloc16_pBitmap1, pPalette, pLut);
            }
            else if ( dbloc16_pLast == dbloc16_pBitmap2->pPixels )
            {
                dbloc16_pShown = dbloc16_pBitmap2;
                mixer_Blit(pBitmap, pClip, x, y, dbloc16_pBitmap2, pPalette, pLut);
            }

            rc = 1;
        }
    }

    if ( pHdr->seq == dbloc16_prevSeq + 1 )
    {
        if ( pHdr->rotate == 1 )
        {
            uint16_t* pTmp = dbloc16_pCur;
            dbloc16_pCur   = dbloc16_pBuf1;
            dbloc16_pBuf1  = pTmp;
        }
        else if ( pHdr->rotate == 2 )
        {
            uint16_t* pTmp = dbloc16_pBuf0;
            dbloc16_pBuf0  = dbloc16_pBuf1;
            dbloc16_pBuf1  = dbloc16_pCur;
            dbloc16_pCur   = pTmp;
        }
    }

    dbloc16_prevSeq = pHdr->seq;
    return rc; // TODO: [BUG] rc is uninitialized when no frame was blitted (pBitmap is NULL, nothing decoded yet, or the blit callback was not given a known bitmap)
}

void J3DAPI dbloc16_BuildLut(LecBitmapHeader* pBitmap)
{
    if ( dbloc16_pLut != NULL
        && pBitmap->pixelSize == 2
        && (pBitmap->colorMode == 1 || pBitmap->colorMode == 2)
        && (pBitmap->format.redPosShift != dbloc16_pBitmap0->format.redPosShift || pBitmap->format.bluePosShift != dbloc16_pBitmap0->format.bluePosShift || pBitmap->format.greenPosShift != dbloc16_pBitmap0->format.greenPosShift)
        && (pBitmap->format.redPosShift != dbloc16_red || pBitmap->format.bluePosShift != dbloc16_blue || pBitmap->format.greenPosShift != dbloc16_green) )
    {
        dbloc16_red   = pBitmap->format.redPosShift;
        dbloc16_green = pBitmap->format.greenPosShift;
        dbloc16_blue  = pBitmap->format.bluePosShift;
        for ( int i = 0; i < 0x10000; i++ ) {
            dbloc16_pLut[i] = (uint16_t)i;
        }

        decode_Convert((uint8_t*)dbloc16_pLut, dbloc16_pBitmap0, 0x10000, NULL, (uint8_t*)dbloc16_pLut, pBitmap, NULL);
    }
}

void J3DAPI dbloc16_Scale(uint16_t* pDst, uint16_t* pSrc, int width, int height)
{
    uint16_t* pOut = pDst + width;
    for ( int y = 0; y < height; y += 2 )
    {
        unsigned cur = *pSrc++;
        *(unsigned*)pOut = (cur << 16) | cur;
        pOut += 2;
        for ( int x = width - 2; x > 0; x -= 2 )
        {
            unsigned prev = cur;
            cur = *pSrc++;
            *(unsigned*)pOut = (cur << 16) | dbloc16_Average((uint16_t)prev, (uint16_t)cur);
            pOut += 2;
        }

        pOut += width;
    }

    framestream_Copy(pDst, pDst + width, width * 2);
    pOut = pDst + width * 2;
    for ( int y = 0; y < height - 2; y += 2 )
    {
        int x = width;
        while ( x-- != 0 )
        {
            *pOut = dbloc16_Average(pOut[width], pOut[-width]);
            pOut++;
        }

        pOut += width;
    }
}

uint16_t J3DAPI dbloc16_Average(uint16_t a, uint16_t b)
{
    return ((((b & 0x7E0) + (a & 0x7E0)) & 0xFC0) | (((b & 0xF800) + (a & 0xF800)) & 0x1F000) | ((b & 0x1F) + (a & 0x1F))) >> 1;
}

void J3DAPI dbloc16_ScalePal(uint16_t* pDst, uint8_t* pSrc, int width, int height, uint16_t* pPalette)
{
    uint16_t* pOut = pDst + width;
    for ( int y = 0; y < height; y += 2 )
    {
        unsigned cur = pPalette[*pSrc++];
        *(unsigned*)pOut = (cur << 16) | cur;
        pOut += 2;
        for ( int x = width - 2; x > 0; x -= 2 )
        {
            unsigned prev = cur;
            cur = pPalette[*pSrc++];
            *(unsigned*)pOut = (cur << 16) | dbloc16_Average((uint16_t)prev, (uint16_t)cur);
            pOut += 2;
        }

        pOut += width;
    }

    framestream_Copy(pDst, pDst + width, width * 2);
    pOut = pDst + width * 2;
    for ( int y = 0; y < height - 2; y += 2 )
    {
        int x = width;
        while ( x-- != 0 )
        {
            *pOut = dbloc16_Average(pOut[width], pOut[-width]);
            pOut++;
        }

        pOut += width;
    }
}

#if defined(J3D_TEST)
// Added: Test-only access to the tables codec 2 reads.
void DBloc16_TestGetTables(const int16_t** ppOffsets, const Bl16Small** ppSmall, const Bl16Big** ppBig);
void DBloc16_TestGetTables(const int16_t** ppOffsets, const Bl16Small** ppSmall, const Bl16Big** ppBig)
{
    *ppOffsets = dbloc16_offsets;
    *ppSmall   = dbloc16_small;
    *ppBig     = dbloc16_big;
}
#endif
