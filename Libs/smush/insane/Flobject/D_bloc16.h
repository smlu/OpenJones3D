#ifndef SMUSH_INSANE_FLOBJECT_D_BLOC16_H
#define SMUSH_INSANE_FLOBJECT_D_BLOC16_H
#include <assert.h>
#include <j3dcore/j3d.h>
#include <smush/insane/lec.h>
#include <smush/insane/Memory/Memory.h>
#include <smush/insane/Bitmap/Bitmap.h>
#include <smush/insane/Sound/Mixer.h>

/*
 * D_bloc16.c: the blocky16 frame-object decoder (`FLOBJECT=insane_d_bloc16`); codec 2's block decoder is Bloc16Asm.asm.
 * The structures below are serialized or shared with the assembly and are packed like the original (/Zp1).
 */

J3D_EXTERN_C_START

/* The `Bl16` chunk as the decoder reads it in place (offsets from the original; the 16-bit fields are padded to 32). */
#pragma pack(push, 1)
typedef struct sBl16Header
{
    int16_t x;                      /* 0x000 */
    int16_t pad02;
    int16_t y;                      /* 0x004 */
    int16_t pad06;
    int16_t width;                  /* 0x008 */
    int16_t pad0A;
    int16_t height;                 /* 0x00C */
    int16_t pad0E;
    int16_t seq;                    /* 0x010 */
    uint8_t codec;                  /* 0x012 */
    uint8_t rotate;                 /* 0x013 */
    uint8_t pad14[4];
    uint8_t params[8];              /* 0x018: codec 2's motion parameters */
    uint8_t fill[4];                /* 0x020: the pixel the delta buffers start with */
    int rleSize;                    /* 0x024 */
    uint16_t palette[256];          /* 0x028 */
    int16_t depth;                  /* 0x228 */
    int16_t pad22A[3];
    uint8_t data[1];                /* 0x230 */
} Bl16Header;
#pragma pack(pop)
static_assert(sizeof(Bl16Header) == 0x231, "sizeof(Bl16Header) == 0x231");
static_assert(offsetof(Bl16Header, data) == 0x230, "offsetof(Bl16Header, data) == 0x230");

/* The frame object's parse header. */
typedef struct sBl16Parse
{
    int16_t codec;
    int16_t width;
    int16_t height;
    int16_t depth;
} Bl16Parse;
static_assert(sizeof(Bl16Parse) == 8, "sizeof(Bl16Parse) == 8");

typedef struct sBl16Motion
{
    int16_t x;
    int16_t y;
} Bl16Motion;
static_assert(sizeof(Bl16Motion) == 4, "sizeof(Bl16Motion) == 4");

/* The glyph tables (per from-edge/to-edge pair, the pixel offsets of the two halves of a 4x4 or 8x8 glyph); read by Bloc16Asm. */
typedef struct sBl16Small
{
    uint16_t off1[16];               /* 0x00 */
    uint16_t off2[16];               /* 0x20 */
    uint8_t idx1[16];               /* 0x40 */
    uint8_t idx2[16];               /* 0x50 */
    uint8_t n1;                     /* 0x60 */
    uint8_t n2;                     /* 0x61 */
    uint8_t pad[30];
} Bl16Small;
static_assert(sizeof(Bl16Small) == 0x80, "sizeof(Bl16Small) == 0x80");

typedef struct sBl16Big
{
    uint16_t off1[64];               /* 0x000 */
    uint16_t off2[64];               /* 0x080 */
    uint8_t idx1[64];               /* 0x100 */
    uint8_t idx2[64];               /* 0x140 */
    uint8_t n1;                     /* 0x180 */
    uint8_t n2;                     /* 0x181 */
    uint8_t pad[2];
} Bl16Big;
static_assert(sizeof(Bl16Big) == 0x184, "sizeof(Bl16Big) == 0x184");

typedef struct sBl16Interface
{
    const char* (*Name)(int index);
    int  (*GetTag)(void);
    int  (*GetFlags)(void);
    void (*Reset)(void);
    int  (*Parse)(const Bl16Parse* pData, int size);
    int  (*Decode)(void* a1, int a2, int a3, int a4, void* a5, const uint8_t* pData, int size, int mode, int a9, int a10);
    int  (*Query1)(void);
    int  (*Query)(const Bl16Header* pData, int tag, int* pX, int* pY, int* pWidth, int* pHeight);
} Bl16Interface;
static_assert(sizeof(Bl16Interface) == 0x20, "sizeof(Bl16Interface) == 0x20");

typedef int (*Bl16Blit)(LecBitmapHeader* pDst, MixerRect* pClip, int x, int y, LecBitmapHeader* pSrc, void* pPalette, void* pTable);

#define BL16_NUMTABLES 256 /* glyph tables per width: dbloc16_small[256] (0x8000 bytes), dbloc16_big[256] (0x18400 bytes) */

int J3DAPI dbloc16_Register(LecShare* pShare, void* pParam);
int J3DAPI dbloc16_Configure(int tag, ...);
void dbloc16_Unregister(void);
int dbloc16_Startup(void);
void dbloc16_Shutdown(void);
const char* J3DAPI dbloc16_Name(int index);
int dbloc16_GetTag(void);
int dbloc16_GetFlags(void); // Folded onto 0x004E74A0; no address of its own, never hook
void dbloc16_Reset(void);
int J3DAPI dbloc16_Parse(const Bl16Parse* pData, int size);
int J3DAPI dbloc16_Alloc(int width, int height, int depth);
void dbloc16_Free(void);
int J3DAPI dbloc16_Decode(void* a1, int a2, int a3, int a4, void* a5, const uint8_t* pData, int size, int mode, int a9, int a10);
int dbloc16_Query1(void); // Folded onto 0x004E8260; no address of its own, never hook
int J3DAPI dbloc16_Query(const Bl16Header* pData, int tag, int* pX, int* pY, int* pWidth, int* pHeight);
void J3DAPI dbloc16_MakeTables(int width);
void J3DAPI dbloc16_MakeGlyphs(int size);
int J3DAPI dbloc16_DecodeFrame(LecBitmapHeader* pBitmap, MixerRect* pClip, int x, int y, void* pPalette, Bl16Header* pHdr, int flags);
void J3DAPI dbloc16_BuildLut(LecBitmapHeader* pBitmap);
void J3DAPI dbloc16_Scale(uint16_t* pDst, uint16_t* pSrc, int width, int height);
uint16_t J3DAPI dbloc16_Average(uint16_t a, uint16_t b);
void J3DAPI dbloc16_ScalePal(uint16_t* pDst, uint8_t* pSrc, int width, int height, uint16_t* pPalette);

void D_bloc16_InstallHooks(void);
void D_bloc16_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_FLOBJECT_D_BLOC16_H
